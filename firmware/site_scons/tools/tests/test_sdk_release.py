#!/usr/bin/env python3
"""SDK packaging regression using the actual bundle_sdk method and synthetic files.
The fixture is NOT a compiled SDK; no artifact from this test is published.
"""
import ast
import csv
import json
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
import zipfile

TOOLS = Path(__file__).resolve().parents[1]
ROOT = TOOLS.parents[1]
sys.path.insert(0, str(TOOLS))
from release_asset import package_sdk, validate_sdk, PackageError
from build_all import filenames, validate_files, VERSION


def bundle_function():
    # Load the actual method without pulling SCons' unrelated dependencies.
    tree = ast.parse((TOOLS/'sconsdist.py').read_text())
    cls = next(n for n in tree.body if isinstance(n, ast.ClassDef) and n.name=='Main')
    method = next(n for n in cls.body if isinstance(n, ast.FunctionDef) and n.name=='bundle_sdk')
    scope = dict(zipfile=zipfile, json=json, walk=os.walk, basename=os.path.basename,
                 join=os.path.join, relpath=os.path.relpath,
                 fg=types.SimpleNamespace(boldgreen=lambda s:s))
    exec(compile(ast.Module(body=[method],type_ignores=[]),str(TOOLS/'sconsdist.py'),'exec'),scope)
    return scope['bundle_sdk']


class SdkTest(unittest.TestCase):
    def test_bundle_and_publish(self):
        with tempfile.TemporaryDirectory() as tmp:
            p=Path(tmp); paths={}
            def write(name, data):
                path=p/name;path.parent.mkdir(parents=True,exist_ok=True);path.write_bytes(data);return str(path)
            # Only the ELF header is meaningful; this is a structural test fixture.
            elf=bytearray(64);elf[:6]=b'\x7fELF\x01\x01';elf[18:20]=b'\x28\x00'
            paths['firmware.elf']=write('firmware.elf',elf)
            paths['full.bin']=write('full.bin',b'fixture')
            paths['update.dir']=str(p/'update')
            write('update/update.fuf',b'fixture')
            paths['sdk_headers.dir']=str(p/'sdk_headers')
            write('sdk_headers/sdk.opts',json.dumps({'hardware':'7'}).encode())
            write('sdk_headers/f7_sdk/targets/f7/api_symbols.csv',(ROOT/'targets/f7/api_symbols.csv').read_bytes())
            paths['lib.dir']=str(p/'lib');write('lib/libtest.a',b'fixture')
            # Source directory really is named tools, reproducing the previous bug.
            paths['scripts.dir']=str(p/'site_scons/tools')
            for name in ['ufbt/SConstruct','ufbt/site_tools/ufbt_state.py','runfap.py','toolchain/fbtenv.sh']:
                write('site_scons/tools/'+name,(TOOLS/name).read_bytes())
            dist=p/'dist';dist.mkdir()
            src=dist/f'weave-f7-sdk-{VERSION}.zip'
            fake=types.SimpleNamespace(_dist_components=paths,target='f7',flavor='C',
                  args=types.SimpleNamespace(version=VERSION),
                  logger=types.SimpleNamespace(info=lambda _:None),
                  get_dist_path=lambda name:str(dist/name),get_dist_file_name=lambda *_:src.name)
            bundle_function()(fake)
            with zipfile.ZipFile(src) as z:
                self.assertIn('scripts/ufbt/SConstruct',z.namelist())
                self.assertFalse(any(n.startswith('tools/') for n in z.namelist()))
                self.assertEqual(json.loads(z.read('components.json'))['components']['scripts.dir'],'scripts')
                self.assertIn(b'scripts/ufbt/site_tools',z.read('scripts/ufbt/SConstruct'))
            validate_sdk(src,VERSION)
            dest=package_sdk(dist,p/'release',VERSION)
            self.assertEqual(dest.name,filenames(VERSION)['sdk'])
            validate_files(dest.parent,VERSION,('sdk',))
            old=dest.read_bytes()
            bad=p/'bad.zip'
            with zipfile.ZipFile(src) as inp, zipfile.ZipFile(bad,'w') as out:
                for item in inp.infolist():
                    contents=inp.read(item)
                    if item.filename=='components.json':
                        m=json.loads(contents);m['components']['scripts.dir']='tools';contents=json.dumps(m).encode()
                    out.writestr(item,contents)
            bad.replace(src)
            with self.assertRaises(PackageError):package_sdk(dist,dest.parent,VERSION)
            self.assertEqual(dest.read_bytes(),old)
            self.assertFalse(dest.with_suffix('.zip.tmp').exists())
            print('SDK canonical scripts path, ABI manifest, publication, invalid layout rejection and previous asset preservation: PASS')

    def test_symbol_hashes(self):
        sys.path.insert(0,str(TOOLS/'fbt/sdk'))
        from hashes import gnu_sym_hash
        with (ROOT/'targets/f7/api_symbols.csv').open() as stream:
            rows=list(csv.DictReader(stream))
        entries=[r for r in rows if r['entry'] in ('Function','Variable')]
        with (Path(__file__).parent/'api_v3.csv').open() as stream:
            previous=list(csv.DictReader(stream))
        current={tuple(sorted(r.items())) for r in rows}
        self.assertTrue(all(tuple(sorted(r.items())) in current for r in previous))
        print(f'Previous Weave ABI signatures retained: {len(previous)}')
        names=[r['name'] for r in entries]
        self.assertEqual(len(names),len(set(names)))
        hashes=[gnu_sym_hash(n) for n in names]
        self.assertEqual(len(hashes),len(set(hashes)))
        print(f'SDK {len(entries)} symbols: no duplicate names or GNU hash collisions')

if __name__=='__main__':unittest.main(verbosity=2)
