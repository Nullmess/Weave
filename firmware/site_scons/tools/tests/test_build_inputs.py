#!/usr/bin/env python3
"""Dependency export tests: synthetic source trees, not downloaded dependencies."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import export_build_inputs as exporter


class BuildInputsTest(unittest.TestCase):
    def test_export(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            firmware = root / 'firmware'
            dep = firmware / 'lib' / 'test'
            cache = root / 'cache'
            cache.mkdir()
            dep.parent.mkdir(parents=True)
            dep.symlink_to(cache, target_is_directory=True)
            (root / 'VERSION').write_text('1.0.4\n')
            (cache / 'source.c').write_bytes(b'test source')
            (cache / '.git').write_text('not exported')
            out = root / 'inputs.zip'
            with patch.object(exporter.weave_deps, 'ROOT', firmware):
                exporter.export(out, [dep])
                with zipfile.ZipFile(out) as archive:
                    name = 'firmware/lib/test/source.c'
                    self.assertEqual(set(archive.namelist()), {name, 'weave-inputs-manifest.json'})
                    manifest = json.loads(archive.read('weave-inputs-manifest.json'))
                    self.assertEqual(manifest['files_sha256'][name], hashlib.sha256(b'test source').hexdigest())
                with self.assertRaises(RuntimeError):
                    exporter.export(out, [dep])
                with self.assertRaises(RuntimeError):
                    exporter.export(root / 'missing.zip', [firmware / 'missing'])
                with self.assertRaises(RuntimeError):
                    exporter.export(cache / 'nested.zip', [dep])
                (cache / 'outside').symlink_to(root / 'VERSION')
                with self.assertRaises(RuntimeError):
                    exporter.export(root / 'unsafe.zip', [dep])
                self.assertFalse((root / 'unsafe.zip').exists())


if __name__ == '__main__':
    unittest.main(verbosity=2)
