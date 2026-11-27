#include "weave_permissions.h"
#include "weave_security.h"
#include <furi.h>
#include <furi_hal_random.h>
#include <storage/storage.h>
#include <mbedtls/gcm.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define VAULT_DIR EXT_PATH(".weave/vault")
#define VAULT_ITERATIONS 20000U
#define VAULT_MAGIC "WVS1"
#define VAULT_HEADER_SIZE 52U /* magic4 + iter4 + salt16 + nonce12 + tag16 */
#define VAULT_RECORD_MAX (VAULT_HEADER_SIZE + WEAVE_SECRET_MAX_SIZE)
#define VAULT_ID_MAX 48U
#define VAULT_AAD_MAX (2 * VAULT_ID_MAX + 2 + 64)
static FuriMutex* vault_mutex;

void weave_security_init(void) {
    if(!vault_mutex) vault_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(vault_mutex);
}

void weave_security_wipe(void* data, size_t size) {
    if(!data) return;
    volatile uint8_t* p = data;
    while(size--) *p++ = 0;
}

bool weave_security_equal(const uint8_t* a, const uint8_t* b, size_t size) {
    if((!a || !b) && size) return false;
    uint8_t delta = 0;
    for(size_t i = 0; i < size; ++i) delta |= a[i] ^ b[i];
    return delta == 0;
}

bool weave_security_random(uint8_t* out, size_t size) {
    if((!out && size) || size > UINT32_MAX) return false;
    if(size) furi_hal_random_fill_buf(out, (uint32_t)size);
    return true;
}

bool weave_security_aead_encrypt(
    const uint8_t key[32], const uint8_t nonce[12], const uint8_t* aad,
    size_t aad_size, const uint8_t* plaintext, size_t size, uint8_t* ciphertext,
    uint8_t tag[16]) {
    if(!key || !nonce || !tag || (!aad && aad_size) || (!plaintext && size) ||
       (!ciphertext && size) || (size && plaintext == ciphertext)) return false;
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);
    bool ok = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256) == 0 &&
        mbedtls_gcm_crypt_and_tag(&ctx, MBEDTLS_GCM_ENCRYPT, size, nonce, 12,
           aad, aad_size, plaintext, ciphertext, 16, tag) == 0;
    mbedtls_gcm_free(&ctx);
    if(!ok) {
        if(ciphertext) weave_security_wipe(ciphertext, size);
        weave_security_wipe(tag, 16);
    }
    return ok;
}

bool weave_security_aead_decrypt(
    const uint8_t key[32], const uint8_t nonce[12], const uint8_t* aad,
    size_t aad_size, const uint8_t* ciphertext, size_t size,
    const uint8_t tag[16], uint8_t* plaintext) {
    if(!key || !nonce || !tag || (!aad && aad_size) || (!ciphertext && size) ||
       (!plaintext && size) || (size && plaintext == ciphertext)) return false;
    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);
    bool ok = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 256) == 0 &&
        mbedtls_gcm_auth_decrypt(&ctx, size, nonce, 12, aad, aad_size,
                                tag, 16, ciphertext, plaintext) == 0;
    mbedtls_gcm_free(&ctx);
    if(!ok && plaintext) weave_security_wipe(plaintext, size);
    return ok;
}

/* Filename is SHA-256(namespace NUL name), not user-provided path material.
 * The full namespace/name also enter GCM AAD, preventing substitution. */
static bool vault_identifiers(
    const char* ns, const char* name, char path[sizeof(VAULT_DIR) + 68],
    uint8_t aad[VAULT_AAD_MAX], size_t* aad_len) {
    if(!ns || !name) return false;
    /* Furi app ID is assigned by the loader (and inherited by child threads).
     * It is a logical owner identifier, not a hardware-enforced identity. */
    const char* owner = furi_thread_get_appid(furi_thread_get_current_id());
    if(!owner) return false;
    const size_t owner_len = strnlen(owner, 64);
    if(!owner_len || owner_len >= 64 || !strcmp(owner, "unknown") ||
       !strcmp(owner, "driver") || !strcmp(owner, "system")) return false;
    size_t a = strnlen(ns, VAULT_ID_MAX + 1), b = strnlen(name, VAULT_ID_MAX + 1);
    if(!a || !b || a > VAULT_ID_MAX || b > VAULT_ID_MAX) return false;
    memcpy(aad, owner, owner_len);
    aad[owner_len] = 0;
    memcpy(aad + owner_len + 1, ns, a);
    aad[owner_len + a + 1] = 0;
    memcpy(aad + owner_len + a + 2, name, b);
    aad[owner_len + a + b + 2] = 0;
    *aad_len = owner_len + a + b + 3;
    uint8_t digest[32];
    if(!weave_security_sha256(aad, *aad_len, digest)) return false;
    static const char hex[] = "0123456789abcdef";
    size_t dir_len = strlen(VAULT_DIR);
    memcpy(path, VAULT_DIR, dir_len);
    path[dir_len++] = '/';
    for(size_t i = 0; i < 32; ++i) {
        path[dir_len + i * 2] = hex[digest[i] >> 4];
        path[dir_len + i * 2 + 1] = hex[digest[i] & 15];
    }
    path[dir_len + 64] = 0;
    weave_security_wipe(digest, sizeof(digest));
    return true;
}

