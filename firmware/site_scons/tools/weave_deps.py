#!/usr/bin/env python3
"""Dépendances externes hors du dépôt et nettoyage réversible."""

import configparser
import hashlib
import tarfile
import urllib.request
import os
import shutil
import stat
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CACHE_BASE = (Path(os.environ.get("LOCALAPPDATA", str(Path.home() / "AppData" / "Local"))) / "Weave" / "cache") \
    if os.name == "nt" else (Path(os.environ.get("XDG_CACHE_HOME", str(Path.home() / ".cache"))) / "weave")
CACHE = Path(os.environ.get("WEAVE_CACHE_HOME", str(CACHE_BASE))).expanduser().resolve() / "deps"
SOURCE = "https://github.com/flipperdevices/flipperzero-firmware.git"
MONOCYPHER_VERSION = "4.0.3"
MONOCYPHER = ROOT / "lib" / f"monocypher-{MONOCYPHER_VERSION}"
MONOCYPHER_SHA512 = (
    "40904ada5c7ee4f7741733e38b69a30a4b0561cbffba5ffe7c2dce16136d540251"
    "ec0d9056ff606510d3b5b708fb8a40db7e0870d4a0b2dc17ba2bfb880f8965"
)
MONOCYPHER_FILES = (
    "src/monocypher.c", "src/monocypher.h",
    "src/optional/monocypher-ed25519.c", "src/optional/monocypher-ed25519.h",
    "LICENCE.md",
)



def sources():
    config = configparser.ConfigParser()
    config.read(ROOT.parent / ".gitmodules")
    return [ROOT.parent / config[name]["path"] for name in config.sections()] + [MONOCYPHER]


def ready(path):
    if not path.is_dir():
        return False
    if path.name == MONOCYPHER.name:
        return all((path / item).is_file() for item in MONOCYPHER_FILES)
    return any(item.is_file() and item.name != ".git" for item in path.rglob("*"))


def run(cmd, cwd=None):
    subprocess.run(cmd, cwd=cwd, check=True)


def is_link(path):
    if path.is_symlink():
        return True
    if os.name == "nt" and path.exists():
        attributes = os.stat(path, follow_symlinks=False).st_file_attributes
        return bool(attributes & stat.FILE_ATTRIBUTE_REPARSE_POINT)
    return False


def cache_path(path):
    return CACHE / path.relative_to(ROOT)


def managed_link(path):
    # Ne retirer que les liens WEAVE vers le cache attendu.
    return is_link(path) and path.resolve() == cache_path(path).resolve()


def previous_cache_link(path):
    """Recognize Weave's own symlinks after their source folder was moved.

    Existing build/deps links use absolute paths; moving the folder makes them
    stale. Never follow/rewrite arbitrary symlinks to unrelated directories.
    """
    if not is_link(path):
        return False
    relative = path.relative_to(ROOT)
    resolved = path.resolve(strict=False)
    # Cache locations from previous Weave versions, and the current cache
    # when the source tree is moved between machines (e.g. a GitHub runner).
    # Match whole path components, never a substring of an arbitrary target.
    for prefix in (
        ("build", "deps"),
        ("weave", "deps"),
        ("cache", "deps"),
        ("build", ".work", "deps"),
    ):
        tail = (*prefix, *relative.parts)
        if resolved.parts[-len(tail):] == tail:
            return True
    return False


def relocate_cache_link(path):
    if not is_link(path) or managed_link(path):
        return
    if not previous_cache_link(path):
        # A checkout may contain stale, absolute symlinks committed by
        # accident. In the ephemeral GitHub Actions workspace, an already
        # broken dependency link contains no data worth preserving. Never
        # unlink an unknown *working* link or a local user's custom link.
        if os.environ.get("GITHUB_ACTIONS") == "true" and not path.exists():
            unlink(path)
            print(f"Weave : lien de dépendance cassé retiré (CI) : {path.relative_to(ROOT)}", flush=True)
            return
        raise RuntimeError(f"Lien non géré : {path} -> {os.readlink(path) if path.is_symlink() else '?'}")
    previous = path.resolve(strict=False)
    cached = cache_path(path)
    if not ready(cached) and ready(previous):
        cached.parent.mkdir(parents=True, exist_ok=True)
        shutil.copytree(previous, cached, symlinks=True, dirs_exist_ok=True,
                        ignore=shutil.ignore_patterns('.git'))
    unlink(path)
    print(f"Weave : lien du projet déplacé réparé : {path.relative_to(ROOT)}", flush=True)


def unlink(path):
    if path.is_symlink():
        path.unlink()
    else:
        path.rmdir()  # Jonction Windows (ne suit pas la cible).


def clean_links():
    for path in sources():
        if managed_link(path):
            unlink(path)
            print(f"Weave clean: lien {path.relative_to(ROOT)}")


def link_cache(target):
    cached = cache_path(target)
    if not ready(cached):
        raise RuntimeError(f"Cache incomplet : {cached}")
    if is_link(target):
        if not managed_link(target):
            raise RuntimeError(f"Lien non géré : {target}")
        unlink(target)
    elif target.exists():
        if not target.is_dir() or any(target.iterdir()):
            raise RuntimeError(f"Source existante non gérée : {target}")
        target.rmdir()
    target.parent.mkdir(parents=True, exist_ok=True)
    if os.name == "nt":
        run(["cmd", "/d", "/c", "mklink", "/J", str(target), str(cached)])
    else:
        target.symlink_to(cached, target_is_directory=True)


