#!/usr/bin/env python3
"""Host policy and exact HAL MPU source tests. Neither proves hardware isolation."""
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SRC = ROOT / 'applications/services/weave'
HAL = ROOT / 'targets/f7/furi_hal/furi_hal_mpu.c'
FLAGS = ['gcc', '-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-Werror',
         '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-no-pie']

MOCK = r'''
#pragma once
#include <stdint.h>
#define LL_MPU_ACCESS_BUFFERABLE 1U
#define LL_MPU_ACCESS_CACHEABLE 2U
#define LL_MPU_ACCESS_SHAREABLE 4U
#define LL_MPU_TEX_LEVEL1 8U
#define LL_MPU_INSTRUCTION_ACCESS_ENABLE 0U
#define LL_MPU_CTRL_PRIVILEGED_DEFAULT 4U
#define LL_MPU_REGION_NO_ACCESS 0U
#define LL_MPU_REGION_PRIV_RO_URO 0x06000000U
#define MPU_RASR_SIZE_Pos 8U
uint32_t __get_PRIMASK(void);
void __disable_irq(void);
void __set_PRIMASK(uint32_t);
void __DSB(void);
void __ISB(void);
void LL_MPU_Enable(uint32_t);
void LL_MPU_Disable(void);
void LL_MPU_ConfigRegion(uint32_t, uint32_t, uint32_t, uint32_t);
void LL_MPU_DisableRegion(uint32_t);
'''
TEST = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "furi_hal_mpu.h"
#include "stm32wbxx_ll_cortex.h"
char _stack_end, _stack_size;
static uint32_t mask, enabled=1, barriers, writes, region, address, attrs, disabled;
uint32_t __get_PRIMASK(void){return mask;}
void __disable_irq(void){mask=1;}
void __set_PRIMASK(uint32_t x){mask=x;}
void __DSB(void){++barriers;}
void __ISB(void){++barriers;}
void LL_MPU_Enable(uint32_t x){(void)x;enabled=1;}
void LL_MPU_Disable(void){assert(!"Unexpected global disable");}
void LL_MPU_DisableRegion(uint32_t r){assert(enabled && mask);disabled=r;++writes;}
void LL_MPU_ConfigRegion(uint32_t r,uint32_t s,uint32_t a,uint32_t at){
  assert(enabled && mask && disabled==r && s==0);region=r;address=a;attrs=at;++writes;
}
extern void furi_hal_mpu_set_stack_protection(uint32_t*);
int main(void){
  for(unsigned saved=0;saved<2;++saved){
    mask=saved;
    for(unsigned offset=0;offset<64;offset+=4){
      uint32_t addr=0x20001000+offset;
      furi_hal_mpu_set_stack_protection((uint32_t*)(uintptr_t)addr);
      assert(mask==saved && enabled && region==2 && address==((addr+31U)&~31U));
      assert((attrs&0x06000000U)==0x06000000U);
    }
    furi_hal_mpu_protect_no_access(FuriHalMpuRegion3,0x20002000,FuriHalMPURegionSize32B);
    assert(mask==saved && region==3 && address==0x20002000);
    furi_hal_mpu_protect_disable(FuriHalMpuRegion3);assert(mask==saved && disabled==3);
  }
  assert(writes==70 && barriers>100);
  puts("HAL MPU: 32-byte alignment, region-local update, barriers and PRIMASK preservation PASS");
}
'''

if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='weave-hw-policy-') as tmp:
        p = Path(tmp)
        exe = p/'policy'
        subprocess.run([*FLAGS, '-I',str(SRC),str(SRC/'weave_hardware_policy.c'),
                        str(HERE/'test_hardware_policy.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
        exe = p/'trusted-record'
        subprocess.run([*FLAGS, '-I',str(SRC),str(SRC/'weave_hardware_policy.c'),
                        str(SRC/'weave_trusted_record.c'),str(HERE/'test_trusted_record.c'),
                        '-lcrypto','-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
        (p/'stm32wbxx_ll_cortex.h').write_text(MOCK)
        (p/'stm32wb55_linker.h').write_text('extern char _stack_end, _stack_size;\n')
        (p/'core').mkdir()
        (p/'core/check.h').write_text('#include <assert.h>\n#define furi_check(x) assert(x)\n')
        (p/'test.c').write_text(TEST)
        exe=p/'hal'
        subprocess.run([*FLAGS, '-I',str(p),'-I',str(ROOT/'targets/furi_hal_include'),
                        str(HAL),str(p/'test.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