static void vault_u32_store(uint8_t* p, uint32_t v) {
    p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v;
}
static uint32_t vault_u32_load(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | p[3];
}

WeaveSecurityResult weave_secret_store(
    const char* namespace_id, const char* name, const uint8_t* password,
    size_t password_size, const uint8_t* value, size_t value_size) {
    uint8_t aad[VAULT_AAD_MAX]; size_t aad_len;
    char path[sizeof(VAULT_DIR) + 68];
    if(!vault_mutex || !password || !password_size || (!value && value_size) ||
       value_size > WEAVE_SECRET_MAX_SIZE ||
       !vault_identifiers(namespace_id, name, path, aad, &aad_len)) return WeaveSecurityInvalid;
    if(!weave_permission_check(WeavePermissionVault)) return WeaveSecurityDenied;
    uint8_t* record = malloc(VAULT_HEADER_SIZE + value_size);
    if(!record) return WeaveSecurityStorageError;
    uint8_t key[32];
    memcpy(record, VAULT_MAGIC, 4);
    vault_u32_store(record + 4, VAULT_ITERATIONS);
    bool ok = weave_security_random(record + 8, 28) &&
        weave_security_pbkdf2_sha256(password, password_size, record + 8, 16,
                                    VAULT_ITERATIONS, key, sizeof(key)) &&
        weave_security_aead_encrypt(key, record + 24, aad, aad_len,
                                    value, value_size, record + VAULT_HEADER_SIZE,
                                    record + 36);
    weave_security_wipe(key, sizeof(key));
    if(!ok) {
        weave_security_wipe(record, VAULT_HEADER_SIZE + value_size); free(record);
        return WeaveSecurityCryptoError;
    }
    furi_mutex_acquire(vault_mutex, FuriWaitForever);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_common_mkdir(storage, EXT_PATH(".weave"));
    FS_Error mkdir_result = storage_common_mkdir(storage, VAULT_DIR);
    /* Directory may already exist. Check it rather than trusting FSE_EXIST alone. */
    FileInfo dir_info;
    bool dir_ok = (mkdir_result == FSE_OK || mkdir_result == FSE_EXIST) &&
       storage_common_stat(storage, VAULT_DIR, &dir_info) == FSE_OK &&
       file_info_is_dir(&dir_info);
    char temp[sizeof(path) + 5];
    snprintf(temp, sizeof(temp), "%s.tmp", path);
    File* file = storage_file_alloc(storage);
    ok = dir_ok && storage_file_open(file, temp, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) ok = storage_file_write(file, record, VAULT_HEADER_SIZE + value_size) ==
               VAULT_HEADER_SIZE + value_size;
    if(storage_file_is_open(file)) ok = storage_file_close(file) && ok;
    storage_file_free(file);
    /* Update with rollback on failure. A second same-name writer is serialized. */
    char backup[sizeof(path) + 5];
    snprintf(backup, sizeof(backup), "%s.bak", path);
    bool had_old = false;
    if(ok) {
        FileInfo old_info;
        had_old = storage_common_stat(storage, path, &old_info) == FSE_OK;
        if(had_old) {
            storage_common_remove(storage, backup);
            ok = storage_common_rename(storage, path, backup) == FSE_OK;
        }
    }
    if(ok) {
        ok = storage_common_rename(storage, temp, path) == FSE_OK;
        if(!ok && had_old) storage_common_rename(storage, backup, path);
    }
    if(ok && had_old) storage_common_remove(storage, backup);
    if(!ok) storage_common_remove(storage, temp);
    furi_record_close(RECORD_STORAGE);
    furi_mutex_release(vault_mutex);
    weave_security_wipe(record, VAULT_HEADER_SIZE + value_size);
    free(record);
    return ok ? WeaveSecurityOk : WeaveSecurityStorageError;
}

