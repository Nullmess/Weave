#!/usr/bin/env python3
"""Download pin-checked build tools into the user cache without changing system software.

An existing Node.js >=22 or Gradle 8 is always preferred. Distributions are
verified against the vendor's official SHA256 listing before extraction.
"""
from __future__ import annotations
import hashlib
import http.client
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import time
import tarfile
import urllib.request
import urllib.error
import zipfile

ROOT=Path(__file__).resolve().parents[2]
CACHE_BASE = (Path(os.environ.get('LOCALAPPDATA', str(Path.home() / 'AppData' / 'Local'))) / 'Weave' / 'cache') \
    if os.name == 'nt' else (Path(os.environ.get('XDG_CACHE_HOME', str(Path.home() / '.cache'))) / 'weave')
TOOLS=Path(os.environ.get('WEAVE_CACHE_HOME', str(CACHE_BASE))).expanduser().resolve()/'tools' 
GRADLE_VERSION='8.10.2'
NODE_VERSION='22.16.0'

class ToolUnavailable(RuntimeError):pass

def _request(url: str, extra_headers: dict[str, str] | None = None) -> urllib.request.Request:
    # Several binary/CDN endpoints reject Python's default urllib user agent.
    headers = {
        'User-Agent': 'Weave-Build/' + (ROOT.parent / 'VERSION').read_text().strip(),
        'Accept': '*/*', 'Accept-Encoding': 'identity',
    }
    headers.update(extra_headers or {})
    return urllib.request.Request(url, headers=headers)


def _retry_wait(attempt: int, url: str, error: Exception) -> None:
    delay = min(2 ** attempt, 8)
    print(f'Weave: download interrupted ({error}); retrying {url} in {delay}s '
          f'(attempt {attempt + 2}/5)', flush=True)
    time.sleep(delay)


def _download_with_curl(url: str, target: Path, tmp: Path,
                        expected_sha256: str | None) -> bool:
    """Prefer the platform's curl when available for large resumable downloads.

    curl handles transient TCP/TLS failures independently of Python's urllib.
    A partial archive is retained in build/tools, and the vendor SHA256 is
    always checked before the archive can be installed. If curl is missing or
    fails, the urllib downloader takes over using the same partial file.
    """
    curl = shutil.which('curl')
    if not curl or not url.startswith('https://'):
        return False
    args = [curl, '--fail', '--location', '--silent', '--show-error',
            '--retry', '6', '--retry-all-errors', '--retry-delay', '2',
            '--connect-timeout', '30', '--speed-limit', '1024',
            '--speed-time', '90', '--continue-at', '-',
            '--output', str(tmp), url]
    print('Weave: downloading with resumable curl (network errors are retried)',
          flush=True)
    for attempt in range(2):
        try:
            result = subprocess.run(args, check=False)
        except OSError as exc:
            print(f'Weave: curl unavailable ({exc}); trying Python downloader',
                  flush=True)
            return False
        if result.returncode == 0:
            if expected_sha256 is not None and _sha256(tmp) != expected_sha256.lower():
                tmp.unlink(missing_ok=True)
                print('Weave: curl archive failed SHA256 verification; retrying safely',
                      flush=True)
                return False
            tmp.replace(target)
            return True
        if result.returncode == 33 and tmp.exists() and attempt == 0:
            # Some CDNs refuse HTTP Range requests. Restart the transfer
            # instead of mistakenly appending a full archive to a partial one.
            print('Weave: server rejected resume; retrying from byte 0',
                  flush=True)
            tmp.unlink(missing_ok=True)
            continue
        print(f'Weave: curl exited {result.returncode}; using Python downloader '
              'with the saved partial download', flush=True)
        return False
    return False


