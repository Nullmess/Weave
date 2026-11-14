#pragma once

#include <storage/storage.h>

#define WEAVE_FACTORY_RESTORE_MARKER EXT_PATH(".weave/restore-default-faps")

/* Marque le boot post-reset sans toucher aux FAP de la SD. */
static inline bool weave_factory_mark_restore(Storage* storage) {
    if(storage_sd_status(storage) != FSE_OK ||
       !storage_simply_mkdir(storage, EXT_PATH(".weave"))) return false;
    File* marker = storage_file_alloc(storage);
    bool ok = storage_file_open(
        marker, WEAVE_FACTORY_RESTORE_MARKER, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) ok = storage_file_close(marker);
    storage_file_free(marker);
    return ok;
}
