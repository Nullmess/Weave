#!/usr/bin/env python3
"""Weave USB CDC data snapshot of both Flipper storage volumes.

The archive mirrors readable filesystem content, not microcontroller OTP, radio
calibration, bootloader or protected secrets. Firmware is recorded as metadata;
restoring data cannot by itself install the recorded firmware version.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import hashlib
import json
import logging
import os
from pathlib import Path, PurePosixPath
import posixpath
import re
import shutil
import sys
import tempfile
import time
import zipfile

FORMAT = 'weave-flipper-snapshot-v1'
VOLUMES = ('int', 'ext')
MAX_FILES = 250_000
MAX_BYTES = 128 * 1024**3


def safe_relative(name: str) -> str:
    if not isinstance(name, str) or '\\' in name or not name or '\x00' in name:
        raise ValueError(f'Invalid snapshot path {name!r}')
    path = PurePosixPath(name)
    if path.is_absolute() or not path.parts or any(p in ('.', '..') for p in name.split('/')):
        raise ValueError(f'Unsafe snapshot path: {name!r}')
    if path.parts[0] not in VOLUMES or len(path.parts) < 2:
        raise ValueError(f'Unexpected snapshot volume: {name!r}')
    if any(not part or '"' in part or '\r' in part or '\n' in part for part in path.parts):
        raise ValueError(f'Unsafe Flipper filename: {name!r}')
    name.encode('ascii') # Flipper USB CDC CLI only supports ASCII paths
    return path.as_posix()


def get_port(requested: str):
    from flipper.utils.cdc import resolve_port
    port = resolve_port(logging.getLogger('WeaveSnapshot'), requested)
    if not port:
        raise RuntimeError('Flipper USB CDC not found. Connect it by USB outside DFU mode, and close qFlipper.')
    return port


def console_command(storage, command: str) -> str:
    storage.send_and_wait_eol(command + '\r')
    reply = storage.read.until(storage.CLI_PROMPT, timeout=20)
    return reply.decode('utf-8', errors='replace').strip()


def list_directory(storage, path: str):
    """Strict parser: never silently skip an unreadable file (partial backups)."""
    storage.send_and_wait_eol(f'storage list "{path}"\r')
    payload = storage.read.until(storage.CLI_PROMPT, timeout=30)
    if b'Storage error:' in payload:
        raise RuntimeError(f'Cannot list {path}: {payload.decode("utf-8", "replace").strip()}')
    directories, files = [], []
    for raw in payload.split(b'\r\n'):
        raw = raw.strip()
        if not raw or raw == b'Empty':
            continue
        text = raw.decode('ascii')  # fail rather than silently lose Unicode files
        if text.startswith('[D] '):
            directories.append(text[4:])
        elif text.startswith('[F] '):
            match = re.fullmatch(r'(.+)\s+(\d+)', text[4:])
            if not match:
                raise RuntimeError(f'Unknown CLI file listing: {text!r}')
            files.append((match.group(1), int(match.group(2))))
        else:
            raise RuntimeError(f'Unexpected CLI directory response in {path}: {text!r}')
    names = [n for n in directories] + [n for n, _ in files]
    if len(names) != len(set(names)):
        raise RuntimeError(f'Duplicate entry in device directory {path}')
    for name in names:
        safe_relative(path.strip('/') + '/' + name)
        if '/' in name:
            raise RuntimeError(f'Invalid device entry: {name!r}')
    return sorted(directories), sorted(files)


def walk_device(storage):
    seen = set()
    for volume in VOLUMES:
        root = '/' + volume
        if not storage.exist_dir(root):
            raise RuntimeError(f'{root} is unavailable; a full two-volume snapshot is impossible')
        stack = [(root, 0)]
        while stack:
            current, depth = stack.pop()
            if depth > 64:
                raise RuntimeError('Device folder nesting exceeds safety limit')
            rel = current.lstrip('/')
            seen.add(rel)
            dirs, files = list_directory(storage, current)
            yield ('dir', rel, 0)
            for name, size in files:
                rel_file = safe_relative(rel + '/' + name)
                if rel_file in seen:
                    raise RuntimeError('Duplicate Flipper file path')
                seen.add(rel_file)
                yield ('file', rel_file, size)
            for name in reversed(dirs):
                stack.append((posixpath.join(current, name), depth + 1))
            if len(seen) > MAX_FILES:
                raise RuntimeError('Snapshot exceeds maximum item count')


def file_hash(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def device_metadata(storage):
    return {'device_info': console_command(storage, 'device_info'),
            'version': console_command(storage, 'version get')}


def receive_to_disk(storage, source_path: str, dest: Path | None, expected_size: int) -> str:
    """Stream binary CLI chunks and return SHA-256, optionally writing the data."""
    if not hasattr(storage, 'port'):
        # In-memory fake endpoint in unit tests.
        data = storage.read_file(source_path)
        if len(data) != expected_size:
            raise RuntimeError(f'Incomplete read: {source_path}')
        if dest is not None:
            dest.write_bytes(data)
        return hashlib.sha256(data).hexdigest()
    block_size = min(512, storage.chunk_size)
    storage.send_and_wait_eol(f'storage read_chunks "{source_path}" {block_size}\r')
    answer = storage.read.until(storage.CLI_EOL, timeout=30)
    if storage.has_error(answer):
        raise RuntimeError(f'Unable to read {source_path}: {answer.decode("ascii", "replace")}')
    try:
        announced_size = int(answer.split(b': ')[1])
    except (IndexError, ValueError) as exc:
        raise RuntimeError(f'Unexpected file size response for {source_path}: {answer!r}') from exc
    if announced_size != expected_size:
        raise RuntimeError(f'File changed during backup: {source_path} ({announced_size} vs {expected_size})')
    got = 0
    digest = hashlib.sha256()
    output = dest.open('wb') if dest is not None else None
    try:
        while got < announced_size:
            storage.read.until('Ready?' + storage.CLI_EOL, timeout=30)
            storage.send('y')
            pending = min(announced_size - got, block_size)
            deadline = time.monotonic() + 30
            while pending:
                data = storage.port.read(pending)
                if data:
                    if output: output.write(data)
                    digest.update(data)
                    got += len(data)
                    pending -= len(data)
                    deadline = time.monotonic() + 30
                elif time.monotonic() >= deadline:
                    raise TimeoutError(f'USB read stalled at byte {got} of {announced_size}: {source_path}')
        storage.read.until(storage.CLI_PROMPT, timeout=30)
    finally:
        if output: output.close()
    return digest.hexdigest()


def backup(storage, destination: Path) -> None:
    if destination.exists():
        raise FileExistsError(f'Snapshot already exists: {destination} (will not overwrite)')
    if not destination.parent.is_dir():
        raise FileNotFoundError(f'Destination folder does not exist: {destination.parent}')
    count, total = 0, 0
    print('Weave backup: collecting /int and /ext over USB CDC (including .fap, settings and data).')
    with tempfile.TemporaryDirectory(prefix='weave-snapshot-') as temp:
        stage = Path(temp)
        records, dirs = [], []
        metadata = device_metadata(storage)
        for kind, name, expected_size in walk_device(storage):
            local = stage / name
            if kind == 'dir':
                local.mkdir(parents=True, exist_ok=True)
                dirs.append(name + '/')
                continue
            count += 1
            total += expected_size
            if total > MAX_BYTES:
                raise RuntimeError('Backup exceeds 128 GiB limit; no incomplete archive will be published')
            local.parent.mkdir(parents=True, exist_ok=True)
            print(f'[{count}] /{name} ({expected_size} bytes)', flush=True)
            digest = receive_to_disk(storage, '/' + name, local, expected_size)
            if local.stat().st_size != expected_size:
                raise RuntimeError(f'Incomplete Flipper transfer: /{name}')
            records.append({'path': name, 'size': expected_size, 'sha256': digest})
        manifest = {'format': FORMAT, 'created_utc': datetime.now(timezone.utc).isoformat(),
                    'volumes': list(VOLUMES), 'device': metadata,
                    'firmware_included': False, 'directories': sorted(dirs),
                    'files': sorted(records, key=lambda row: row['path'])}
        temp_output = destination.with_name(destination.name + '.partial')
        try:
            with zipfile.ZipFile(temp_output, 'w', zipfile.ZIP_DEFLATED, allowZip64=True) as archive:
                archive.writestr('manifest.json', json.dumps(manifest, indent=2, ensure_ascii=False))
                for name in dirs:
                    archive.writestr('storage/' + name, b'')
                for row in records:
                    archive.write(stage / row['path'], 'storage/' + row['path'])
            with zipfile.ZipFile(temp_output) as check:
                if check.testzip():
                    raise RuntimeError('Snapshot ZIP failed integrity check')
            temp_output.replace(destination)
        finally:
            temp_output.unlink(missing_ok=True)
    print(f'Weave backup: saved {count} files / {total} bytes to {destination}')
    print('Firmware, OTP hardware identifiers and inaccessible hardware secrets are NOT cloned.')


def validate_archive(archive: zipfile.ZipFile):
    try:
        info = archive.getinfo('manifest.json')
        if info.file_size > 32 * 1024 * 1024:
            raise ValueError('Oversized manifest')
        manifest = json.loads(archive.read('manifest.json'))
    except (KeyError, json.JSONDecodeError) as exc:
        raise ValueError('Missing or invalid Weave manifest') from exc
    if manifest.get('format') != FORMAT or manifest.get('volumes') != list(VOLUMES) or manifest.get('firmware_included') is not False:
        raise ValueError('Invalid or unsupported Weave snapshot')
    rows, dirs = manifest.get('files'), manifest.get('directories')
    if not isinstance(rows, list) or not isinstance(dirs, list) or len(rows) + len(dirs) > MAX_FILES:
        raise ValueError('Invalid snapshot inventory')
    seen, size = set(), 0
    for row in rows:
        if not isinstance(row, dict): raise ValueError('Malformed file descriptor')
        name = safe_relative(row.get('path'))
        if name in seen: raise ValueError('Duplicate snapshot file')
        seen.add(name)
        length = row.get('size')
        digest = row.get('sha256')
        if type(length) is not int or length < 0 or not re.fullmatch('[0-9a-f]{64}', digest or ''):
            raise ValueError(f'Invalid hash or size in {name}')
        info = archive.getinfo('storage/' + name)
        if info.is_dir() or info.file_size != length:
            raise ValueError(f'Mismatched snapshot file length: {name}')
        size += length
        if size > MAX_BYTES: raise ValueError('Snapshot exceeds 128 GiB')
    for name in dirs:
        if not isinstance(name, str) or not name.endswith('/'):
            raise ValueError('Malformed snapshot directory')
        safe_relative(name.rstrip('/') + '/placeholder')
        if not archive.getinfo('storage/' + name).is_dir():
            raise ValueError('Malformed directory ZIP entry')
    expected = {'manifest.json'} | {'storage/' + row['path'] for row in rows} | {'storage/' + name for name in dirs}
    if set(archive.namelist()) != expected or len(archive.infolist()) != len(expected):
        raise ValueError('Unexpected or duplicate archive entries')
    return manifest, size


def unpack_verified(archive: zipfile.ZipFile, manifest: dict, stage: Path):
    for name in manifest['directories']:
        (stage / name).mkdir(parents=True, exist_ok=True)
    for row in manifest['files']:
        name = row['path']
        target = stage / name
        target.parent.mkdir(parents=True, exist_ok=True)
        with archive.open('storage/' + name) as source, target.open('wb') as output:
            shutil.copyfileobj(source, output, length=512 * 1024)
        if file_hash(target) != row['sha256']:
            raise ValueError(f'SHA-256 verification failed for {name}')


def restore(storage, source: Path, *, dry_run: bool = False) -> None:
    with zipfile.ZipFile(source, 'r') as archive:
        manifest, total = validate_archive(archive)
        print(f"Weave restore: snapshot {manifest.get('created_utc','date unknown')} / {len(manifest['files'])} files / {total} bytes")
        print('Firmware version in snapshot:', str(manifest.get('device', {}).get('version', 'unknown')).splitlines()[:2])
        print('Existing Flipper data NOT in the snapshot will be REMOVED from /int and /ext.')
        print('The backup does not restore firmware, hardware OTP, radio calibration or inaccessible secrets.')
        # Validate the entire snapshot before examining or modifying a connected device.
        with tempfile.TemporaryDirectory(prefix='weave-restore-') as temp:
            stage = Path(temp)
            unpack_verified(archive, manifest, stage)
            device = list(walk_device(storage))
            current_files = {name for kind, name, _ in device if kind == 'file'}
            current_dirs = {name for kind, name, _ in device if kind == 'dir'}
            desired_files = {row['path'] for row in manifest['files']}
            desired_dirs = {d.rstrip('/') for d in manifest['directories']}
            extras = sorted(current_files - desired_files)
            old_dirs = sorted(current_dirs - desired_dirs - set(VOLUMES), key=lambda p: (-p.count('/'), p))
            print(f'Planned: overwrite/write {len(desired_files)}, remove {len(extras)} old files and {len(old_dirs)} old folders.')
            if dry_run:
                print('Dry-run: Flipper not modified.')
                return
            if not sys.stdin.isatty():
                raise RuntimeError('Interactive RESTORE confirmation required; no changes made')
            answer = input('Type RESTORE to replace both internal and SD data (potential data loss): ')
            if answer != 'RESTORE':
                print('Restore cancelled without modification.')
                return
            # Remove extras, then recreate directories and overwrite file contents.
            for name in extras:
                print('Removing /' + name)
                storage.remove('/' + name)
            for name in old_dirs:
                print('Removing directory /' + name)
                storage.remove('/' + name)
            from flipper.storage import FlipperStorageOperations
            ops = FlipperStorageOperations(storage)
            for name in sorted(desired_dirs, key=lambda p: (p.count('/'), p)):
                if not storage.exist_dir('/' + name):
                    ops.mkpath('/' + name)
            for row in manifest['files']:
                name = row['path']
                local = stage / name
                ops.mkpath(posixpath.dirname('/' + name))
                print('Restoring /' + name, flush=True)
                storage.send_file(str(local), '/' + name)
                # Remote MD5 readback is possible but expensive; verify exact bytes.
                if receive_to_disk(storage, '/' + name, None, row['size']) != row['sha256']:
                    raise RuntimeError('Read-back verification FAILED on /' + name)
        print('Restore finished: file contents verified. Reboot the Flipper to reload settings.')
        print('Reinstall the original firmware separately if its version differs.')


def main(argv=None):
    parser = argparse.ArgumentParser(description='Snapshot Flipper /int and /ext using USB CDC')
    parser.add_argument('command', choices=['backup', 'restore'])
    parser.add_argument('--port', '-p', default='auto')
    parser.add_argument('--output', help='Backup destination ZIP; existing file is never overwritten')
    parser.add_argument('--input', help='Snapshot ZIP to restore')
    parser.add_argument('--dry-run', action='store_true', help='Validate and preview restoration without modifying data')
    args = parser.parse_args(argv)
    if args.command == 'backup' and (args.input or args.dry_run): parser.error('--input and --dry-run apply only to restore')
    if args.command == 'restore' and args.output: parser.error('--output applies only to backup')
    try:
        if args.command == 'backup':
            default = str(Path.home() / ('Weave-Flipper-' + datetime.now().strftime('%Y%m%d-%H%M%S') + '.zip'))
            choice = args.output
            if not choice and sys.stdin.isatty():
                choice = input(f'Where should the complete data backup be saved? [{default}]: ').strip()
            target = Path(choice or default).expanduser().resolve()
            if target.exists(): raise FileExistsError(f'Destination exists: {target}')
        else:
            choice = args.input
            if not choice and sys.stdin.isatty(): choice = input('Path to Weave Flipper snapshot ZIP: ').strip()
            if not choice: raise ValueError('Specify --input PATH or select a snapshot interactively')
            target = Path(choice).expanduser().resolve()
            if not target.is_file(): raise FileNotFoundError(target)
            # Restore requires explicit confirmation, but integrity checking is safe.
            with zipfile.ZipFile(target) as snapshot:
                validate_archive(snapshot)
        from flipper.storage import FlipperStorage
        with FlipperStorage(get_port(args.port), chunk_size=512, cli_timeout=30) as storage:
            if args.command == 'backup': backup(storage, target)
            else: restore(storage, target, dry_run=args.dry_run)
        return 0
    except (OSError, ValueError, RuntimeError, zipfile.BadZipFile, UnicodeError, KeyError, TypeError) as error:
        print(f'Weave {args.command}: {error}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