def _download(url: str, target: Path, *, expected_sha256: str | None = None) -> None:
    """Download with bounded retries and HTTP Range resume, then atomically publish.

    A failed transfer leaves a .part inside build/tools for the next invocation.
    Downloads with a known digest are checked BEFORE being marked complete.
    A proxy/CDN ignoring Range triggers a safe full restart instead of appending
    duplicate bytes. Authentication and 404 failures are never retried.
    """
    target.parent.mkdir(parents=True, exist_ok=True)
    tmp = target.with_name(target.name + '.part')
    if _download_with_curl(url, target, tmp, expected_sha256):
        return
    for attempt in range(5):
        offset = tmp.stat().st_size if tmp.exists() else 0
        headers = {'Range': f'bytes={offset}-'} if offset else {}
        try:
            with urllib.request.urlopen(_request(url, headers), timeout=90) as src:
                status = src.getcode()
                if status == 206:
                    match = re.match(r'bytes (\d+)-(\d+)/(?:\d+|\*)$',
                                     src.headers.get('Content-Range', ''))
                    if not match or int(match.group(1)) != offset:
                        tmp.unlink(missing_ok=True)
                        raise OSError('Invalid HTTP resume offset; restarting download')
                    append = bool(offset)
                elif status == 200:
                    append = False  # Server did not honor Range; start over.
                else:
                    raise OSError(f'Unexpected HTTP status {status}')
                length = src.headers.get('Content-Length')
                bytes_received = 0
                with tmp.open('ab' if append else 'wb') as out:
                    while True:
                        chunk = src.read(1024 * 1024)
                        if not chunk:
                            break
                        out.write(chunk)
                        bytes_received += len(chunk)
                if length is not None and bytes_received != int(length):
                    raise OSError(f'Incomplete download ({bytes_received}/{length} bytes)')
            if expected_sha256 is not None and _sha256(tmp) != expected_sha256.lower():
                tmp.unlink(missing_ok=True)
                raise OSError('SHA256 mismatch; restarting download')
            tmp.replace(target)
            return
        except urllib.error.HTTPError as exc:
            if exc.code == 416:  # Stale .part: download the complete archive.
                tmp.unlink(missing_ok=True)
            elif exc.code not in (408, 429, 500, 502, 503, 504):
                raise
            error = exc
        except (OSError, TimeoutError, urllib.error.URLError,
                http.client.IncompleteRead) as exc:
            error = exc
        if attempt == 4:
            raise ToolUnavailable(f'Download failed after 5 attempts: {url}: {error}') from error
        _retry_wait(attempt, url, error)


def _fetch_text(url: str, *, limit: int = 4096) -> str:
    for attempt in range(5):
        try:
            with urllib.request.urlopen(_request(url), timeout=45) as response:
                return response.read(limit).decode('utf-8')
        except urllib.error.HTTPError as exc:
            if exc.code not in (408, 429, 500, 502, 503, 504):
                raise
            error = exc
        except (OSError, TimeoutError, urllib.error.URLError,
                http.client.IncompleteRead) as exc:
            error = exc
        if attempt == 4:
            raise ToolUnavailable(f'Cannot fetch {url} after 5 attempts: {error}') from error
        _retry_wait(attempt, url, error)
    raise AssertionError('unreachable')

