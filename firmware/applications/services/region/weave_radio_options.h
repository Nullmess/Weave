#pragma once

#include <storage/storage.h>
#include <string.h>
#include <furi_hal_region.h>
#include <furi_hal_subghz.h>

#define WEAVE_RADIO_CONFIG EXT_PATH(".weave/subghz.radio")
#define WEAVE_RADIO_CONFIG_TMP EXT_PATH(".weave/subghz.radio.tmp")
#define WEAVE_RADIO_CONFIG_BAK EXT_PATH(".weave/subghz.radio.bak")

/* Persistent flags: opt-in, no forced radio transmission. */
static inline void weave_radio_options_apply(bool bypass, bool extended) {
    furi_hal_subghz_set_extended_range(bypass && extended);
    furi_hal_region_set_tx_bypass(bypass);
}

static inline void weave_radio_options_load(Storage* storage) {
    weave_radio_options_apply(false, false);
    /* Recover from a power cut between backing up and replacing the config. */
    if(!storage_common_exists(storage, WEAVE_RADIO_CONFIG) &&
       storage_common_exists(storage, WEAVE_RADIO_CONFIG_BAK))
        storage_common_rename(storage, WEAVE_RADIO_CONFIG_BAK, WEAVE_RADIO_CONFIG);
    File* file = storage_file_alloc(storage);
    uint8_t bytes[6] = {0};
    bool valid = false;
    if(storage_file_open(file, WEAVE_RADIO_CONFIG, FSAM_READ, FSOM_OPEN_EXISTING)) {
        valid = storage_file_read(file, bytes, sizeof(bytes)) == sizeof(bytes) &&
                memcmp(bytes, "WSG1", 4) == 0 && bytes[4] <= 1 && bytes[5] <= 1;
        storage_file_close(file);
    }
    storage_file_free(file);
    if(valid) weave_radio_options_apply(bytes[4] != 0, bytes[5] != 0);
}

static inline bool weave_radio_options_save(Storage* storage, bool bypass, bool extended) {
    if(storage_sd_status(storage) != FSE_OK) return false;
    if(!storage_simply_mkdir(storage, EXT_PATH(".weave")) &&
       !storage_common_exists(storage, EXT_PATH(".weave"))) return false;
    uint8_t bytes[6] = {'W', 'S', 'G', '1', bypass ? 1 : 0, bypass && extended ? 1 : 0};
    File* file = storage_file_alloc(storage);
    bool ok = storage_file_open(file, WEAVE_RADIO_CONFIG_TMP, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) ok = storage_file_write(file, bytes, sizeof(bytes)) == sizeof(bytes);
    if(storage_file_is_open(file)) storage_file_close(file);
    storage_file_free(file);
    if(!ok) {
        storage_common_remove(storage, WEAVE_RADIO_CONFIG_TMP);
        return false;
    }
    const bool had_old = storage_common_exists(storage, WEAVE_RADIO_CONFIG);
    storage_common_remove(storage, WEAVE_RADIO_CONFIG_BAK);
    if(had_old && storage_common_rename(storage, WEAVE_RADIO_CONFIG,
                                       WEAVE_RADIO_CONFIG_BAK) != FSE_OK) return false;
    if(storage_common_rename(storage, WEAVE_RADIO_CONFIG_TMP, WEAVE_RADIO_CONFIG) != FSE_OK) {
        if(had_old) storage_common_rename(storage, WEAVE_RADIO_CONFIG_BAK, WEAVE_RADIO_CONFIG);
        return false;
    }
    storage_common_remove(storage, WEAVE_RADIO_CONFIG_BAK);
    weave_radio_options_apply(bypass, extended);
    return true;
}
