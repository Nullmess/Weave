#!/usr/bin/env python3
"""Install an actual Weave SDK in an isolated uFBT state; build API v3/v4 smoke FAPs.
No flash, no permanent SDK changes. --launch explicitly uploads and starts one test.
With --legacy-sdk, build the v3 FAP with an actual earlier SDK (canonical scripts/ layout).
A successful build validates APPCHK, not physical execution. Record the PASS dialog separately.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from release_asset import validate_sdk


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sdk', type=Path, required=True)
    parser.add_argument('--legacy-sdk', type=Path)
    parser.add_argument('--launch', choices=('v3', 'v4'))
    parser.add_argument('--port', default='auto')
    args = parser.parse_args()
    sdk = args.sdk.resolve(strict=True)
    legacy = args.legacy_sdk.resolve(strict=True) if args.legacy_sdk else sdk
    validate_sdk(sdk, '1.0.3')
    ufbt = shutil.which('ufbt')
    if not ufbt:
        parser.error('uFBT executable missing; install ufbt in a virtual environment or pipx')
    with tempfile.TemporaryDirectory(prefix='weave-sdk-validation-') as temp:
        root = Path(temp)
        env = os.environ.copy()
        env['UFBT_HOME'] = str(root / 'ufbt')
        # Override any alternate externally configured SDK paths only in the child process.
        for key in ('UFBT_STATE_DIR', 'UFBT_SCRIPT_DIR'):
            env.pop(key, None)
        installed = None
        for version in ('v3', 'v4'):
            archive = legacy if version == 'v3' else sdk
            if archive != installed:
                subprocess.run([ufbt, 'update', '--hw-target=f7', f'--local={archive}'], env=env, check=True)
                installed = archive
            appid = 'weave_smoke_' + version
            project = root / appid
            project.mkdir()
            (project / 'application.fam').write_text(
                f'App(appid="{appid}", name="Weave Smoke {version}", '
                'apptype=FlipperAppType.EXTERNAL, entry_point="weave_smoke_app", '
                'stack_size=4096, fap_category="Tools")\n', encoding='utf-8')
            (project / 'smoke.c').write_text('#define SMOKE_V4 '+str(int(version=='v4'))+'\n'+
                                               (HERE/'security_smoke.c').read_text(), encoding='utf-8')
            cmd = [ufbt, 'STRICT_FAP_IMPORT_CHECK=1']
            subprocess.run(cmd, cwd=project, env=env, check=True)
            fap = project/'dist'/f'{appid}.fap'
            if not fap.is_file() or fap.stat().st_size == 0:
                raise RuntimeError(f'FAP missing: {fap}')
            print(f'{version}: build and strict APPCHK passed', flush=True)
            if args.launch == version:
                subprocess.run([ufbt, 'launch', 'STRICT_FAP_IMPORT_CHECK=1', f'FLIP_PORT={args.port}'],
                               cwd=project, env=env, check=True)
                print('Upload/start accepted. Verify the PASS dialog on the physical Flipper.', flush=True)
        if legacy == sdk:
            print('v3 uses legacy functions with the new SDK; loading a prebuilt 89.2 FAP remains a device test.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