def _sha256(file:Path)->str:
    h=hashlib.sha256()
    with file.open('rb') as inp:
        for chunk in iter(lambda:inp.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()

def _install_zip(archive:Path,target:Path,prefix:str)->None:
    target.mkdir(parents=True,exist_ok=True)
    with zipfile.ZipFile(archive) as zf:
        for item in zf.infolist():
            if not item.filename.startswith(prefix):continue
            relative=Path(item.filename).relative_to(prefix)
            if not relative.parts or any(part=='..' for part in relative.parts):continue
            dest=target/relative
            if item.is_dir():dest.mkdir(parents=True,exist_ok=True);continue
            dest.parent.mkdir(parents=True,exist_ok=True)
            with zf.open(item) as source,dest.open('wb') as output:shutil.copyfileobj(source,output)

def _node_platform()->tuple[str,str]:
    os_name={'Windows':'win','Linux':'linux','Darwin':'darwin'}.get(platform.system())
    machine={'x86_64':'x64','AMD64':'x64','aarch64':'arm64','arm64':'arm64'}.get(platform.machine())
    if not os_name or not machine:raise ToolUnavailable('Unsupported Node platform')
    return os_name,machine

def node_tools()->tuple[str,str]:
    existing=shutil.which('node');npm=shutil.which('npm')
    if existing and npm:
        try:
            v=subprocess.run([existing,'--version'],capture_output=True,text=True,timeout=5,check=True).stdout.strip()
            if int(v.lstrip('v').split('.')[0])>=22:return existing,npm
        except (ValueError,OSError,subprocess.SubprocessError):pass
    os_name,machine=_node_platform()
    ext='zip' if os_name=='win' else 'tar.xz'
    basename=f'node-v{NODE_VERSION}-{os_name}-{machine}'
    filename=f'{basename}.{ext}'
    base=f'https://nodejs.org/dist/v{NODE_VERSION}'
    dest=TOOLS/'node'
    binary=dest/('node.exe' if os_name=='win' else 'bin/node')
    npm_bin=dest/('npm.cmd' if os_name=='win' else 'bin/npm')
    if os_name!='win' and binary.is_file() and not npm_bin.is_file() and (dest/'lib/node_modules/npm/bin/npm-cli.js').is_file():
        npm_bin.parent.mkdir(parents=True,exist_ok=True)
        npm_bin.write_text('#!/bin/sh\nexec "' + str(binary) + '" "' + str(dest/'lib/node_modules/npm/bin/npm-cli.js') + '" "$@"\n')
        npm_bin.chmod(0o755)
    if binary.is_file() and npm_bin.exists():return str(binary),str(npm_bin)
    try:
        checks=(TOOLS/'node-SHASUMS256.txt')
        if not checks.exists():_download(f'{base}/SHASUMS256.txt',checks)
        match=re.search(rf'(?m)^([0-9a-f]{{64}})\s+{re.escape(filename)}$',checks.read_text())
        if not match:raise ToolUnavailable('Node checksum not in official manifest')
        archive=TOOLS/filename
        if archive.exists() and _sha256(archive)!=match.group(1):archive.unlink()
        if not archive.exists():_download(f'{base}/{filename}',archive, expected_sha256=match.group(1))
        if _sha256(archive)!=match.group(1):archive.unlink();raise ToolUnavailable('Node checksum mismatch')
        if os_name=='win': _install_zip(archive,dest,basename+'/')
        else:
            dest.mkdir(parents=True,exist_ok=True)
            with tarfile.open(archive,'r:xz') as tf:
                for m in tf.getmembers():
                    if m.name.startswith(basename+'/') and m.isfile():
                        relative=Path(m.name).relative_to(basename)
                        if '..' in relative.parts:continue
                        file=dest/relative;file.parent.mkdir(parents=True,exist_ok=True)
                        source=tf.extractfile(m)
                        if source:
                            with source,file.open('wb') as out:shutil.copyfileobj(source,out)
                            file.chmod(m.mode & 0o755)
        if os_name!='win' and binary.is_file() and not npm_bin.is_file():
            script=dest/'lib/node_modules/npm/bin/npm-cli.js'
            if script.is_file():
                npm_bin.write_text('#!/bin/sh\nexec "' + str(binary) + '" "' + str(script) + '" "$@"\n')
                npm_bin.chmod(0o755)
        if not binary.is_file() or not npm_bin.exists():raise ToolUnavailable('Node installation incomplete')
        return str(binary),str(npm_bin)
    except (OSError,TimeoutError,urllib.error.URLError,tarfile.TarError,zipfile.BadZipFile) as exc:
        raise ToolUnavailable(f'Cannot install Node locally: {exc}') from exc

def gradle_command()->list[str]:
    command=shutil.which('gradle')
    if command:
        try:
            result=subprocess.run([command,'--version'],capture_output=True,text=True,timeout=12)
            if result.returncode==0 and re.search(r'Gradle\s+8\.',result.stdout):return [command]
        except (OSError,subprocess.SubprocessError):pass
    dest=TOOLS/'gradle'
    binary=dest/'bin'/('gradle.bat' if os.name=='nt' else 'gradle')
    if binary.is_file():return [str(binary)]
    url=f'https://services.gradle.org/distributions/gradle-{GRADLE_VERSION}-bin.zip'
    zipfile_path=TOOLS/f'gradle-{GRADLE_VERSION}-bin.zip'
    checksum_file=TOOLS/f'gradle-{GRADLE_VERSION}-bin.zip.sha256'
    try:
        if not checksum_file.exists():_download(url+'.sha256',checksum_file)
        checksum=checksum_file.read_text().split()[0].strip()
        if not re.fullmatch('[0-9a-f]{64}',checksum):raise ToolUnavailable('Bad Gradle checksum')
        if zipfile_path.exists() and _sha256(zipfile_path)!=checksum:zipfile_path.unlink()
        if not zipfile_path.exists():_download(url,zipfile_path, expected_sha256=checksum)
        if _sha256(zipfile_path)!=checksum:zipfile_path.unlink();raise ToolUnavailable('Gradle checksum mismatch')
        _install_zip(zipfile_path,dest,f'gradle-{GRADLE_VERSION}/')
        if os.name!='nt' and binary.exists():binary.chmod(0o755)
        if not binary.exists():raise ToolUnavailable('Gradle executable missing after installation')
        return [str(binary)]
    except (OSError,TimeoutError,urllib.error.URLError,zipfile.BadZipFile) as exc:
        raise ToolUnavailable(f'Cannot install Gradle locally: {exc}') from exc



# Gradle/AGP require Java 17+, and an old system `java` may shadow JAVA_HOME.
# Use a working JDK explicitly for both sdkmanager and Gradle.
def _java_major(java: Path) -> int | None:
    try:
        process = subprocess.run([str(java), '-version'], capture_output=True, text=True,
                                 timeout=10, check=False)
        output = process.stderr + process.stdout
        match = re.search(r'(?:openjdk|java) version "(\d+)(?:\.(\d+))?', output)
        if not match:
            return None
        first = int(match.group(1))
        return int(match.group(2)) if first == 1 and match.group(2) else first
    except (OSError, subprocess.SubprocessError):
        return None


def _java_executable(home: Path) -> Path:
    return home / 'bin' / ('java.exe' if os.name == 'nt' else 'java')


def _javac_executable(home: Path) -> Path:
    return home / 'bin' / ('javac.exe' if os.name == 'nt' else 'javac')


def _javac_major(javac: Path) -> int | None:
    """Check that the Java installation includes a working compiler, not just a JRE."""
    try:
        process = subprocess.run([str(javac), '-version'], capture_output=True,
                                 text=True, timeout=10, check=False)
        if process.returncode:
            return None
        match = re.search(r'\bjavac\s+(\d+)(?:\.(\d+))?', process.stdout + process.stderr)
        if not match:
            return None
        major = int(match.group(1))
        return int(match.group(2)) if major == 1 and match.group(2) else major
    except (OSError, subprocess.SubprocessError):
        return None


def _full_jdk_major(home: Path) -> int | None:
    """Only accept installations that have matching java and javac 17-23."""
    java = _java_major(_java_executable(home))
    if java is None or java not in range(17, 24):
        return None
    return java if _javac_major(_javac_executable(home)) == java else None


def _jdk_candidates() -> list[Path]:
    candidates: list[Path] = []
    for envname in ('WEAVE_JAVA_HOME', 'JAVA_HOME'):
        if os.environ.get(envname):
            candidates.append(Path(os.environ[envname]))
    candidates.append(TOOLS / 'jdk-17')
    executable = shutil.which('java')
    if executable:
        candidates.append(Path(executable).resolve().parent.parent)
    if platform.system() == 'Linux':
        candidates.extend(Path('/usr/lib/jvm').glob('*'))
        candidates.extend(Path('/opt').glob('jdk*'))
    elif platform.system() == 'Darwin':
        candidates.extend(p / 'Contents' / 'Home' for p in
                          Path('/Library/Java/JavaVirtualMachines').glob('*.jdk'))
    elif platform.system() == 'Windows':
        base = Path(os.environ.get('ProgramFiles', r'C:\Program Files'))
        for dirname in ('Eclipse Adoptium', 'Java', 'Microsoft', 'Amazon Corretto'):
            candidates.extend((base / dirname).glob('*'))
    return candidates


def _temurin_platform() -> tuple[str, str]:
    system = {'Linux': 'linux', 'Darwin': 'mac', 'Windows': 'windows'}.get(platform.system())
    arch = {'x86_64': 'x64', 'AMD64': 'x64', 'aarch64': 'aarch64',
            'arm64': 'aarch64'}.get(platform.machine())
    if not system or not arch:
        raise ToolUnavailable('No Temurin JDK 17 installer for this platform')
    return system, arch


def _jdk_archive_extract(archive: Path, folder: Path) -> Path:
    """Extract vendor archive, rejecting absolute and escaping archive entries."""
    import tempfile
    folder.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='.jdk-stage-', dir=TOOLS) as staging:
        temp = Path(staging)
        if zipfile.is_zipfile(archive):
            with zipfile.ZipFile(archive) as zf:
                for entry in zf.infolist():
                    rel = Path(entry.filename)
                    if rel.is_absolute() or '..' in rel.parts:
                        raise ToolUnavailable('Unsafe JDK zip archive')
                    dest = temp / rel
                    if entry.is_dir():
                        dest.mkdir(parents=True, exist_ok=True)
                    else:
                        dest.parent.mkdir(parents=True, exist_ok=True)
                        with zf.open(entry) as source, dest.open('wb') as target:
                            shutil.copyfileobj(source, target)
        else:
            with tarfile.open(archive, 'r:gz') as tf:
                for entry in tf.getmembers():
                    rel = Path(entry.name)
                    if rel.is_absolute() or '..' in rel.parts or not (
                        entry.isdir() or entry.isfile() or entry.issym() or entry.islnk()
                    ):
                        raise ToolUnavailable('Unsafe JDK tar archive')
                    if entry.issym():
                        target = (temp / rel.parent / entry.linkname).resolve(strict=False)
                        if not target.is_relative_to(temp):
                            raise ToolUnavailable('Unsafe JDK symlink')
                    if entry.islnk():
                        target = (temp / entry.linkname).resolve(strict=False)
                        if not target.is_relative_to(temp):
                            raise ToolUnavailable('Unsafe JDK hardlink')
                try:
                    tf.extractall(temp, filter='data')
                except TypeError as exc:
                    # Python <3.12: all paths and links were vetted above.
                    if 'filter' not in str(exc):
                        raise
                    tf.extractall(temp)
        possible = [p.parent.parent for p in temp.rglob('bin/java')]
        if os.name == 'nt':
            possible = [p.parent.parent for p in temp.rglob('bin/java.exe')]
        homes = [p for p in possible if _full_jdk_major(p) == 17]
        if not homes:
            raise ToolUnavailable('Temurin archive does not contain a complete Java 17 JDK (java + javac)')
        if folder.exists():
            shutil.rmtree(folder)
        shutil.copytree(homes[0], folder, symlinks=True)
        return folder


