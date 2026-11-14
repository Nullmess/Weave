#pragma once

#include "mf_desfire.h"

#include <lib/nfc/protocols/iso14443_4a/iso14443_4a_poller.h>
#include <lib/nfc/helpers/nxp_native_command_mode.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MfDesfirePoller MfDesfirePoller;

typedef enum {
    MfDesfirePollerEventTypeReadSuccess,
    MfDesfirePollerEventTypeReadFailed,
} MfDesfirePollerEventType;

typedef union {
    MfDesfireError error;
} MfDesfirePollerEventData;

typedef struct {
    MfDesfirePollerEventType type; /* Type d’événement émis */
    MfDesfirePollerEventData* data; /* Données propres à l’événement */
} MfDesfirePollerEvent;

/* Mode commande appliqué aux échanges suivants */

void mf_desfire_poller_set_command_mode(
    MfDesfirePoller* instance,
    NxpNativeCommandMode command_mode);

/* Usage uniquement dans callback ; échanges chunkés avec timeout FWT */
MfDesfireError mf_desfire_poller_send_chunks(
    MfDesfirePoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/**
  * @warning deprecated, use mf_desfire_poller_send_chunks instead
  */
MfDesfireError mf_desfire_send_chunks(
    MfDesfirePoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_version(MfDesfirePoller* instance, MfDesfireVersion* data);

/* Usage uniquement dans le callback */
MfDesfireError
    mf_desfire_poller_read_free_memory(MfDesfirePoller* instance, MfDesfireFreeMemory* data);

/* Usage uniquement dans le callback */
MfDesfireError
    mf_desfire_poller_read_key_settings(MfDesfirePoller* instance, MfDesfireKeySettings* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_key_version(
    MfDesfirePoller* instance,
    uint8_t key_num,
    MfDesfireKeyVersion* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_key_versions(
    MfDesfirePoller* instance,
    SimpleArray* data,
    uint32_t count);

/* Usage uniquement dans le callback */
MfDesfireError
    mf_desfire_poller_read_application_ids(MfDesfirePoller* instance, SimpleArray* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_select_application(
    MfDesfirePoller* instance,
    const MfDesfireApplicationId* id);

/* Usage uniquement dans callback ; application sélectionnée requise */
MfDesfireError mf_desfire_poller_read_file_ids(MfDesfirePoller* instance, SimpleArray* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_file_settings(
    MfDesfirePoller* instance,
    MfDesfireFileId id,
    MfDesfireFileSettings* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_file_settings_multi(
    MfDesfirePoller* instance,
    const SimpleArray* file_ids,
    SimpleArray* data);

/* Usage uniquement dans callback ; ISO DF ID/name optionnels */
MfDesfireError mf_desfire_poller_create_application(
    MfDesfirePoller* instance,
    const MfDesfireApplicationId* id,
    const MfDesfireKeySettings* key_settings,
    uint16_t iso_df_id,
    const uint8_t* iso_df_name,
    uint8_t iso_df_name_len);

/* Usage uniquement dans callback ; ISO EF ID optionnel */
MfDesfireError mf_desfire_poller_create_file(
    MfDesfirePoller* instance,
    MfDesfireFileId id,
    const MfDesfireFileSettings* data,
    uint16_t iso_ef_id);

/* Usage uniquement dans callback ; offset et size en bytes */
MfDesfireError mf_desfire_poller_read_file_data(
    MfDesfirePoller* instance,
    MfDesfireFileId id,
    uint32_t offset,
    size_t size,
    MfDesfireFileData* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_file_value(
    MfDesfirePoller* instance,
    MfDesfireFileId id,
    MfDesfireFileData* data);

/* Usage uniquement dans callback ; offset et size en bytes */
MfDesfireError mf_desfire_poller_read_file_records(
    MfDesfirePoller* instance,
    MfDesfireFileId id,
    uint32_t offset,
    size_t size,
    MfDesfireFileData* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_file_data_multi(
    MfDesfirePoller* instance,
    const SimpleArray* file_ids,
    const SimpleArray* file_settings,
    SimpleArray* data);

/* Usage uniquement dans callback ; application sélectionnée requise */
MfDesfireError
    mf_desfire_poller_read_application(MfDesfirePoller* instance, MfDesfireApplication* data);

/* Usage uniquement dans le callback */
MfDesfireError mf_desfire_poller_read_applications(
    MfDesfirePoller* instance,
    const SimpleArray* app_ids,
    SimpleArray* data);

#ifdef __cplusplus
}
#endif
