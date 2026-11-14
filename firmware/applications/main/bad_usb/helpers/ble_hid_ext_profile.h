#pragma once

#include <ble_profile/extra_profiles/hid_profile.h>

/* Paramètres profil optionnels via FuriHalBleProfileParams */

typedef struct {
    char name[FURI_HAL_BT_ADV_NAME_LENGTH]; /**< Full device name */
    uint8_t mac[GAP_MAC_ADDR_SIZE]; /**< Full device address */
    bool bonding;
    GapPairing pairing; /**< Pairing security method */
} BleProfileHidExtParams;

/** Hid Keyboard Profile descriptor */
extern const FuriHalBleProfileTemplate* ble_profile_hid_ext;