def _verified_jdk_download(url: str, digest: str, archive: Path, destination: Path) -> Path:
    """Install only if the archive matches the vendor-published SHA256."""
    digest = digest.lower()
    if not re.fullmatch(r'[0-9a-f]{64}', digest):
        raise ToolUnavailable('Invalid JDK SHA256 checksum')
    if archive.exists() and _sha256(archive) != digest:
        archive.unlink()  # A cached older version must not be reused.
    if not archive.exists():
        _download(url, archive, expected_sha256=digest)
    if _sha256(archive) != digest:
        archive.unlink(missing_ok=True)
        raise ToolUnavailable('Downloaded JDK SHA256 mismatch')
    return _jdk_archive_extract(archive, destination)


def _install_temurin17(system: str, arch: str) -> Path:
    """Official Eclipse Adoptium JSON metadata, with a checksum per asset."""
    import json
    url = (f'https://api.adoptium.net/v3/assets/latest/17/hotspot?'
           f'architecture={arch}&image_type=jdk&os={system}&vendor=eclipse')
    releases = json.loads(_fetch_text(url, limit=1024 * 1024))
    package = releases[0]['binary']['package']
    link, digest = package['link'], package['checksum']
    if not link.startswith('https://'):
        raise ToolUnavailable('Invalid Temurin download URL')
    archive = TOOLS / ('temurin-jdk17.zip' if system == 'windows' else 'temurin-jdk17.tar.gz')
    return _verified_jdk_download(link, digest, archive, TOOLS / 'jdk-17')


