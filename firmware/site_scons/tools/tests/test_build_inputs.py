#!/usr/bin/env python3
"""Dependency export tests: synthetic source trees, not downloaded dependencies."""
import hashlib
import json
from pathlib import Path
import sys
import subprocess
import tempfile
import unittest
from unittest.mock import patch, call
import zipfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
import export_build_inputs as exporter
import weave_deps as deps


class BuildInputsTest(unittest.TestCase):
    def test_fetch_only_requested_submodule(self):
        upstream = Path('/upstream')
        with patch.object(deps, 'retry_git') as run:
            deps.fetch_dependency(upstream, Path('lib/FreeRTOS-Kernel'))
            deps.fetch_dependency(upstream, Path('lib/mbedtls'))
        self.assertEqual(run.call_args_list, [
            call(['submodule', 'update', '--init', '--jobs', '2', '--', 'lib/FreeRTOS-Kernel'],
                 cwd=upstream, label='submodule lib/FreeRTOS-Kernel'),
            call(['submodule', 'update', '--init', '--recursive', '--jobs', '2', '--', 'lib/mbedtls'],
                 cwd=upstream, label='submodule lib/mbedtls'),
        ])

    def test_retry_git(self):
        failure = subprocess.CalledProcessError(128, ['git'])
        with patch.object(deps, 'run', side_effect=[failure, None]) as run, \
             patch.object(deps.time, 'sleep') as sleep:
            deps.retry_git(['submodule', 'update'], label='fixture')
            self.assertEqual(run.call_count, 2)
            self.assertEqual(run.call_args.args[0][-2:], ['submodule', 'update'])
            sleep.assert_called_once_with(2)
        with patch.object(deps, 'run', side_effect=failure) as run, \
             patch.object(deps.time, 'sleep'):
            with self.assertRaisesRegex(RuntimeError, 'fixture: git exit 128 after 3 attempts'):
                deps.retry_git(['clone'], label='fixture')
            self.assertEqual(run.call_count, 3)

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
