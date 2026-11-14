#!/usr/bin/env python3
"""Weave companion: native Electron development or launch a built desktop app."""
from __future__ import annotations

import argparse
import os
from pathlib import Path, PurePosixPath
import shutil
import stat
import subprocess
import sys
import zipfile

ROOT = Path(__file__).resolve().parents[2]
BUILD = ROOT / 'build'
DESKTOP = ROOT.parent / 'apps' / 'desktop'


def host():
    if sys.platform == 'win32': return 'windows'
    if sys.platform == 'darwin': return 'macos'
    if sys.platform.startswith('linux'): return 'linux'
    raise RuntimeError(f'Unsupported OS: {sys.platform}')


def node_environment():
    from bootstrap_tools import node_tools
    node, npm = node_tools()
    env = os.environ.copy()
    env['PATH'] = str(Path(node).parent) + os.pathsep + env.get('PATH','')
    env['npm_config_cache'] = str(BUILD / 'tools' / 'npm-cache')
    env['ELECTRON_CACHE'] = str(BUILD / 'tools' / 'electron-cache')
    env['ELECTRON_BUILDER_CACHE'] = str(BUILD / 'tools' / 'electron-builder-cache')
    return npm, env


def dev():
    """Launch Electron development mode, not a browser-only Vite preview.

    Browser previews cannot access Electron serialport and therefore cannot
    control the USB Flipper. The native desktop window is mandatory here.
    """
    npm, env = node_environment()
    modules = DESKTOP / 'node_modules'
    needs_install = any(not (modules / item).exists() for item in (
        '.bin/vite', '.bin/electron', '.bin/tsc', 'serialport/package.json'))
    if needs_install:
        print('Weave dev: installing native desktop dependencies.', flush=True)
        command = [npm, 'ci' if (DESKTOP/'package-lock.json').is_file() else 'install',
                   '--prefer-offline', '--no-audit', '--no-fund']
        result = subprocess.run(command, cwd=DESKTOP, env=env, check=False)
        if result.returncode: return result.returncode
    print('Weave dev: launching native Electron + Vite (USB CDC supported).', flush=True)
    print('On Linux, /dev/ttyACM* must be accessible to your user.', flush=True)
    print('Ctrl+C closes the native development session.', flush=True)
    try:
        return subprocess.run([npm, 'run', 'dev'], cwd=DESKTOP, env=env, check=False).returncode
    except KeyboardInterrupt:
        return 130


def safe_extract(archive: zipfile.ZipFile, destination: Path):
    entries = archive.infolist()
    if not entries or len(entries) > 40_000:
        raise ValueError('Invalid desktop bundle')
    seen = set()
    for item in entries:
        name = item.filename
        p = PurePosixPath(name)
        if not name or p.is_absolute() or any(c in ('..','.') for c in name.split('/')) or '\\' in name:
            raise ValueError(f'Unsafe native app ZIP entry: {name!r}')
        if p.as_posix() in seen: raise ValueError('Duplicate archive entry')
        seen.add(p.as_posix())
        mode = item.external_attr >> 16
        is_link = stat.S_ISLNK(mode)
        if is_link:
            link = archive.read(item).decode('utf-8')
            target = (destination / p.parent / link).resolve(strict=False)
            if not target.is_relative_to(destination.resolve()):
                raise ValueError(f'Unsafe application ZIP symlink: {name}')
        elif mode and not (stat.S_ISREG(mode) or stat.S_ISDIR(mode)):
            raise ValueError(f'Unsupported archive entry: {name}')
    for item in entries:
        p = destination.joinpath(*PurePosixPath(item.filename).parts)
        p.parent.mkdir(parents=True, exist_ok=True)
        if item.is_dir(): p.mkdir(parents=True, exist_ok=True); continue
        mode = item.external_attr >> 16
        if stat.S_ISLNK(mode):
            p.symlink_to(archive.read(item).decode('utf-8'))
        else:
            with archive.open(item) as src, p.open('wb') as dst:
                shutil.copyfileobj(src, dst, length=1024*1024)
            if mode & 0o111:
                p.chmod(p.stat().st_mode | 0o111)


def app():
    system = host()
    version = (ROOT.parent / 'VERSION').read_text(encoding='utf-8').strip()
    name = f'Weave-{version}-{system}.zip'
    bundle = BUILD / 'releases' / name
    if not bundle.is_file():
        print(f'Weave app: missing already-built {name}.',file=sys.stderr)
        print('Build on your native OS first, or download the matching artifact from GitHub Actions.',file=sys.stderr)
        print(f'Place the ZIP in: {BUILD / "releases"}',file=sys.stderr)
        return 1
    output = BUILD / 'runtime' / f'{system}-{version}'
    installed = output / '.weave-package-ready'
    try:
        if not installed.is_file() or installed.read_text(encoding='ascii').strip() != str(bundle.stat().st_mtime_ns):
            if output.exists(): shutil.rmtree(output)
            output.mkdir(parents=True)
            with zipfile.ZipFile(bundle) as archive:
                if archive.testzip(): raise ValueError('Corrupt app archive')
                safe_extract(archive, output)
            installed.write_text(str(bundle.stat().st_mtime_ns),encoding='ascii')
        if system == 'macos':
            apps = list(output.rglob('Weave.app'))
            if len(apps) != 1: raise FileNotFoundError('Weave.app missing from macOS archive')
            command = ['open', str(apps[0])]
        else:
            exe = 'Weave.exe' if system == 'windows' else 'Weave'
            files = [p for p in output.rglob(exe) if p.is_file() and not p.is_symlink() and p.name == exe]
            if len(files) != 1: raise FileNotFoundError(f'{exe} missing from {name}')
            executable = files[0]
            if system == 'linux': executable.chmod(executable.stat().st_mode | stat.S_IXUSR)
            command = [str(executable)]
        print(f'Weave app: launching {system} native companion from {bundle}',flush=True)
        subprocess.Popen(command, cwd=output, start_new_session=(os.name != 'nt'))
        return 0
    except (OSError, ValueError, zipfile.BadZipFile) as error:
        print(f'Weave app: {error}',file=sys.stderr)
        return 1


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('mode',choices=('dev','app'))
    args = parser.parse_args(argv)
    try:
        return dev() if args.mode == 'dev' else app()
    except (OSError, RuntimeError) as error:
        print(f'Weave {args.mode}: {error}',file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