def _corretto17_filename(system: str, arch: str) -> str:
    """Official Amazon Corretto 17 portable archives (not system installers)."""
    if system == 'windows':
        if arch != 'x64':
            raise ToolUnavailable('No portable Corretto 17 JDK for Windows ARM')
        return 'amazon-corretto-17-x64-windows-jdk.zip'
    if system == 'linux':
        return f'amazon-corretto-17-{arch}-linux-jdk.tar.gz'
    if system == 'mac':
        return f'amazon-corretto-17-{arch}-macos-jdk.tar.gz'
    raise ToolUnavailable('No portable Corretto 17 JDK for this OS')


def _install_corretto17(system: str, arch: str) -> Path:
    """Fallback provider when Adoptium or its GitHub CDN returns HTTP 403.

    Amazon publishes a separate SHA256 file for every latest JDK archive.
    The two URLs are official and version-matched by basename.
    """
    filename = _corretto17_filename(system, arch)
    base = 'https://corretto.aws/downloads/'
    checksum = _fetch_text(base + 'latest_sha256/' + filename).strip()
    match = re.search(r'(?<![0-9a-fA-F])[0-9a-fA-F]{64}(?![0-9a-fA-F])', checksum)
    if not match:
        raise ToolUnavailable('Invalid official Corretto 17 SHA256 response')
    archive = TOOLS / ('corretto-' + filename)
    return _verified_jdk_download(base + 'latest/' + filename, match.group(), archive, TOOLS / 'jdk-17')


def jdk_home() -> Path:
    """Reuse a complete JDK, else download verified JDK17 without system changes."""
    seen: set[Path] = set()
    for candidate in _jdk_candidates():
        candidate = candidate.expanduser()
        if candidate in seen:
            continue
        seen.add(candidate)
        if _full_jdk_major(candidate) is not None:
            return candidate
        major = _java_major(_java_executable(candidate))
        if major in range(17, 24):
            print(f'Weave: ignoring Java {major} runtime without matching javac: {candidate}')
    if os.environ.get('WEAVE_NO_INSTALL') == '1':
        raise ToolUnavailable('Complete JDK 17-23 with javac required (WEAVE_NO_INSTALL=1)')
    # Temurin/Corretto JDK 17 are reusable, SHA256-verified local build tools.
    # No system package changes or administrator privileges are required.
    print('Weave: a complete JDK 17-23 (java + javac) was not found. '
          'Automatically preparing a verified JDK 17 in build/tools/ ...', flush=True)
    system, arch = _temurin_platform()
    failures = []
    for label, installer in (('Eclipse Temurin', _install_temurin17),
                             ('Amazon Corretto', _install_corretto17)):
        print(f'Weave: downloading {label} 17 ({system}/{arch}) ...')
        try:
            home = installer(system, arch)
            if _full_jdk_major(home) != 17:
                raise ToolUnavailable(f'{label} did not install a working JDK 17')
            print(f'Weave: {label} JDK 17 ready in {home}')
            return home
        except (ToolUnavailable, OSError, TimeoutError, ValueError, IndexError, KeyError,
                urllib.error.URLError, tarfile.TarError, zipfile.BadZipFile) as exc:
            failures.append(f'{label}: {exc}')
            print(f'Weave: {label} unavailable ({exc}); trying another provider.' if label == 'Eclipse Temurin'
                  else f'Weave: {label} unavailable ({exc})')
    raise ToolUnavailable('Cannot download JDK 17. ' + '; '.join(failures) +
                          '. Install a complete JDK 17/21 locally or set WEAVE_JAVA_HOME.')


