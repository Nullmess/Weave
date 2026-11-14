#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi_ble/profile_interface.h>
#include <core/common_defines.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_BT "bt"

typedef struct Bt Bt;

typedef enum {
    BtStatusUnavailable,
    BtStatusOff,
    BtStatusAdvertising,
    BtStatusConnected,
} BtStatus;

typedef void (*BtStatusChangedCallback)(BtStatus status, void* context);

/* Changement profil redémarre CPU2 ; params NULL admis */

FURI_WARN_UNUSED FuriHalBleProfileBase* bt_profile_start(
    Bt* bt,
    const FuriHalBleProfileTemplate* profile_template,
    FuriHalBleProfileParams params);

/* Restauration profil par défaut redémarre CPU2 */

bool bt_profile_restore_default(Bt* bt);

void bt_disconnect(Bt* bt);

void bt_set_status_changed_callback(Bt* bt, BtStatusChangedCallback callback, void* context);

/* Forget devices : wipe clés BLE et suppression bt.keys */

void bt_forget_bonded_devices(Bt* bt);

void bt_keys_storage_set_storage_path(Bt* bt, const char* keys_storage_path);

void bt_keys_storage_set_default_path(Bt* bt);

#ifdef __cplusplus
}
#endif
