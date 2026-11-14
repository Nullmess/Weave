#!/usr/bin/env python3
"""Weave CLI: isolate build intermediates, publish only release assets in build/."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys

PROJECT = Path(__file__).resolve().parent
FIRMWARE = PROJECT / "firmware"
PUBLIC_BUILD = PROJECT / "build"
WORK = PUBLIC_BUILD / ".work"
FIRMWARE_BUILD = FIRMWARE / "build"


def _managed_link() -> bool:
    """Never remove an unrelated directory or user-created symlink."""
    if FIRMWARE_BUILD.is_symlink():
        return FIRMWARE_BUILD.resolve(strict=False) == WORK.resolve(strict=False)
    if os.name == "nt" and FIRMWARE_BUILD.is_dir():
        try:
            return FIRMWARE_BUILD.is_junction() and FIRMWARE_BUILD.resolve() == WORK.resolve()
        except (OSError, AttributeError):
            pass
    return False


def _remove_path(path: Path) -> None:
    if path.is_symlink():
        path.unlink()
    elif os.name == "nt" and getattr(path, "is_junction", lambda: False)():
        path.rmdir()
    elif path.is_dir():
        shutil.rmtree(path)
    elif path.exists():
        path.unlink()


def _prepare_build() -> None:
    if PUBLIC_BUILD.is_symlink() or (PUBLIC_BUILD.exists() and not PUBLIC_BUILD.is_dir()):
        raise RuntimeError("build/ must be a real directory (not a symlink)")
    PUBLIC_BUILD.mkdir(exist_ok=True)
    WORK.mkdir(exist_ok=True)
    if _managed_link():
        return
    if FIRMWARE_BUILD.exists() or FIRMWARE_BUILD.is_symlink():
        raise RuntimeError("firmware/build exists outside the managed workspace. "
                           "Run 'python3 fbt.py clean' before switching layouts")
    if os.name == "nt":
        # NTFS directory junctions work without Windows Developer Mode.
        subprocess.run(["cmd", "/d", "/c", "mklink", "/J", str(FIRMWARE_BUILD),
                        str(WORK)], check=True)
    else:
        FIRMWARE_BUILD.symlink_to(WORK, target_is_directory=True)


def _checksum(path: Path) -> str:
    sha = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            sha.update(chunk)
    return sha.hexdigest()


def _publish(firmware_only: bool) -> None:
    """Stage, validate, swap release files; roll back if replacement fails."""
    tools = FIRMWARE / "site_scons" / "tools"
    sys.path.insert(0, str(tools))
    from build_all import VERSION, filenames, validate_files, host_platform

    status_file = WORK / "release-status.json"
    if not status_file.is_file():
        raise RuntimeError("Build did not generate release-status.json")
    status = json.loads(status_file.read_text(encoding="utf-8"))
    built = status.get("built", [])
    if status.get("version") != VERSION or not isinstance(built, list):
        raise RuntimeError("Invalid release status for the current VERSION")
    required = {"firmware"} if firmware_only else {"firmware", "android", host_platform()}
    if not required.issubset(set(built)):
        raise RuntimeError("Missing essential release assets: " + ", ".join(sorted(required - set(built))))
    names = filenames(VERSION)
    source = WORK / "releases"
    validate_files(source, VERSION, tuple(built))

    ready = PUBLIC_BUILD / ".ready"
    backup = PUBLIC_BUILD / ".previous"
    if ready.exists() or backup.exists():
        raise RuntimeError("An unfinished release swap exists (.ready/.previous); "
                           "inspect build/ before retrying")
    ready.mkdir()
    moved_new = []
    moved_old = []
    try:
        for kind in built:
            file = source / names[kind]
            target = ready / file.name
            shutil.copy2(file, target)
            if _checksum(file) != _checksum(target):
                raise RuntimeError("Release asset checksum mismatch: " + file.name)
        validate_files(ready, VERSION, tuple(built))
        backup.mkdir()
        for old in PUBLIC_BUILD.iterdir():
            if old.name in {".work", ".ready", ".previous"}:
                continue
            old.rename(backup / old.name)
            moved_old.append(old.name)
        for item in ready.iterdir():
            item.rename(PUBLIC_BUILD / item.name)
            moved_new.append(item.name)
    except Exception:
        for name in moved_new:
            _remove_path(PUBLIC_BUILD / name)
        for name in moved_old:
            if (backup / name).exists() or (backup / name).is_symlink():
                (backup / name).rename(PUBLIC_BUILD / name)
        raise
    finally:
        if ready.exists():
            _remove_path(ready)
    _remove_path(backup)
    if not _managed_link():
        raise RuntimeError("Unsafe firmware/build link; refusing to delete build workspace")
    _remove_path(FIRMWARE_BUILD)
    # Electron must compile in its own application folder so its packaging
    # resolves renderer/main paths correctly. Remove those intermediate outputs
    # only after all ZIPs have been validated and published.
    desktop = PROJECT / "apps" / "desktop"
    for name in ("dist", "dist-electron", "release", "build"):
        generated = desktop / name
        if generated.exists() or generated.is_symlink():
            _remove_path(generated)
    scons_state = FIRMWARE / ".sconsign.dblite"
    if scons_state.exists():
        scons_state.unlink()
    _remove_path(WORK)
    print("\nWeave: final release assets in build/ (no intermediate files):", flush=True)
    for kind in built:
        print("  " + names[kind], flush=True)
    missing = [kind for kind in names if kind not in built]
    if missing:
        print("Weave: other platforms pending native CI: " + ", ".join(missing), flush=True)


def _invoke(args: list[str]) -> int:
    return subprocess.call([sys.executable, str(FIRMWARE / "fbt.py"), *args], cwd=FIRMWARE)


def main() -> int:
    args = sys.argv[1:]
    if args and args[0] in ("clean", "clear"):
        result = _invoke(args)
        if result == 0 and PUBLIC_BUILD.exists():
            if PUBLIC_BUILD.is_symlink():
                raise RuntimeError("Refusing to clean symlinked build/")
            _remove_path(PUBLIC_BUILD)
        return result
    if not args or args[0] != "build":
        return _invoke(args)
    firmware_only = "--firmware-only" in args
    _prepare_build()
    # Previous failed attempts must not be mistaken for a fresh release.
    for stale in (WORK / "release-status.json", WORK / "releases"):
        if stale.exists():
            _remove_path(stale)
    result = _invoke(args)
    if result:
        # The local builder deliberately returns 1 if native macOS/Windows
        # assets require other OSes. Its status and every essential asset are
        # independently validated before treating this as a local success.
        if not (WORK / "release-status.json").is_file():
            print("Weave: build failed; previous build/ assets preserved.", file=sys.stderr)
            return result
    try:
        _publish(firmware_only)
    except (OSError, ValueError, RuntimeError, KeyError, json.JSONDecodeError) as exc:
        print(f"Weave: release not published: {exc}", file=sys.stderr)
        print("Weave: previous assets preserved; intermediates remain in build/.work/.", file=sys.stderr)
        return result or 1
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print("\nWeave: build interrupted; previous release assets preserved.", file=sys.stderr)
        sys.exit(130)
    except (OSError, RuntimeError, subprocess.CalledProcessError) as exc:
        print(f"Weave: {exc}", file=sys.stderr)
        sys.exit(1)