def jdk_environment() -> dict[str, str]:
    """Force SDK Manager and Gradle to use the same compatible Java."""
    home = jdk_home()
    env = os.environ.copy()
    env['JAVA_HOME'] = str(home)
    env['PATH'] = str(home / 'bin') + os.pathsep + env.get('PATH', '')
    return env

# Fixed checksums published by Google on https://developer.android.com/studio
# (command-line tools release 15859902). Never execute an unverified archive.
ANDROID_TOOLS = {
    ('Linux','x86_64'):('commandlinetools-linux-15859902_latest.zip','4e4c464f145a7512b57d088ac6c278c03c9eea610886b35a5e0804e74eedf583'),
    ('Linux','aarch64'):None,  # Google's official command-line package has no Linux ARM host variant.
    ('Windows','AMD64'):('commandlinetools-win-15859902_latest.zip','90ae805d20434428bffcb699c290860f19bb5f66a67e6b330067e3de801fb04a'),
    ('Windows','x86_64'):('commandlinetools-win-15859902_latest.zip','90ae805d20434428bffcb699c290860f19bb5f66a67e6b330067e3de801fb04a'),
    ('Darwin','x86_64'):('commandlinetools-mac_x86_64-15859902_latest.zip','c5a6378ab5cf7e0d5701921405115befff13e9ff7417fb588389338f8bd050f3'),
    ('Darwin','arm64'):('commandlinetools-mac_arm64-15859902_latest.zip','835b62a26162b229b441d1f6d4680383815a270809eb33522c0d480fa5002c4e'),
}

def _weave_sdk_cache_dir() -> Path:
    """SDK location retained across project ZIP replacements and fbt.py clean."""
    if os.name == 'nt':
        sdk_base = Path(os.environ.get('LOCALAPPDATA') or (Path.home() / 'AppData' / 'Local'))
    elif sys.platform == 'darwin':
        sdk_base = Path.home() / 'Library' / 'Application Support'
    else:
        sdk_base = Path(os.environ.get('XDG_DATA_HOME') or (Path.home() / '.local' / 'share'))
    return sdk_base / 'Weave' / 'android-sdk'


def _sdk_candidates() -> list[Path]:
    """Find Android SDKs; keep Weave's SDK outside disposable build/ folders."""
    candidates: list[Path] = []
    for name in ('ANDROID_HOME', 'ANDROID_SDK_ROOT'):
        if os.environ.get(name):
            candidates.append(Path(os.environ[name]).expanduser())
    # Projects are often re-extracted from a ZIP, and fbt.py clean deletes build/.
    # Keep Google's downloaded tools and accepted license records between builds.
    candidates.append(_weave_sdk_cache_dir())
    # Backward compatibility: a working SDK inside a previous build/ is reused.
    candidates.append(TOOLS / 'android-sdk')
    system = platform.system()
    home = Path.home()
    if system == 'Linux':
        candidates.extend((home / 'Android' / 'Sdk', home / '.local/share/android-sdk',
                           Path('/opt/android-sdk'), Path('/usr/lib/android-sdk')))
    elif system == 'Darwin':
        candidates.append(home / 'Library' / 'Android' / 'sdk')
    elif system == 'Windows':
        if os.environ.get('LOCALAPPDATA'):
            candidates.append(Path(os.environ['LOCALAPPDATA']) / 'Android' / 'Sdk')
        candidates.append(home / 'AppData' / 'Local' / 'Android' / 'Sdk')
    return list(dict.fromkeys(candidates))


def _sdk_ready(sdk: Path) -> bool:
    aapt = 'aapt2.exe' if os.name == 'nt' else 'aapt2'
    return ((sdk / 'platforms' / 'android-35' / 'android.jar').is_file()
            and (sdk / 'build-tools' / '35.0.0' / aapt).is_file())


def _sdkmanager_path(sdk: Path) -> Path | None:
    name = 'sdkmanager.bat' if os.name == 'nt' else 'sdkmanager'
    tools = sdk / 'cmdline-tools'
    choices = [tools / 'latest' / 'bin' / name]
    if tools.is_dir():
        choices.extend(p / 'bin' / name for p in sorted(tools.iterdir(), reverse=True)
                       if p.is_dir() and p.name != 'latest')
    choices.append(sdk / 'tools' / 'bin' / name)
    return next((p for p in choices if p.is_file()), None)


def _android_host_tools() -> tuple[str, str]:
    system, machine = platform.system(), platform.machine()
    # Normalize naming reported by Python and the OS (e.g. Windows AMD64).
    arch = machine.lower()
    if system == 'Windows' and arch in ('amd64', 'x86_64', 'x64'):
        key = ('Windows', 'AMD64')
    elif system == 'Linux' and arch in ('amd64', 'x86_64', 'x64'):
        key = ('Linux', 'x86_64')
    elif system == 'Darwin' and arch in ('arm64', 'aarch64'):
        key = ('Darwin', 'arm64')
    elif system == 'Darwin' and arch in ('x86_64', 'amd64', 'x64'):
        key = ('Darwin', 'x86_64')
    else:
        key = (system, machine)
    item = ANDROID_TOOLS.get(key)
    if item is None:
        raise ToolUnavailable(f'Google Android SDK CLI tools are not available for '
                              f'{system}/{machine}; use an installed, compatible SDK')
    return item


