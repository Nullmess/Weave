#!/usr/bin/env python3
"""Host regression of NFC mailbox and actual native block parser, not RF or HAL."""
from pathlib import Path
import subprocess, tempfile
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SRC=ROOT/'applications/services/weave'
with tempfile.TemporaryDirectory() as td:
    p=Path(td)
    (p/'furi.h').write_text('''#pragma once
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define furi_assert(x) assert(x)
#define furi_check(x) assert(x)
#define UNUSED(x) ((void)(x))
#define FURI_BIT(a,b) (((a)>>(b))&1U)
''')
    # Verify the internal allocator is emitted without application SRV_* defines.
    # Preprocessor only: the native HAL is not part of this host test.
    import re
    for header in ('nfc.h', 'furi_hal_nfc.h', 'furi/furi.h'):
        target=p/header
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text('')
    native=p/'nfc.c'
    native.write_bytes((ROOT/'lib/nfc/nfc.c').read_bytes())
    for defines in ([], ['-DFURI_RAM_EXEC']):
        preprocessed=subprocess.check_output(['gcc','-E','-P','-I'+str(p),*defines,str(native)],text=True)
        assert re.search(r'\bNfc\s*\*\s*nfc_try_alloc\s*\([^)]*\)\s*\{',preprocessed), 'nfc_try_alloc missing from library'
    exe=p/'test'
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror','-fsanitize=address,undefined','-fno-pie','-no-pie','-g','-I'+str(p),'-I'+str(ROOT/'lib'),'-I'+str(SRC),str(HERE/'test_nfc_transport.c'),str(SRC/'weave_nfc_mailbox.c'),str(SRC/'weave_isodep.c'),str(ROOT/'lib/nfc/helpers/iso14443_4_layer.c'),str(ROOT/'lib/toolbox/bit_buffer.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
