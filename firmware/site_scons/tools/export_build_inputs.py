#!/usr/bin/env python3
"""Export external firmware sources for offline integration; never export credentials or caches wholesale."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import zipfile

import weave_deps


def dependency_files(root):
    # The dependency root may be a managed cache link. Do not follow links
    # inside the dependency tree (including links to files outside it).
    root = root.resolve(strict=True)
    for directory, subdirs, files in os.walk(root, followlinks=False):
        base = Path(directory)
        subdirs[:] = sorted(name for name in subdirs if name != '.git')
        for name in subdirs:
            if (base / name).is_symlink():
                raise RuntimeError(f'Nested directory link unsupported: {base / name}')
        for name in sorted(files):
            if name == '.git':
                continue
            file = base / name
            if file.is_symlink() or not file.is_file():
                raise RuntimeError(f'Non-regular dependency file: {file}')
            yield file, file.relative_to(root)


def export(output, roots):
    missing = [path for path in roots if not weave_deps.ready(path)]
    if missing:
        raise RuntimeError('Missing dependencies: ' + ', '.join(str(path) for path in missing))
    output = output.resolve()
    if output.exists():
        raise RuntimeError(f'Output already exists: {output}')
    if any(output.is_relative_to(root.resolve()) for root in roots):
        raise RuntimeError("Output must be outside dependency trees")
    output.parent.mkdir(parents=True, exist_ok=True)
    entries = {}
    with tempfile.TemporaryDirectory(prefix='.weave-inputs-', dir=output.parent) as temp:
        staged = Path(temp) / 'inputs.zip'
        with zipfile.ZipFile(staged, 'w', compression=zipfile.ZIP_DEFLATED, compresslevel=6) as archive:
            for root in roots:
                prefix = root.relative_to(weave_deps.ROOT.parent)
                for file, relative in dependency_files(root):
                    name = (prefix / relative).as_posix()
                    if name in entries:
                        raise RuntimeError(f'Duplicate dependency file: {name}')
                    content = file.read_bytes()
                    entries[name] = hashlib.sha256(content).hexdigest()
                    archive.writestr(name, content)
            archive.writestr('weave-inputs-manifest.json', json.dumps({
                'format': 1,
                'weave_version': (weave_deps.ROOT.parent / 'VERSION').read_text().strip(),
                'upstream_ref_requested': os.environ.get('WEAVE_UPSTREAM_REF', '1.5.1'),
                'files_sha256': entries,
            }, indent=2, sort_keys=True) + '\n')
        # Link rather than overwrite an existing output, even on a race.
        os.link(staged, output)
    print(f'Exported {len(entries)} dependency files to {output}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    try:
        export(args.output, weave_deps.sources())
    except (OSError, RuntimeError, ValueError) as error:
        parser.exit(1, f'Build inputs not exported: {error}\n')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