def _android_licenses_present(sdk: Path) -> bool:
    """Only trust license acceptance actually recorded by Google's SDK Manager.

    The android-sdk-license file contains vendor-issued SHA-1 license hashes,
    not a blanket approval flag supplied by Weave.
    """
    license_file = sdk / 'licenses' / 'android-sdk-license'
    try:
        return any(re.fullmatch(r'[0-9a-fA-F]{40}', line.strip())
                   for line in license_file.read_text(encoding='utf-8').splitlines())
    except (OSError, UnicodeError):
        return False


def _android_install_approval_file() -> Path:
    """Keep approval outside build/, because `fbt.py clean` deletes build/."""
    if os.name == 'nt':
        base = Path(os.environ.get('APPDATA') or (Path.home() / 'AppData' / 'Roaming'))
    elif sys.platform == 'darwin':
        base = Path.home() / 'Library' / 'Application Support'
    else:
        base = Path(os.environ.get('XDG_CONFIG_HOME') or (Path.home() / '.config'))
    return base / 'Weave' / 'android-sdk-install-approved'


def _ask_android_install_permission() -> str:
    """Read a deliberate yes/no from the terminal, not a consumed build stdin.

    Firmware/build subprocesses share inherited stdin. Reading /dev/tty on
    POSIX keeps SDK approval independent from pipe/terminal state. Empty reads
    are retried rather than silently interpreted as a refusal after a user
    attempted to type y. No licenses are accepted by this function.
    """
    prompt = 'Install the missing Android SDK tools? Type y or n: '
    if os.name != 'nt':
        try:
            with open('/dev/tty', 'r+', encoding='utf-8') as terminal:
                for attempt in range(3):
                    terminal.write(prompt)
                    terminal.flush()
                    answer = terminal.readline().strip().casefold()
                    if answer:
                        return answer
                    if attempt < 2:
                        terminal.write('Weave: no answer received; please type y or n.\n')
                        terminal.flush()
                return ''
        except OSError:
            pass
    if not sys.stdin.isatty():
        raise ToolUnavailable('Interactive terminal required to approve Android SDK '
                              'installation. Run python3 fbt.py build in a terminal '
                              'or reuse a previously approved SDK.')
    try:
        for attempt in range(3):
            answer = input(prompt).strip().casefold()
            if answer:
                return answer
            if attempt < 2:
                print('Weave: no answer received; please type y or n.', flush=True)
    except (EOFError, KeyboardInterrupt) as exc:
        raise ToolUnavailable('Android SDK setup not confirmed. '
                              'Run the build interactively and type y.') from exc
    return ''


def _approve_android_setup(sdk: Path) -> None:
    """Remember explicit download approval across clean, never auto-accept licenses.

    The SDK-manager licenses are separate and validated from Google's official
    hashed license records. This marker approves only tool downloads.
    """
    marker = TOOLS / '.android-sdk-install-approved'
    persistent_marker = _android_install_approval_file()
    if marker.is_file() or persistent_marker.is_file() or _android_licenses_present(sdk):
        return
    print(f'Weave: Android SDK 35 is incomplete or missing for {platform.system()}/'
          f'{platform.machine()}. It will be installed in {sdk}.', flush=True)
    print('Weave: download requires Google Android SDK terms: '
          'https://developer.android.com/studio', flush=True)
    answer = _ask_android_install_permission()
    if answer not in {'y', 'yes', 'o', 'oui'}:
        raise ToolUnavailable('Android SDK installation not approved. '
                              'Run python3 fbt.py build and type y to install the SDK.')
    content = ('User approved local Android SDK tool downloads. '
               'Google SDK package licenses require separate acceptance.\n')
    marker.parent.mkdir(parents=True, exist_ok=True)
    marker.write_text(content, encoding='utf-8')
    try:
        persistent_marker.parent.mkdir(parents=True, exist_ok=True)
        persistent_marker.write_text(content, encoding='utf-8')
    except OSError as exc:
        # The transient marker still lets the current build continue.
        print(f'Weave: cannot persist installation preference ({exc}); '
              'future clean builds may ask again.', flush=True)