WeaveSecurityResult weave_secret_load(
    const char* namespace_id, const char* name, const uint8_t* password,
    size_t password_size, uint8_t* output, size_t capacity, size_t* output_size) {
    uint8_t aad[VAULT_AAD_MAX]; size_t aad_len;
    char path[sizeof(VAULT_DIR) + 68];
    if(output_size) *output_size = 0;
    if(!vault_mutex || !password || !password_size || !output_size ||
       (!output && capacity) || !vault_identifiers(namespace_id, name, path, aad, &aad_len))
       return WeaveSecurityInvalid;
    if(!weave_permission_check(WeavePermissionVault)) return WeaveSecurityDenied;
    furi_mutex_acquire(vault_mutex, FuriWaitForever);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FileInfo info;
    FS_Error stat = storage_common_stat(storage, path, &info);
    if(stat != FSE_OK) {
        furi_record_close(RECORD_STORAGE);
        furi_mutex_release(vault_mutex);
        return stat == FSE_NOT_EXIST ? WeaveSecurityNotFound : WeaveSecurityStorageError;
    }
    if(info.size < VAULT_HEADER_SIZE || info.size > VAULT_RECORD_MAX) {
        furi_record_close(RECORD_STORAGE);
        furi_mutex_release(vault_mutex);
        return WeaveSecurityAuthenticationFailed;
    }
    size_t value_size = info.size - VAULT_HEADER_SIZE;
    if(capacity < value_size) {
        *output_size = value_size;
        furi_record_close(RECORD_STORAGE);
        furi_mutex_release(vault_mutex);
        return WeaveSecurityBufferTooSmall;
    }
    uint8_t* record = malloc(info.size);
    if(!record) {
        furi_record_close(RECORD_STORAGE);
        furi_mutex_release(vault_mutex);
        return WeaveSecurityStorageError;
    }
    File* file = storage_file_alloc(storage);
    bool ok = storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING);
    if(ok) ok = storage_file_read(file, record, info.size) == info.size;
    if(storage_file_is_open(file)) ok = storage_file_close(file) && ok;
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    furi_mutex_release(vault_mutex);
    WeaveSecurityResult result = WeaveSecurityStorageError;
    if(ok) {
        result = WeaveSecurityAuthenticationFailed;
        uint32_t iterations = vault_u32_load(record + 4);
        if(memcmp(record, VAULT_MAGIC, 4) == 0 && iterations == VAULT_ITERATIONS) {
            uint8_t key[32];
            if(weave_security_pbkdf2_sha256(password, password_size, record + 8, 16,
                                            iterations, key, sizeof(key)) &&
               weave_security_aead_decrypt(key, record + 24, aad, aad_len,
                   record + VAULT_HEADER_SIZE, value_size, record + 36, output)) {
                *output_size = value_size;
                result = WeaveSecurityOk;
            }
            weave_security_wipe(key, sizeof(key));
        }
    }
    if(result != WeaveSecurityOk && output && capacity) weave_security_wipe(output, capacity);
    weave_security_wipe(record, info.size);
    free(record);
    return result;
}

WeaveSecurityResult weave_secret_delete(const char* namespace_id, const char* name) {
    uint8_t aad[VAULT_AAD_MAX]; size_t aad_len;
    char path[sizeof(VAULT_DIR) + 68];
    if(!vault_mutex || !vault_identifiers(namespace_id, name, path, aad, &aad_len))
        return WeaveSecurityInvalid;
    if(!weave_permission_check(WeavePermissionVault)) return WeaveSecurityDenied;
    furi_mutex_acquire(vault_mutex, FuriWaitForever);
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FS_Error err = storage_common_remove(storage, path);
    furi_record_close(RECORD_STORAGE);
    furi_mutex_release(vault_mutex);
    return err == FSE_OK ? WeaveSecurityOk :
           err == FSE_NOT_EXIST ? WeaveSecurityNotFound : WeaveSecurityStorageError;
}
