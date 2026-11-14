#!/usr/bin/env python3
"""Locate Electron-builder's native --dir output and publish its ZIP."""
import argparse
from pathlib import Path
from release_asset import package_desktop
from build_all import validate_files, verify_native_serialport_bindings

root=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser()
parser.add_argument('--platform',required=True,choices=['linux','windows','macos'])
parser.add_argument('--output',default='build')
args=parser.parse_args()
desktop=root.parent/'apps/desktop/release'
if args.platform=='macos':candidates=list(desktop.glob('mac*/Weave.app'))
elif args.platform=='windows':candidates=list(desktop.glob('win*-unpacked'))
else:candidates=list(desktop.glob('linux*-unpacked'))
if not candidates:raise SystemExit('No native Electron package produced')
verify_native_serialport_bindings(candidates[0], args.platform)
version=(root.parent/'VERSION').read_text().strip()
path=package_desktop(candidates[0],root.parent/args.output,version,args.platform)
validate_files(root.parent/args.output, version, (args.platform,))
print(path)
