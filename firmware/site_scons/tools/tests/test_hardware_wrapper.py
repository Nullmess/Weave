#!/usr/bin/env python3
"""Host-only AES-GCM wrapper regression with OpenSSL and sanitizers; not real HAL."""
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = ROOT / 'applications/services/weave'
HAL = ROOT / 'targets/furi_hal_include'

if __name__ == '__main__':
    with tempfile.TemporaryDirectory() as temp:
        exe = Path(temp) / 'test_hardware_wrapper'
        subprocess.run(['gcc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
                        '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie',
                        '-I', str(SOURCE), '-I', str(HAL), str(SOURCE/'weave_security_hw.c'),
                        str(HERE/'test_hardware_wrapper.c'), '-lcrypto', '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)
