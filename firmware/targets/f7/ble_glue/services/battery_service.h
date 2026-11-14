#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* auto_update actif : niveau/charge actualisés depuis power events */

typedef struct BleServiceBattery BleServiceBattery;

BleServiceBattery* ble_svc_battery_start(bool auto_update);

void ble_svc_battery_stop(BleServiceBattery* service);

bool ble_svc_battery_update_level(BleServiceBattery* service, uint8_t battery_level);

bool ble_svc_battery_update_power_state(BleServiceBattery* service, bool charging);

/* Met à jour instances auto_update ; paramètres NULL ignorés */

void ble_svc_battery_state_update(uint8_t* battery_level, bool* charging);

#ifdef __cplusplus
}
#endif
