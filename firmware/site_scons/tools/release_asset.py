#!/usr/bin/env python3
"""Build a named, validated release asset without mistaking debug files for releases."""
from __future__ import annotations

import argparse
import shutil
import os
import stat
import sys
import zipfile
from pathlib import Path


class PackageError(RuntimeError):
    pass


def package_firmware(source: Path, output: Path, version: str) -> Path:
    if not source.is_dir():
        raise PackageError(f"Firmware dist not found: {source}")
    # A prior build can leave another version in dist/. Never publish it.
    packages = sorted(source.glob(f"*update-{version}.tgz"))
    if len(packages) != 1:
        raise PackageError(f"Expected exactly one firmware TGZ for {version} in {source}, found {len(packages)}")
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"Weave-{version}-firmware.tgz"
    shutil.copy2(packages[0], destination)
    if destination.stat().st_size < 1000:
        raise PackageError("Firmware package looks too small")
    return destination


def package_desktop(source: Path, output: Path, version: str, platform: str) -> Path:
    if platform not in {"windows", "linux", "macos"}:
        raise PackageError("Unknown desktop platform")
    if not source.exists():
        raise PackageError(f"Desktop build not found: {source}")
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"Weave-{version}-{platform}.zip"
    # Zip a single packaged executable or the macOS .app bundle, never sources.
    if source.is_dir():
        files = [p for p in source.rglob("*") if p.is_file() or p.is_symlink()]
    else:
        files = [source]
    if not files:
        raise PackageError("Desktop build is empty")
    with zipfile.ZipFile(destination, "w", zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
        for path in files:
            name = (Path(source.name) / path.relative_to(source)) if source.is_dir() else Path(source.name)
            arcname = str(name).replace("\\", "/")
            if path.is_symlink():
                info = zipfile.ZipInfo(arcname)
                info.create_system = 3
                info.external_attr = (stat.S_IFLNK | 0o777) << 16
                archive.writestr(info, os.readlink(path).encode("utf-8"))
            else:
                archive.write(path, arcname=arcname, compress_type=zipfile.ZIP_DEFLATED)
    with zipfile.ZipFile(destination) as archive:
        bad = archive.testzip()
        if bad:
            raise PackageError(f"Corrupt ZIP entry: {bad}")
    return destination


def package_android(source: Path, output: Path, version: str) -> Path:
    if not source.is_file() or source.stat().st_size == 0:
        raise PackageError("Signed release APK is missing")
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"Weave-{version}-android.apk"
    shutil.copy2(source, destination)
    return destination


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("kind", choices=["firmware", "desktop", "android"])
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", default="release")
    parser.add_argument("--platform", choices=["windows", "linux", "macos"])
    parser.add_argument("--version", default=Path(__file__).resolve().parents[3].joinpath("VERSION").read_text().strip())
    args = parser.parse_args()
    try:
        if args.kind == "firmware":
            path = package_firmware(Path(args.input), Path(args.output), args.version)
        elif args.kind == "desktop":
            if not args.platform:
                raise PackageError("--platform is required for desktop builds")
            path = package_desktop(Path(args.input), Path(args.output), args.version, args.platform)
        else:
            path = package_android(Path(args.input), Path(args.output), args.version)
        print(path)
    except PackageError as exc:
        sys.exit(str(exc))


if __name__ == "__main__":
    main()
