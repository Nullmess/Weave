#!/usr/bin/env python3
"""Best-effort import of compatible open GitHub sound files at build time.

Does not run on the device, never blocks an offline firmware build, and never
modifies an existing user's /ext/.weave/ringtones directory.
"""
from __future__ import annotations

import io
import re
import time
import urllib.error
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
STOCK = ROOT / 'assets' / 'weave_resources' / '.weave' / 'stock-ringtones'
MARKER = STOCK / '.github-import-attempted-v1'
TRACK = STOCK / '.github-imported-files-v1'
FAILED_ATTEMPT = ROOT / 'build' / 'cache' / 'ringtones-attempt.timestamp'
SOURCES = (
    ('Tonsil', 'Tonsil/flipper-music-files', 26),
    ('FlipperRTTTL', 'neverfa11ing/FlipperMusicRTTTL', 26),
)
MAX_ARCHIVE_SIZE = 12 * 1024 * 1024
MAX_FILE_SIZE = 16 * 1024
FORMATS = ('.tone', '.fmf', '.txt', '.rtttl')


def download_zip(repo: str) -> zipfile.ZipFile | None:
    for branch in ('master', 'main'):
        url = f'https://codeload.github.com/{repo}/zip/refs/heads/{branch}'
        try:
            request = urllib.request.Request(url, headers={'User-Agent': 'Weave-Ringtone-Build/1.0'})
            with urllib.request.urlopen(request, timeout=6) as response:
                archive = response.read(MAX_ARCHIVE_SIZE + 1)
            if len(archive) > MAX_ARCHIVE_SIZE:
                return None
            return zipfile.ZipFile(io.BytesIO(archive))
        except (OSError, urllib.error.URLError, zipfile.BadZipFile):
            continue
    return None


def safe_name(prefix: str, original: str) -> str | None:
    original = Path(original).name
    ext = Path(original).suffix.lower()
    if ext not in FORMATS:
        return None
    stem = re.sub(r'[^A-Za-z0-9_-]', '_', Path(original).stem)
    stem = stem[:37-len(prefix)-len(ext)]
    return f'{prefix}{stem}{ext}' if stem else None


def import_files(repo_name: str, repo: str, cap: int) -> list[str]:
    zf = download_zip(repo)
    if zf is None:
        print(f'Weave ringtones: {repo} unavailable; bundled tones remain available')
        return []
    count = 0
    imported: list[str] = []
    with zf:
        for member in sorted(zf.infolist(), key=lambda i: i.filename.lower()):
            if count >= cap:
                break
            name = safe_name('F_' if repo_name == 'Tonsil' else 'R_', member.filename)
            if not name or member.is_dir() or member.file_size > MAX_FILE_SIZE:
                continue
            # Never import compressed nested archives, executable files, or binary payloads.
            with zf.open(member) as handle:
                data = handle.read(MAX_FILE_SIZE + 1)
            if len(data) > MAX_FILE_SIZE or b'\0' in data:
                continue
            try:
                text = data.decode('utf-8-sig')
            except UnicodeDecodeError:
                continue
            if not text.strip():
                continue
            if name.endswith('.fmf') and 'Flipper Music Format' not in text:
                continue
            if name.endswith(('.txt','.rtttl')) and text.count(':') < 2:
                continue
            if name.endswith('.tone') and not re.search(r'^\s*\d+\s+\d+',text,re.M):
                continue
            dest = STOCK / name
            if dest.exists():
                continue
            # Record a future cleanup target *before* writing it, so clean
            # also works after an interrupted download/build.
            with TRACK.open('a', encoding='utf8') as log:
                log.write(f'file={name}\n')
            dest.write_text(text, encoding='utf8')
            imported.append(name)
            count += 1
    return imported


def main():
    STOCK.mkdir(parents=True, exist_ok=True)
    if MARKER.exists():
        return 0
    # Offline builds should not stall on unreachable Github at every rebuild.
    if FAILED_ATTEMPT.exists() and time.time() - FAILED_ATTEMPT.stat().st_mtime < 86400:
        return 0
    FAILED_ATTEMPT.parent.mkdir(parents=True, exist_ok=True)
    FAILED_ATTEMPT.touch()
    imported: list[str] = []
    for name, repo, cap in SOURCES:
        imported.extend(import_files(name, repo, cap))
    # Record exact generated names so clean does not touch bundled/user files.
    if imported:
        MARKER.write_text(
            'Weave ringtone imports (generated files)\n' +
            ''.join(f'file={name}\n' for name in imported), encoding='utf8')
    print(f'Weave ringtones: {len(imported)} new files from GitHub, bundled originals available')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
