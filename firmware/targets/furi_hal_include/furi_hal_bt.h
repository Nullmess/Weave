
#pragma once

#include <furi.h>
#include <stdbool.h>
#include <gap.h>
#include <extra_beacon.h>
#include <furi_ble/profile_interface.h>
#include <ble_glue.h>
#include <ble_app.h>
#include <stdint.h>

#define FURI_HAL_BT_STACK_VERSION_MAJOR (1)
#define FURI_HAL_BT_STACK_VERSION_MINOR (12)
#define FURI_HAL_BT_C2_START_TIMEOUT    (1000)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriHalBtStackUnknown,
    FuriHalBtStackLight,
    FuriHalBtStackFull,
} FuriHalBtStack;

void furi_hal_bt_init(void);

/* Verrouillage des transitions d’état core2 */
void furi_hal_bt_lock_core2(void);

/* Verrouillage des transitions d’état core2 */
void furi_hal_bt_unlock_core2(void);

/* Retour true si démarrage radio réussi */
bool furi_hal_bt_start_radio_stack(void);

FuriHalBtStack furi_hal_bt_get_radio_stack(void);

bool furi_hal_bt_is_gatt_gap_supported(void);

bool furi_hal_bt_is_testing_supported(void);

/* Profile NULL : utilisation du profile courant */
bool furi_hal_bt_check_profile_type(
    FuriHalBleProfileBase* profile,
    const FuriHalBleProfileTemplate* profile_template);

/* Params peut être NULL ; retour NULL si échec */
FURI_WARN_UNUSED FuriHalBleProfileBase* furi_hal_bt_start_app(
    const FuriHalBleProfileTemplate* profile_template,
    FuriHalBleProfileParams params,
    const GapRootSecurityKeys* root_keys,
    GapEventCallback event_cb,
    void* context);

/* Réinitialisation core2, également préparatoire aux modes stop */
void furi_hal_bt_reinit(void);

/* Redémarrage core2 ; params NULL permis ; retour NULL si échec */
FURI_WARN_UNUSED FuriHalBleProfileBase* furi_hal_bt_change_app(
    const FuriHalBleProfileTemplate* profile_template,
    FuriHalBleProfileParams profile_params,
    const GapRootSecurityKeys* root_keys,
    GapEventCallback event_cb,
    void* context);

void furi_hal_bt_update_battery_level(uint8_t battery_level);

void furi_hal_bt_update_power_state(bool charging);

/* Actif si connecté ou advertising */
bool furi_hal_bt_is_active(void);

void furi_hal_bt_start_advertising(void);

void furi_hal_bt_stop_advertising(void);

void furi_hal_bt_dump_state(FuriString* buffer);

/* Retour true si core2 vivant */
bool furi_hal_bt_is_alive(void);

void furi_hal_bt_get_key_storage_buff(uint8_t** key_buff_addr, uint16_t* key_buff_size);

/* Acquisition obligatoire avant accès SRAM2 */
void furi_hal_bt_nvm_sram_sem_acquire(void);

/* Release obligatoire après accès SRAM2 */
void furi_hal_bt_nvm_sram_sem_release(void);

/* Retour true si succès */
bool furi_hal_bt_clear_white_list(void);

void furi_hal_bt_set_key_storage_change_callback(
    BleGlueKeyStorageChangedCallback callback,
    void* context);

void furi_hal_bt_start_tone_tx(uint8_t channel, uint8_t power);

void furi_hal_bt_stop_tone_tx(void);

void furi_hal_bt_start_packet_tx(uint8_t channel, uint8_t pattern, uint8_t datarate);

/* Retour nombre de paquets émis */
uint16_t furi_hal_bt_stop_packet_test(void);

void furi_hal_bt_start_packet_rx(uint8_t channel, uint8_t datarate);

void furi_hal_bt_start_rx(uint8_t channel);

void furi_hal_bt_stop_rx(void);

/* RSSI en dBm */
float furi_hal_bt_get_rssi(void);

uint32_t furi_hal_bt_get_transmitted_packets(void);

bool furi_hal_bt_ensure_c2_mode(BleGlueC2Mode mode);

/* Tous états ; len <= EXTRA_BEACON_MAX_DATA_SIZE */
bool furi_hal_bt_extra_beacon_set_data(const uint8_t* data, uint8_t len);

/* Buffer >= EXTRA_BEACON_MAX_DATA_SIZE ; retour longueur valide */
uint8_t furi_hal_bt_extra_beacon_get_data(uint8_t* data);

/* Retour true si succès */
bool furi_hal_bt_extra_beacon_set_config(const GapExtraBeaconConfig* config);

/* Beacon configuré et arrêté requis avant démarrage */
bool furi_hal_bt_extra_beacon_start(void);

/* Retour true si succès */
bool furi_hal_bt_extra_beacon_stop(void);

bool furi_hal_bt_extra_beacon_is_active(void);

/* Retour NULL si beacon jamais configuré */
const GapExtraBeaconConfig* furi_hal_bt_extra_beacon_get_config(void);

#ifdef __cplusplus
}
#endif
