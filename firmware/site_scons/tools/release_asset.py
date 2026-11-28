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


def validate_sdk(path: Path, version: str) -> None:
    import json
    with zipfile.ZipFile(path) as archive:
        names = set(archive.namelist())
        if archive.testzip():
            raise PackageError(f"Corrupt SDK ZIP: {path}")
        manifest = json.loads(archive.read("components.json"))
        components = manifest["components"]
        meta = manifest["meta"]
        if meta["hw_target"] != "f7" or meta["version"] != version:
            raise PackageError("Wrong SDK target or version")
        if components["scripts.dir"] != "scripts":
            raise PackageError("uFBT requires scripts/ at SDK root")
        required = {"scripts/ufbt/SConstruct", "scripts/ufbt/site_tools/ufbt_state.py",
                    "scripts/runfap.py", "scripts/toolchain/fbtenv.sh",
                    components["sdk_headers.dir"] + "/sdk.opts",
                    components["firmware.elf"], components["full.bin"]}
        if not required.issubset(names):
            raise PackageError(f"Missing SDK files: {required - names}")
        for component in ("update.dir", "lib.dir"):
            if not any(n.startswith(components[component] + "/") for n in names):
                raise PackageError(f"Missing SDK component: {component}")
        symbols = [n for n in names if n.endswith("/api_symbols.csv")]
        if len(symbols) != 1:
            raise PackageError("SDK API symbol manifest missing or ambiguous")
        import csv, io
        rows = list(csv.DictReader(io.StringIO(archive.read(symbols[0]).decode())))
        version_row = next((r for r in rows if r["entry"] == "Version"), None)
        if not version_row or version_row["name"] != "89.4" or version_row["status"] != "+":
            raise PackageError("Expected approved SDK ABI 89.4")
        # Reject any stale or incomplete symbol table, including disabled exports.
        source = Path(__file__).resolve().parents[2] / "targets/f7/api_symbols.csv"
        with source.open(newline="", encoding="utf-8") as stream:
            expected = list(csv.DictReader(stream))
        canonical = lambda records: {tuple(sorted(r.items())) for r in records}
        if canonical(rows) != canonical(expected):
            raise PackageError("SDK symbol table differs from the current source API")
        sdk_opts = json.loads(archive.read(components["sdk_headers.dir"] + "/sdk.opts"))
        if str(sdk_opts["hardware"]) != "7":
            raise PackageError("Wrong sdk.opts hardware target")
        elf = archive.open(components["firmware.elf"]).read(20)
        if len(elf) != 20 or elf[:6] != b"\x7fELF\x01\x01" or elf[18:20] != b"\x28\x00":
            raise PackageError("SDK firmware ELF is not ARM32 little-endian")
        if archive.getinfo(components["full.bin"]).file_size == 0:
            raise PackageError("Empty SDK firmware image")


def package_sdk(source: Path, output: Path, version: str) -> Path:
    packages = sorted(source.glob(f"weave-f7-sdk-{version}.zip"))
    if len(packages) != 1:
        raise PackageError(f"Expected SDK archive in {source}")
    validate_sdk(packages[0], version)
    output.mkdir(parents=True, exist_ok=True)
    destination = output / f"Weave-{version}-sdk.zip"
    temporary = destination.with_suffix(".zip.tmp")
    try:
        shutil.copy2(packages[0], temporary)
        validate_sdk(temporary, version)
        os.replace(temporary, destination)
    finally:
        temporary.unlink(missing_ok=True)
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
    parser.add_argument("kind", choices=["firmware", "desktop", "android", "sdk"])
    parser.add_argument("--input", required=True)
    parser.add_argument("--output", default="release")
    parser.add_argument("--platform", choices=["windows", "linux", "macos"])
    parser.add_argument("--version", default=Path(__file__).resolve().parents[3].joinpath("VERSION").read_text().strip())
    args = parser.parse_args()
    try:
        if args.kind == "firmware":
            path = package_firmware(Path(args.input), Path(args.output), args.version)
        elif args.kind == "sdk":
            path = package_sdk(Path(args.input), Path(args.output), args.version)
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
