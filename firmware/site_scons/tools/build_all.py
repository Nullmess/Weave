#!/usr/bin/env python3
"""Local Weave release build. No GitHub API, credentials, or fake cross-compiles.

The firmware is already built by fbt.py. Electron/Vite desktop is bundled
for supported local OS targets; Android uses a bootstrapped Gradle with Java/SDK. Missing
prerequisites are reported and a full six-asset build returns failure when incomplete.
GitHub Actions is a separate, optional manual multi-OS build.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
VERSION = (ROOT.parent / "VERSION").read_text(encoding="utf-8").strip()
BUILD = ROOT / "build"
OUT = BUILD / "releases"
CACHE_BASE = (Path(os.environ.get("LOCALAPPDATA", str(Path.home() / "AppData" / "Local"))) / "Weave" / "cache") \
    if os.name == "nt" else (Path(os.environ.get("XDG_CACHE_HOME", str(Path.home() / ".cache"))) / "weave")
USER_CACHE = Path(os.environ.get("WEAVE_CACHE_HOME", str(CACHE_BASE))).expanduser().resolve()
KINDS = ("firmware", "sdk", "android", "windows", "linux", "macos")


class ReleaseError(RuntimeError):
    pass


def filenames(version: str) -> dict[str, str]:
    if not re.fullmatch(r"\d+\.\d+\.\d+", version):
        raise ValueError("Invalid VERSION: expected major.minor.patch")
    return {
        kind: f"Weave-{version}-{kind}.{'tgz' if kind == 'firmware' else 'apk' if kind == 'android' else 'zip'}"
        for kind in KINDS
    }


def host_platform() -> str:
    if sys.platform == "win32":
        return "windows"
    if sys.platform == "darwin":
        return "macos"
    if sys.platform.startswith("linux"):
        return "linux"
    raise ReleaseError(f"Unsupported local desktop OS: {sys.platform}")


def validate_files(directory: Path, version: str, kinds=KINDS) -> None:
    names = filenames(version)
    for kind in kinds:
        path = directory / names[kind]
        if not path.is_file() or path.stat().st_size < 1000:
            raise ReleaseError(f"Missing or empty release asset: {path}")
        if kind == "sdk":
            from release_asset import validate_sdk
            validate_sdk(path, version)
        elif kind in ("windows", "linux", "macos"):
            with zipfile.ZipFile(path) as zf:
                if not zf.namelist() or zf.testzip():
                    raise ReleaseError(f"Invalid companion ZIP: {path}")
                executable = {
                    "windows": "Weave.exe",
                    "linux": "Weave",
                    "macos": "Weave.app/Contents/MacOS/Weave",
                }[kind]
                binaries = [i for i in zf.infolist() if i.filename.endswith(executable)]
                if not binaries:
                    raise ReleaseError(f"Native executable absent in {path}")
                with zf.open(binaries[0]) as fd:
                    magic = fd.read(4)
                valid = {
                    "windows": (b"MZ",),
                    "linux": (b"\x7fELF",),
                    "macos": (b"\xcf\xfa\xed\xfe", b"\xfe\xed\xfa\xcf", b"\xca\xfe\xba\xbe", b"\xbe\xba\xfe\xca"),
                }[kind]
                if not any(magic.startswith(prefix) for prefix in valid):
                    raise ReleaseError(f"Wrong executable format in {path}")
        elif kind == "android":
            with zipfile.ZipFile(path) as zf:
                if not {"AndroidManifest.xml", "classes.dex"}.issubset(zf.namelist()) or zf.testzip():
                    raise ReleaseError(f"Invalid Android APK: {path}")
        elif kind == "firmware":
            with tarfile.open(path, "r:gz") as tar:
                if not tar.getmembers():
                    raise ReleaseError(f"Invalid firmware TGZ: {path}")


def call_firmware_package() -> None:
    from release_asset import package_firmware, package_sdk
    try:
        package_firmware(BUILD / "dist" / "f7-C", OUT, VERSION)
        package_sdk(BUILD / "dist" / "f7-C", OUT, VERSION)
    except Exception as exc:
        raise ReleaseError(f"Firmware packaging failed: {exc}") from exc
    validate_files(OUT, VERSION, ("firmware", "sdk"))


def run_checked(command: list[str], cwd: Path) -> None:
    result = subprocess.run(command, cwd=cwd, check=False)
    if result.returncode:
        raise ReleaseError(f"Build command exited {result.returncode}: {' '.join(map(str, command))}")


def verify_native_serialport_bindings(directory: Path, target: str) -> None:
    """Require a target-compatible SerialPort native addon in packaged Electron.

    The .node bindings must be in app.asar.unpacked and match the target OS.
    Do not treat a cross-packaged Windows Electron exe with Linux native code
    as a working Windows application.
    """
    if target not in ('windows', 'linux'):
        return
    unpacked = directory / 'resources' / 'app.asar.unpacked'
    bindings = list(unpacked.rglob('*.node')) if unpacked.is_dir() else []
    if not bindings:
        raise ReleaseError(f'{target.capitalize()} USB native binding absent; build on {target} runner')
    magic = b'MZ' if target == 'windows' else b'\x7fELF'
    def has_target_magic(binary: Path) -> bool:
        with binary.open('rb') as fd:
            return fd.read(4).startswith(magic)
    if not any(has_target_magic(binary) for binary in bindings):
        expected = 'PE' if target == 'windows' else 'ELF'
        raise ReleaseError(f'{target.capitalize()} USB binding is not a {expected} binary; '
                           f'build on {target} runner')


def verify_cross_windows_native_bindings(directory: Path) -> None:
    """Backward-compatible validation used by release build helpers."""
    verify_native_serialport_bindings(directory, 'windows')


def windows_build_options(host: str, target: str) -> list[str]:
    # On Linux, Electron Builder would download winCodeSign just to modify the
    # executable resources. Avoid that optional external dependency for an
    # unsigned development ZIP. Native Windows packaging retains resource edits.
    if target == 'windows' and host != 'windows':
        return ['--config.win.signAndEditExecutable=false']
    return []


def desktop_native_build() -> tuple[list[str], str]:
    """Build actual Electron apps for supported local targets (no Python GUI)."""
    if os.environ.get('WEAVE_SKIP_DESKTOP') == '1':
        return [], 'Disabled via WEAVE_SKIP_DESKTOP'
    from bootstrap_tools import node_tools, ToolUnavailable
    from release_asset import package_desktop
    # Native first. Linux may also produce Windows unpacked builds; the native
    # serialport binding must be built for the target or that target is rejected.
    platform = host_platform()
    targets = (["linux", "windows"] if platform == "linux" else
               ["windows", "linux"] if platform == "windows" else ["macos"])
    desktop = ROOT.parent / 'apps' / 'desktop'
    try:
        node, npm = node_tools()
    except ToolUnavailable as exc:
        return [], f'Node.js unavailable: {exc}'
    env = os.environ.copy()
    env['PATH'] = str(Path(node).parent) + os.pathsep + env.get('PATH', '')
    env['npm_config_cache'] = str(USER_CACHE / 'tools' / 'npm-cache')
    # electron-builder cache is also kept under build/. This can be reused.
    env['ELECTRON_CACHE'] = str(USER_CACHE / 'tools' / 'electron-cache')
    env['ELECTRON_BUILDER_CACHE'] = str(USER_CACHE / 'tools' / 'electron-builder-cache')
    env['npm_config_fetch_retries'] = '5'
    env['npm_config_fetch_retry_mintimeout'] = '2500'
    env['npm_config_fetch_retry_maxtimeout'] = '10000'
    env['npm_config_fetch_timeout'] = '30000'

    def run(args: list[str]):
        result = subprocess.run(args, cwd=desktop, env=env, check=False)
        if result.returncode: raise ReleaseError(f'{args[0]} exited {result.returncode}')

    def installed() -> bool:
        modules = desktop / 'node_modules'
        return (
            (modules / '.bin' / 'vite').exists()
            and (modules / '.bin' / 'electron-builder').exists()
            and (modules / 'react' / 'package.json').is_file()
            and (modules / 'react-dom' / 'package.json').is_file()
        )

    try:
        # An interrupted `npm install` can leave node_modules/.bin behind while
        # react-dom or other modules are missing. Verify concrete dependencies.
        if not installed():
            if os.environ.get('WEAVE_NO_INSTALL') == '1':
                return [], 'npm dependencies incomplete (WEAVE_NO_INSTALL=1)'
            install_args = ['ci'] if (desktop / 'package-lock.json').exists() else ['install']
            run([npm, *install_args, '--prefer-offline', '--no-audit', '--no-fund'])
            if not installed():
                raise ReleaseError('npm install finished without React, React DOM or the desktop build tools')
        run([npm, 'run', 'build'])
    except (OSError, ReleaseError) as exc:
        return [], (f'Electron build failed: {exc}. If npm reports ENETUNREACH, '
                    'restore connectivity to registry.npmjs.org and retry with '
                    '`python3 fbt.py desktop` (no firmware rebuild).')
    built: list[str] = []
    notes: list[str] = []
    for target in targets:
        try:
            run([npm, 'exec', '--', 'electron-builder', '--publish', 'never', '--dir',
                 '--'+('mac' if target=='macos' else 'win' if target=='windows' else 'linux'),
                 *windows_build_options(platform, target)])
            location = desktop / 'release'
            if target == 'macos':
                candidates = list(location.glob('mac*/Weave.app'))
            elif target == 'windows':
                candidates = list(location.glob('win*-unpacked'))
            else:
                candidates = list(location.glob('linux*-unpacked'))
            if not candidates: raise ReleaseError(f'Missing Electron {target} package')
            verify_native_serialport_bindings(candidates[0], target)
            package_desktop(candidates[0], OUT, VERSION, target)
            validate_files(OUT, VERSION, (target,))
            built.append(target)
            notes.append(f'{target}: Electron native package ready')
        except (OSError, ReleaseError, zipfile.BadZipFile) as exc:
            (OUT / filenames(VERSION)[target]).unlink(missing_ok=True)
            notes.append(f'{target}: {exc} (build on native runner if needed)')
    return built, '; '.join(notes)


def android_local_build() -> tuple[bool, str]:
    """Build Android on any host using Java + SDK + auto-bootstrapped Gradle."""
    if os.environ.get('WEAVE_SKIP_ANDROID') == '1':
        return False, 'Disabled via WEAVE_SKIP_ANDROID'
    from release_asset import package_android
    from bootstrap_tools import gradle_command, android_sdk_dir, jdk_environment, ToolUnavailable
    android = ROOT.parent / 'apps' / 'android'
    output = BUILD / 'android' / 'app' / 'outputs' / 'apk'
    keystore = os.environ.get('WEAVE_ANDROID_KEYSTORE')
    release_env = bool(keystore and Path(keystore).is_file() and all(os.environ.get(k) for k in (
        'WEAVE_ANDROID_STORE_PASSWORD','WEAVE_ANDROID_KEY_ALIAS','WEAVE_ANDROID_KEY_PASSWORD')))
    mode = 'release' if release_env else 'debug'
    try:
        env = jdk_environment()
        sdk = android_sdk_dir(env)
        gradle = gradle_command()
        env['ANDROID_HOME']=str(sdk)
        env['ANDROID_SDK_ROOT']=str(sdk)
        command=gradle + ['--no-daemon','--gradle-user-home',str(USER_CACHE / 'tools' / 'gradle-cache'),f':app:assemble{mode.capitalize()}']
        result=subprocess.run(command,cwd=android,env=env,check=False)
        if result.returncode:raise ReleaseError(f'Gradle build exited {result.returncode}')
        filename = 'app-release.apk' if release_env else 'app-debug.apk'
        package_android(output / mode / filename, OUT, VERSION)
        validate_files(OUT, VERSION, ('android',))
        return True, ('release-signed Android APK' if release_env else 'debug-signed Android APK (development only)')
    except (OSError, subprocess.SubprocessError, ReleaseError, ToolUnavailable) as exc:
        (OUT / filenames(VERSION)['android']).unlink(missing_ok=True)
        return False, f'Android build failed: {exc}'


def record_status(built: list[str], notices: list[str]) -> None:
    BUILD.mkdir(parents=True, exist_ok=True)
    (BUILD / "release-status.json").write_text(json.dumps({
        "version": VERSION, "complete": len(built) == len(KINDS),
        "built": built, "missing": [kind for kind in KINDS if kind not in built],
        "notes": notices,
    }, indent=2) + "\n", encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--firmware-only", action="store_true")
    parser.add_argument("--desktop-only", action="store_true",
                        help="Build only desktop binaries (reuse existing firmware and Android assets)")
    options = parser.parse_args()
    try:
        if options.desktop_only:
            if options.firmware_only:
                raise ReleaseError('Choose --desktop-only or --firmware-only, not both')
            OUT.mkdir(parents=True, exist_ok=True)
            ready, note = desktop_native_build()
            print(f'Weave {VERSION}: desktop rebuild without firmware compilation')
            for kind in ('linux', 'windows', 'macos'):
                print(f"  {'OK     ' if kind in ready else 'PENDING'} {filenames(VERSION)[kind]}")
            print(f'  - {note}')
            if host_platform() not in ready:
                print('Weave: current OS desktop build missing; check npm/network logs.', file=sys.stderr)
                return 1
            return 0
        # Do not let previous-version or previous-attempt files masquerade as new builds.
        names = filenames(VERSION)
        OUT.mkdir(parents=True, exist_ok=True)
        for name in names.values():
            (OUT / name).unlink(missing_ok=True)
        call_firmware_package()
        built = ["firmware", "sdk"]
        notes: list[str] = []
        if not options.firmware_only:
            # Resolve the SDK/JDK and attempt the APK first: users get any
            # license/setup prompt immediately instead of after Electron builds.
            android, detail = android_local_build()
            notes.append(detail)
            if android:
                built.append("android")
            natives, detail = desktop_native_build()
            notes.append(detail)
            built.extend(natives)
        record_status(built, notes)
        print(f"\nWeave {VERSION}: local assets in {OUT}")
        for kind in KINDS:
            print(f"  {'OK     ' if kind in built else 'PENDING'} {names[kind]}")
        for note in notes:
            print(f"  - {note}")
        if len(built) != len(KINDS):
            print("Weave: remaining native OS builds are available from the manual GitHub Actions workflow.")
            print("Weave: no GH_TOKEN or GitHub repository configuration is required for local builds.")
        # A six-asset build is complete only when all six release files
        # have been built. macOS/Windows native bindings require native runners.
        if not options.firmware_only and len(built) != len(KINDS):
            print('Weave: incomplete six-asset build; use the GitHub Actions native runners for missing targets.', file=sys.stderr)
            return 1
        # A missing macOS binary is expected on non-macOS hosts, but an APK
        # setup failure must not be reported as a successful full build.
        if not options.firmware_only and os.environ.get('WEAVE_SKIP_ANDROID') != '1' and 'android' not in built:
            print('Weave: Android APK was not produced. Build failed; see Android error above.',
                  file=sys.stderr)
            return 1
        return 0
    except (ReleaseError, ValueError, OSError, zipfile.BadZipFile, tarfile.TarError) as exc:
        print(f"Weave build error: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
