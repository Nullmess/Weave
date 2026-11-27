#!/usr/bin/env python3
"""Exercise the actual enclave lifecycle functions with fault-injected HAL doubles.
Tests resource cleanup; cannot model FUS or AES registers electrically.
"""
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SRC = ROOT/'targets/f7/furi_hal/furi_hal_crypto.c'

# Compile the exact selected functions, replacing unrelated hardware with a strict double.
source = SRC.read_text()
functions = source[source.index('static void crypto_wipe('):source.index('static void crypto_key_init(')] + source[source.index('bool furi_hal_crypto_enclave_load_key('):source.index('bool furi_hal_crypto_load_key(')]
prelude = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include "furi_hal_crypto.h"
#define furi_check(x) assert(x)
#define furi_crash(x) assert(0 && (x))
#define FuriWaitForever 0
#define FuriStatusOk 0
#define SHCI_Success 0
#define FuriHalBusAES1 0
#define AES_CR_EN 1
#define CLEAR_BIT(reg, bit) ((reg) &= ~(bit))
#define KEYTYPE_MASTER 0
#define KEYTYPE_SIMPLE 1
#define KEYTYPE_ENCRYPTED 2
#define KEYSIZE_16 16
#define KEYSIZE_32 32
typedef int SHCI_CmdStatus_t;
typedef struct {int KeyType, KeySize; uint8_t KeyData[44];} SHCI_C2_FUS_StoreUsrKey_Cmd_Param_t;
static int lock, bus, core2, bt_alive, fus_ok;
static int mutex_storage;
static int* furi_hal_crypto_mutex = &mutex_storage;
static bool furi_hal_crypto_mode_init_done;
static struct {uint32_t CR;} regs;
#define AES1 (&regs)
static int furi_mutex_acquire(int* m,int wait){(void)m;(void)wait;assert(!lock);lock=1;return 0;}
static int furi_mutex_release(int* m){(void)m;assert(lock && !core2 && !bus);lock=0;return 0;}
static void furi_hal_bus_enable(int b){(void)b;assert(lock&&!bus);bus=1;}
static void furi_hal_bus_disable(int b){(void)b;assert(lock&&bus);bus=0;regs.CR=0;}
static void furi_hal_bt_lock_core2(void){assert(lock&&!core2);core2=1;}
static void furi_hal_bt_unlock_core2(void){assert(core2);core2=0;}
static bool furi_hal_bt_is_alive(void){return bt_alive;}
static void crypto_key_init(uint32_t* key,uint32_t* iv){(void)key;(void)iv;assert(bus&&lock&&core2);}
static int SHCI_C2_FUS_LoadUsrKey(uint8_t slot){assert(slot&&bus&&lock&&core2&&bt_alive);return !fus_ok;}
static int SHCI_C2_FUS_UnloadUsrKey(uint8_t slot){assert(slot&&bus&&lock&&core2&&bt_alive);return !fus_ok;}
static int SHCI_C2_FUS_StoreUsrKey(SHCI_C2_FUS_StoreUsrKey_Cmd_Param_t* p,uint8_t* slot){assert(lock&&bt_alive&&p->KeySize==32);*slot=11;return !fus_ok;}
'''
main = r'''
static void idle(void){assert(!lock&&!bus&&!core2);}
int main(void){
    uint32_t iv[4]={0};uint8_t data[32]={0}, slot=0;
    FuriHalCryptoKey key={FuriHalCryptoKeyTypeSimple,FuriHalCryptoKeySize256,data};
    for(int alive=0;alive<2;++alive) for(int ok=0;ok<2;++ok){
        bt_alive=alive;fus_ok=ok;
        assert(furi_hal_crypto_enclave_store_key(&key,&slot)==(alive&&ok));idle();
        bool loaded=furi_hal_crypto_enclave_load_key(11,(uint8_t*)iv);
        assert(loaded==(alive&&ok));
        if(!loaded) idle();
        else {assert(lock&&bus&&!core2);assert(furi_hal_crypto_enclave_unload_key(11));idle();}
    }
    for(int alive=0;alive<2;++alive)for(int ok=0;ok<2;++ok){
        bt_alive=fus_ok=1;assert(furi_hal_crypto_enclave_load_key(11,(uint8_t*)iv));
        bt_alive=alive;fus_ok=ok;
        assert(furi_hal_crypto_enclave_unload_key(11)==(alive&&ok));idle();
        /* Subsequent independent AES operation must be able to acquire the lock. */
        assert(furi_mutex_acquire(furi_hal_crypto_mutex,0)==0);
        furi_hal_bus_enable(0);furi_hal_bus_disable(0);assert(furi_mutex_release(furi_hal_crypto_mutex)==0);
    }
    puts("Enclave cleanup: 4 store, 4 load and 4 unload fault combinations PASS");
}
'''
if __name__ == '__main__':
    with tempfile.TemporaryDirectory() as tmp:
        p=Path(tmp); c=p/'test.c'; exe=p/'test'; c.write_text(prelude+functions+main)
        subprocess.run(['gcc','-std=c11','-O2','-Wall','-Wextra','-Werror','-fsanitize=address,undefined',
                        '-fno-sanitize-recover=all','-no-pie','-I',str(ROOT/'targets/furi_hal_include'),str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
