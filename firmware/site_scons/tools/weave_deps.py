#!/usr/bin/env python3
"""Dépendances externes hors du dépôt et nettoyage réversible."""

import configparser
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


def sources():
    config = configparser.ConfigParser()
    config.read(ROOT.parent / ".gitmodules")
    return [ROOT.parent / config[name]["path"] for name in config.sections()]


def ready(path):
    if not path.is_dir():
        return False
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
    tail = ("build", "deps", *relative.parts)
    resolved = path.resolve(strict=False)
    return resolved.parts[-len(tail):] == tail


def relocate_cache_link(path):
    if not is_link(path) or managed_link(path):
        return
    if not previous_cache_link(path):
        raise RuntimeError(f"Lien non géré : {path}")
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


def bootstrap(missing):
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
    except (OSError, subprocess.CalledProcessError, RuntimeError) as error:
        print(f"Weave : dépendances indisponibles : {error}", file=sys.stderr)
        return 1
    return 0 if all(ready(path) for path in sources()) else 1


if __name__ == "__main__":
    sys.exit(main())