def bootstrap_monocypher():
    """Pin the release archive, extract only required source/license files.

    Monocypher is not a Flipper upstream submodule. Cache it separately and
    use the usual managed link, so `clean` restores the source layout.
    """
    CACHE.mkdir(parents=True, exist_ok=True)
    cached = cache_path(MONOCYPHER)
    urls = (
        f"https://monocypher.org/download/monocypher-{MONOCYPHER_VERSION}.tar.gz",
        f"https://github.com/LoupVaillant/Monocypher/releases/download/{MONOCYPHER_VERSION}/monocypher-{MONOCYPHER_VERSION}.tar.gz",
    )
    with tempfile.TemporaryDirectory(prefix=".monocypher-", dir=CACHE) as folder:
        archive = Path(folder) / "release.tar.gz"
        failures = []
        for url in urls:
            try:
                request = urllib.request.Request(url, headers={"User-Agent": "Weave-Build/1.0.4"})
                with urllib.request.urlopen(request, timeout=60) as response:
                    data = response.read(8 * 1024 * 1024 + 1)
                if len(data) > 8 * 1024 * 1024 or hashlib.sha512(data).hexdigest() != MONOCYPHER_SHA512:
                    raise RuntimeError("Monocypher release checksum mismatch")
                archive.write_bytes(data)
                break
            except (OSError, RuntimeError) as error:
                failures.append(f"{url}: {error}")
        else:
            raise RuntimeError("Monocypher unavailable: " + "; ".join(failures))
        staged = Path(folder) / "staged"
        with tarfile.open(archive, "r:gz") as tar:
            for name in MONOCYPHER_FILES:
                member = tar.getmember(f"monocypher-{MONOCYPHER_VERSION}/{name}")
                if not member.isfile():
                    raise RuntimeError(f"Invalid Monocypher archive entry: {name}")
                target = staged / name
                target.parent.mkdir(parents=True, exist_ok=True)
                with tar.extractfile(member) as source, target.open("wb") as out:
                    shutil.copyfileobj(source, out)
        if not all((staged / name).is_file() for name in MONOCYPHER_FILES):
            raise RuntimeError("Incomplete Monocypher archive")
        cached.parent.mkdir(parents=True, exist_ok=True)
        if cached.exists():
            if ready(cached):
                link_cache(MONOCYPHER)
                return
            if any(cached.iterdir()):
                raise RuntimeError(f"Incomplete non-empty Monocypher cache: {cached}")
            cached.rmdir()
        staged.rename(cached)
        link_cache(MONOCYPHER)
        print(f"Weave: Monocypher {MONOCYPHER_VERSION} SHA-512 verified", flush=True)


def bootstrap(missing):
    if MONOCYPHER in missing:
        bootstrap_monocypher()
        missing = [target for target in missing if target != MONOCYPHER]
    if not missing:
        return
    CACHE.mkdir(parents=True, exist_ok=True)
    url = os.environ.get("WEAVE_UPSTREAM_URL", SOURCE)
    ref = os.environ.get("WEAVE_UPSTREAM_REF", "1.5.1")
    print("Weave : récupération des dépendances manquantes", flush=True)
    with tempfile.TemporaryDirectory(prefix=".fetch-", dir=CACHE) as folder:
        upstream = Path(folder) / "firmware"
        run(["git", "clone", "--quiet", "--depth", "1", "--branch", ref,
             url, str(upstream)])
        run(["git", "submodule", "update", "--init", "--recursive",
             "--jobs", "4"], cwd=upstream)
        for target in missing:
            source = upstream / target.relative_to(ROOT)
            if not ready(source):
                raise RuntimeError(f"Dépendance absente : {source}")
            cached = cache_path(target)
            cached.parent.mkdir(parents=True, exist_ok=True)
            staged = Path(folder) / "staged" / target.relative_to(ROOT)
            staged.parent.mkdir(parents=True, exist_ok=True)
            shutil.copytree(source, staged, symlinks=True,
                            ignore=shutil.ignore_patterns(".git"))
            if target.name == "protobuf":
                version = subprocess.check_output(
                    ["git", "-C", str(source), "describe", "--tags", "--abbrev=0"],
                    text=True,
                ).strip()
                parts = version.split(".")
                if len(parts) != 2 or not all(part.isdigit() for part in parts):
                    raise RuntimeError(f"Version protobuf inconnue : {version}")
                (staged / ".weave-version").write_text(version + "\n")
            if cached.exists():
                if any(cached.iterdir()):
                    raise RuntimeError(f"Cache non vide : {cached}")
                cached.rmdir()
            staged.rename(cached)
            link_cache(target)
            print(f"Weave : {target.relative_to(ROOT)}", flush=True)
    print("Weave : dépendances prêtes", flush=True)


def main():
    missing = []
    try:
        for path in sources():
            relocate_cache_link(path)
            if ready(path):
                continue
            if ready(cache_path(path)):
                link_cache(path)
            else:
                missing.append(path)
        if not missing:
            return 0
        if "--check" in sys.argv:
            for item in missing:
                print(f"Absent : {item.relative_to(ROOT)}")
            return 1
        if os.environ.get("WEAVE_NO_BOOTSTRAP"):
            print("Weave : dépendances absentes (bootstrap désactivé)", file=sys.stderr)
            return 1
        bootstrap(missing)
    except (OSError, subprocess.CalledProcessError, RuntimeError, tarfile.TarError, KeyError) as error:
        print(f"Weave : dépendances indisponibles : {error}", file=sys.stderr)
        return 1
    return 0 if all(ready(path) for path in sources()) else 1


if __name__ == "__main__":
    sys.exit(main())
