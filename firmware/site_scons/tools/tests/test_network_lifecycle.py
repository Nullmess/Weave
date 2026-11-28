#!/usr/bin/env python3
"""Real network FIFO and gateway with strict host mutex/clock doubles (not RF)."""
from pathlib import Path
import subprocess, tempfile
HERE=Path(__file__).resolve().parent
SRC=HERE.parents[2]/'applications/services/weave'
with tempfile.TemporaryDirectory() as td:
    p=Path(td)
    (p/'furi/core').mkdir(parents=True)
    (p/'furi/core/memmgr_heap.h').write_text('#pragma once\n#include <stddef.h>\nsize_t memmgr_heap_get_max_free_block(void);\n')
    (p/'furi.h').write_text('''#pragma once
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#define furi_check(x) assert(x)
#define COUNT_OF(x) (sizeof(x)/sizeof((x)[0]))
#define FuriWaitForever 0
#define MIN(a,b) ((a)<(b)?(a):(b))
typedef struct { bool held; } FuriMutex;
int furi_mutex_acquire(FuriMutex*, int);
int furi_mutex_release(FuriMutex*);
int32_t furi_kernel_lock(void);
void furi_kernel_unlock(void);
uint32_t furi_get_tick(void);
uint32_t furi_ms_to_ticks(uint32_t);
void furi_delay_ms(uint32_t);
''')
    exe=p/'test'
    subprocess.run(['gcc','-D_POSIX_C_SOURCE=200809L','-std=c11','-Wall','-Wextra','-Werror','-Werror=redundant-decls','-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie','-g','-I'+str(p),'-I'+str(SRC),str(HERE/'test_network_lifecycle.c'),str(SRC/'weave.c'),str(SRC/'weave_gateway.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