def _review_android_licenses(executable: Path, sdk: Path, env: dict[str, str]) -> None:
    """Let the user review Google's licenses without silently accepting any.

    sdkmanager considers e.g. 'yy' a refusal. Restart its interactive review
    rather than failing the entire multi-platform build after an accidental
    double keystroke. The user still reads/accepts every agreement personally.
    """
    if not sys.stdin.isatty():
        raise ToolUnavailable('Google Android SDK licenses need an interactive terminal. '
                              'Run python3 fbt.py build in a terminal and answer y '
                              '(one letter, then Enter) at each license prompt.')
    for attempt in range(3):
        if attempt:
            print(f'Weave: SDK license review incomplete; starting it again '
                  f'({attempt + 1}/3).', flush=True)
        print('Weave: at each Google SDK Manager prompt, enter exactly y then '
              'press Enter (not yy). Read each license before accepting.', flush=True)
        result = subprocess.run([str(executable), '--sdk_root=' + str(sdk), '--licenses'],
                                env=env, cwd=ROOT, check=False)
        if result.returncode == 0 and _android_licenses_present(sdk):
            print('Weave: Android SDK license records saved successfully.', flush=True)
            return
    raise ToolUnavailable('Android SDK licenses remain unaccepted. Re-run '
                          'python3 fbt.py build, answer y (not yy) to the '
                          'initial review question, then y to each agreement.')


def android_sdk_dir(java_env: dict[str, str] | None = None) -> Path:
    """Find a usable SDK or install the missing packages for the current OS.

    SDK/JDK/Gradle downloads are local, SHA256-verified and can be resumed.
    Google's Android license agreement still requires explicit user approval.
    """
    candidates = _sdk_candidates()
    for sdk in candidates:
        if _sdk_ready(sdk):
            print(f'Weave: reusing Android SDK at {sdk}', flush=True)
            return sdk
    if os.environ.get('WEAVE_NO_INSTALL') == '1':
        raise ToolUnavailable('Android SDK 35/build-tools 35 missing (WEAVE_NO_INSTALL=1)')
    # Reuse an incomplete but writable SDK if the user configured/installed one.
    # Never try to modify a system-owned read-only SDK.
    sdk = next((p for p in candidates if p.is_dir() and os.access(p, os.W_OK)),
               _weave_sdk_cache_dir())
    executable = _sdkmanager_path(sdk)
    if executable is None:
        # Report unsupported OS/architecture before asking consent.
        filename, expected = _android_host_tools()
    _approve_android_setup(sdk)
    sdk.mkdir(parents=True, exist_ok=True)
    if executable is None:
        archive = TOOLS / filename
        try:
            if archive.exists() and _sha256(archive) != expected:
                archive.unlink()  # Repair an incomplete/corrupted previous cache.
            if not archive.is_file():
                print('Weave: downloading Google Android command-line tools '
                      '(interrupted downloads are resumed automatically)', flush=True)
                _download('https://dl.google.com/android/repository/' + filename,
                          archive, expected_sha256=expected)
            tools = sdk / 'cmdline-tools' / 'latest'
            _install_zip(archive, tools, 'cmdline-tools/')
            executable = _sdkmanager_path(sdk)
            if executable is None:
                raise ToolUnavailable('Android sdkmanager missing after extraction')
            if os.name != 'nt':
                executable.chmod(executable.stat().st_mode | 0o111)
        except (OSError, urllib.error.URLError, zipfile.BadZipFile) as exc:
            raise ToolUnavailable(f'Android SDK command-line tools setup failed: {exc}') from exc
    env = java_env.copy() if java_env is not None else jdk_environment()
    env['ANDROID_HOME'] = str(sdk)
    env['ANDROID_SDK_ROOT'] = str(sdk)
    try:
        if not _android_licenses_present(sdk):
            print('Weave: Google SDK Manager will ask for your explicit approval '
                  'of the Android licenses. This is necessary only once per SDK.',
                  flush=True)
            _review_android_licenses(executable, sdk, env)
        else:
            print('Weave: Android SDK licenses already accepted; continuing '
                  'without another prompt.', flush=True)
        packages = ['platform-tools', 'platforms;android-35', 'build-tools;35.0.0']
        for attempt in range(3):
            result = subprocess.run([str(executable), '--sdk_root=' + str(sdk),
                                     *packages], env=env, cwd=ROOT, check=False)
            if result.returncode == 0 and _sdk_ready(sdk):
                print(f'Weave: Android SDK 35 ready at {sdk}', flush=True)
                return sdk
            if attempt < 2:
                print('Weave: Android SDK packages not ready; retrying installation '
                      f'({attempt + 2}/3)', flush=True)
                time.sleep(2 ** attempt)
        raise ToolUnavailable('Android SDK packages failed to install after 3 attempts; '
                              'check network/proxy and SDK Manager output above')
    except OSError as exc:
        raise ToolUnavailable(f'Android SDK setup failed: {exc}') from exc

if __name__=='__main__':
    if len(sys.argv)<2 or sys.argv[1] not in ('node','gradle'):sys.exit('Usage: bootstrap_tools.py node|gradle [args]')
    try:
        if sys.argv[1]=='gradle':sys.exit(subprocess.run(gradle_command()+sys.argv[2:],cwd=ROOT.parent/'apps/android').returncode)
        print(node_tools())
    except ToolUnavailable as error:sys.exit(str(error))
