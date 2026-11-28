#include "weave_vault_journal.h"
#include <stdio.h>
#include <string.h>
#include <furi.h>

/* Keep two complete copies. Unlike storage_common_rename (copy then delete),
 * writing the inactive slot never destroys the currently selected record.
 * The trailer covers header+encrypted data, and is written last. No claim of
 * secure anti-rollback or FAT-level atomicity. */
#define JOURNAL_HEADER 16U
#define JOURNAL_DIGEST 32U
#define JOURNAL_MAX (JOURNAL_HEADER + WEAVE_VAULT_RECORD_MAX + JOURNAL_DIGEST)
#define JOURNAL_PATH_MAX 128U

typedef struct { bool valid; uint64_t generation; size_t size; } Slot;
static bool path_suffix(char out[JOURNAL_PATH_MAX], const char* path, const char* suffix) {
    int n = snprintf(out, JOURNAL_PATH_MAX, "%s%s", path, suffix);
    return n > 0 && n < (int)JOURNAL_PATH_MAX;
}
static uint64_t get_be(const uint8_t* p, size_t n) {
    uint64_t v = 0;
    while(n--) v = (v << 8) | *p++;
    return v;
}
static void put_be(uint8_t* p, size_t n, uint64_t v) {
    while(n) { p[--n] = (uint8_t)v; v >>= 8; }
}
static WeaveSecurityResult read_file(Storage* storage, const char* path,
    uint8_t* data, size_t capacity, size_t* size) {
    FileInfo info;
    FS_Error err = storage_common_stat(storage, path, &info);
    if(err == FSE_NOT_EXIST) return WeaveSecurityNotFound;
    if(err != FSE_OK) return WeaveSecurityStorageError;
    if(file_info_is_dir(&info) || info.size > capacity || !info.size)
        return WeaveSecurityAuthenticationFailed;
    File* file = storage_file_alloc(storage);
    if(!file) return WeaveSecurityStorageError;
    bool ok = storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING);
    if(ok) ok = storage_file_read(file, data, (size_t)info.size) == (size_t)info.size;
    if(storage_file_is_open(file)) ok = storage_file_close(file) && ok;
    storage_file_free(file);
    if(!ok) return WeaveSecurityStorageError;
    *size = (size_t)info.size;
    return WeaveSecurityOk;
}
static WeaveSecurityResult inspect(Storage* storage, const char* path,
    uint8_t* buffer, Slot* slot) {
    *slot = (Slot){0};
    size_t n = 0;
    WeaveSecurityResult result = read_file(storage, path, buffer, JOURNAL_MAX, &n);
    if(result != WeaveSecurityOk) return result;
    if(n < JOURNAL_HEADER + 52U + JOURNAL_DIGEST || memcmp(buffer, "WVJ1", 4))
        return WeaveSecurityAuthenticationFailed;
    const uint64_t generation = get_be(buffer + 4, 8);
    const uint64_t size = get_be(buffer + 12, 4);
    if(!generation || size < 52U || size > WEAVE_VAULT_RECORD_MAX ||
       n != JOURNAL_HEADER + size + JOURNAL_DIGEST) return WeaveSecurityAuthenticationFailed;
    uint8_t digest[JOURNAL_DIGEST];
    const bool ok = weave_security_sha256(buffer, n - JOURNAL_DIGEST, digest) &&
        weave_security_equal(digest, buffer + n - JOURNAL_DIGEST, JOURNAL_DIGEST);
    weave_security_wipe(digest, sizeof(digest));
    if(!ok) return WeaveSecurityAuthenticationFailed;
    *slot = (Slot){true, generation, (size_t)size};
    return WeaveSecurityOk;
}
/* All functions run under vault_mutex. Never silently ignore I/O errors. */
static WeaveSecurityResult select_slot(Storage* storage, const char* path, uint8_t* buffer,
    Slot slots[2], unsigned* chosen) {
    char slot_path[JOURNAL_PATH_MAX];
    bool invalid = false;
    for(unsigned i = 0; i < 2; i++) {
        if(!path_suffix(slot_path, path, i ? ".1" : ".0")) return WeaveSecurityInvalid;
        WeaveSecurityResult result = inspect(storage, slot_path, buffer, &slots[i]);
        if(result == WeaveSecurityAuthenticationFailed) invalid = true;
        else if(result != WeaveSecurityOk && result != WeaveSecurityNotFound) return result;
    }
    if(!slots[0].valid && !slots[1].valid)
        return invalid ? WeaveSecurityAuthenticationFailed : WeaveSecurityNotFound;
    if(slots[0].valid && slots[1].valid && slots[0].generation == slots[1].generation)
        return WeaveSecurityAuthenticationFailed;
    *chosen = slots[1].valid && (!slots[0].valid || slots[1].generation > slots[0].generation);
    return WeaveSecurityOk;
}
static WeaveSecurityResult read_legacy(Storage* storage, const char* path,
    uint8_t* record, size_t* size) {
    WeaveSecurityResult result = read_file(storage, path, record, WEAVE_VAULT_RECORD_MAX, size);
    if(result == WeaveSecurityNotFound) {
        char backup[JOURNAL_PATH_MAX];
        if(!path_suffix(backup, path, ".bak")) return WeaveSecurityInvalid;
        result = read_file(storage, backup, record, WEAVE_VAULT_RECORD_MAX, size);
    }
    return result;
}
static uint8_t* journal_alloc(void) {
    /* Furi malloc is fatal on exhaustion. Check largest contiguous block first. */
    const int32_t lock = furi_kernel_lock();
    uint8_t* buffer = NULL;
    if(memmgr_heap_get_max_free_block() >= JOURNAL_MAX + 64U) buffer = malloc(JOURNAL_MAX);
    if(lock == 0) furi_kernel_unlock();
    return buffer;
}
static void journal_free(uint8_t* buffer) {
    weave_security_wipe(buffer, JOURNAL_MAX);
    free(buffer);
}
WeaveSecurityResult weave_vault_read(Storage* storage, const char* path,
    uint8_t* record, size_t* size) {
    uint8_t* buffer = journal_alloc();
    if(!buffer) return WeaveSecurityStorageError;
    Slot slots[2]; unsigned chosen = 0;
    WeaveSecurityResult result = select_slot(storage, path, buffer, slots, &chosen);
    if(result == WeaveSecurityOk) {
        char selected[JOURNAL_PATH_MAX]; Slot check;
        if(!path_suffix(selected, path, chosen ? ".1" : ".0")) result = WeaveSecurityInvalid;
        else result = inspect(storage, selected, buffer, &check);
        if(result == WeaveSecurityOk && check.generation != slots[chosen].generation)
            result = WeaveSecurityStorageError;
        if(result == WeaveSecurityOk) {
            memcpy(record, buffer + JOURNAL_HEADER, check.size);
            *size = check.size;
        }
    } else if(result == WeaveSecurityNotFound ||
              (result == WeaveSecurityAuthenticationFailed && !slots[0].valid && !slots[1].valid)) {
        /* A torn first migration can leave the old WVS1 file intact. */
        WeaveSecurityResult legacy = read_legacy(storage, path, record, size);
        if(legacy != WeaveSecurityNotFound) result = legacy;
    }
    journal_free(buffer);
    return result;
}
static bool remove_if_present(Storage* storage, const char* path) {
    FS_Error result = storage_common_remove(storage, path);
    return result == FSE_OK || result == FSE_NOT_EXIST;
}
WeaveSecurityResult weave_vault_write(Storage* storage, const char* path,
    const uint8_t* record, size_t size, bool create_only) {
    if(size < 52U || size > WEAVE_VAULT_RECORD_MAX) return WeaveSecurityInvalid;
    uint8_t* buffer = journal_alloc();
    if(!buffer) return WeaveSecurityStorageError;
    Slot slots[2]; unsigned chosen = 0;
    WeaveSecurityResult result = select_slot(storage, path, buffer, slots, &chosen);
    uint64_t generation = 1;
    unsigned destination = 0;
    if(result == WeaveSecurityOk) {
        if(create_only || slots[chosen].generation == UINT64_MAX) {
            result = WeaveSecurityInvalid; goto done;
        }
        generation = slots[chosen].generation + 1;
        destination = 1U - chosen;
    } else if(result == WeaveSecurityNotFound ||
              (result == WeaveSecurityAuthenticationFailed && !slots[0].valid && !slots[1].valid)) {
        if(create_only) {
            size_t old_size = 0;
            WeaveSecurityResult legacy = read_legacy(storage, path, buffer, &old_size);
            if(legacy != WeaveSecurityNotFound) { result = WeaveSecurityInvalid; goto done; }
            if(result != WeaveSecurityNotFound) goto done;
        }
    } else goto done;
    char target[JOURNAL_PATH_MAX];
    if(!path_suffix(target, path, destination ? ".1" : ".0")) {
        result = WeaveSecurityInvalid; goto done;
    }
    memcpy(buffer, "WVJ1", 4);
    put_be(buffer + 4, 8, generation);
    put_be(buffer + 12, 4, size);
    memcpy(buffer + JOURNAL_HEADER, record, size);
    size_t length = JOURNAL_HEADER + size;
    if(!weave_security_sha256(buffer, length, buffer + length)) {
        result = WeaveSecurityCryptoError; goto done;
    }
    length += JOURNAL_DIGEST;
    File* file = storage_file_alloc(storage);
    bool ok = file && storage_file_open(file, target, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) ok = storage_file_write(file, buffer, length) == length && storage_file_sync(file);
    if(file) {
        if(storage_file_is_open(file)) ok = storage_file_close(file) && ok;
        storage_file_free(file);
    }
    if(!ok) { result = WeaveSecurityStorageError; goto done; }
    Slot check;
    result = inspect(storage, target, buffer, &check);
    if(result != WeaveSecurityOk || check.generation != generation || check.size != size ||
       !weave_security_equal(record, buffer + JOURNAL_HEADER, size)) {
        result = WeaveSecurityStorageError; goto done;
    }
    /* Best effort legacy cleanup after the new copy is verified. Do not delete
     * the other journal slot. A later delete removes all representations. */
    char old[JOURNAL_PATH_MAX];
    (void)remove_if_present(storage, path);
    if(path_suffix(old, path, ".bak")) (void)remove_if_present(storage, old);
    if(path_suffix(old, path, ".tmp")) (void)remove_if_present(storage, old);
    result = WeaveSecurityOk;
done:
    journal_free(buffer);
    return result;
}
WeaveSecurityResult weave_vault_remove(Storage* storage, const char* path) {
    const char* suffixes[] = {".tmp", ".bak", "", ".0", ".1"};
    bool found = false;
    char candidate[JOURNAL_PATH_MAX];
    for(size_t i = 0; i < sizeof(suffixes)/sizeof(suffixes[0]); i++) {
        if(!path_suffix(candidate, path, suffixes[i])) return WeaveSecurityInvalid;
        FS_Error err = storage_common_remove(storage, candidate);
        if(err == FSE_OK) found = true;
        else if(err != FSE_NOT_EXIST) return WeaveSecurityStorageError;
    }
    return found ? WeaveSecurityOk : WeaveSecurityNotFound;
}
