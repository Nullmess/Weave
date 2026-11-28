#!/usr/bin/env python3
"""Compile actual platform, journal, vault, keystore and USB broker sources.
Furi/storage/USB are host doubles, AES-GCM and Ed25519 use OpenSSL instead of
firmware libraries; does not validate the device HAL or other crypto backends.
"""
from pathlib import Path
import subprocess
import tempfile

HERE = Path(__file__).resolve().parent
SOURCE = HERE.parents[2] / 'applications/services/weave'
ROOT = HERE.parents[2]
HEADERS = {
'furi.h': r'''
#pragma once
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#define furi_check(x) assert(x)
#define FuriWaitForever 0
#define FuriMutexTypeNormal 0
typedef struct { int held; } FuriMutex;
FuriMutex* furi_mutex_alloc(int type);
int furi_mutex_acquire(FuriMutex*, int);
int furi_mutex_release(FuriMutex*);
int32_t furi_kernel_lock(void);
void furi_kernel_unlock(void);
size_t memmgr_heap_get_max_free_block(void);
void* furi_record_open(const char*);
void furi_record_close(const char*);
typedef void* FuriThreadId;
FuriThreadId furi_thread_get_current_id(void);
const char* furi_thread_get_appid(FuriThreadId);
''',
 'dialogs/dialogs.h': r''' 
#pragma once
#include <stdint.h>
#define RECORD_DIALOGS "dialogs"
typedef struct DialogsApp DialogsApp;
typedef struct DialogMessage DialogMessage;
typedef enum { DialogMessageButtonLeft, DialogMessageButtonRight } DialogMessageButton;
typedef enum { AlignCenter, AlignTop } Align;
DialogMessage* dialog_message_alloc(void);
void dialog_message_free(DialogMessage*);
void dialog_message_set_header(DialogMessage*,const char*,uint8_t,uint8_t,Align,Align);
void dialog_message_set_text(DialogMessage*,const char*,uint8_t,uint8_t,Align,Align);
void dialog_message_set_buttons(DialogMessage*,const char*,const char*,const char*);
DialogMessageButton dialog_message_show(DialogsApp*,DialogMessage*);
''',
'usb.h': '#pragma once\ntypedef struct usbd_device usbd_device;\nstruct usb_device_descriptor;\n',
'furi_hal_random.h': '#pragma once\n#include <stdint.h>\nvoid furi_hal_random_fill_buf(uint8_t*, uint32_t);\n',
'storage/storage.h': r'''
#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "filesystem_api_defines.h"
#define EXT_PATH(path) "/ext/" path
#define RECORD_STORAGE "storage"
typedef struct Storage Storage;
File* storage_file_alloc(Storage*);
void storage_file_free(File*);
bool storage_file_open(File*, const char*, FS_AccessMode, FS_OpenMode);
bool storage_file_close(File*);
bool storage_file_is_open(File*);
size_t storage_file_read(File*, void*, size_t);
size_t storage_file_write(File*, const void*, size_t);
bool storage_file_sync(File*);
FS_Error storage_common_stat(Storage*, const char*, FileInfo*);
FS_Error storage_common_remove(Storage*, const char*);
FS_Error storage_common_mkdir(Storage*, const char*);
''',
'mbedtls/gcm.h': r'''
#pragma once
#include <stddef.h>
#include <stdint.h>
#define MBEDTLS_CIPHER_ID_AES 1
#define MBEDTLS_GCM_ENCRYPT 1
typedef struct { uint8_t key[32]; } mbedtls_gcm_context;
void mbedtls_gcm_init(mbedtls_gcm_context*);
void mbedtls_gcm_free(mbedtls_gcm_context*);
int mbedtls_gcm_setkey(mbedtls_gcm_context*, int, const uint8_t*, unsigned);
int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context*, int, size_t, const uint8_t*, size_t,
    const uint8_t*, size_t, const uint8_t*, uint8_t*, size_t, uint8_t*);
int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context*, size_t, const uint8_t*, size_t,
    const uint8_t*, size_t, const uint8_t*, size_t, const uint8_t*, uint8_t*);
''',
}
if __name__ == '__main__':
    with tempfile.TemporaryDirectory(prefix='weave-platform-') as temp:
        p=Path(temp)
        for name, contents in HEADERS.items():
            dest=p/name; dest.parent.mkdir(parents=True,exist_ok=True); dest.write_text(contents)
        files=['weave_platform.c','weave_usb.c','weave_vault_journal.c','weave_security.c',
               'weave_security_kdf.c','weave_keystore.c','weave_permissions.c']
        exe=p/'test'
        subprocess.run(['gcc','-D_POSIX_C_SOURCE=200809L','-std=c11','-O1','-g','-Wall','-Wextra','-Werror',
                        '-fsanitize=address,undefined','-fno-sanitize-recover=all','-no-pie',
                        '-I',str(p),'-I',str(SOURCE),'-I',str(ROOT/'targets/furi_hal_include'),
                        '-I',str(ROOT/'applications/services/storage'),
                        *[str(SOURCE/f) for f in files],str(HERE/'test_platform_v1.c'),'-lcrypto','-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
