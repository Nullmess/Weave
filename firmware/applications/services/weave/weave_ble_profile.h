#pragma once
#include "weave_ble.h"
#include <furi_ble/profile_interface.h>
extern const FuriHalBleProfileTemplate* const weave_ble_profile;
void weave_ble_profile_gap(GapEvent event);
WeaveBleState weave_ble_profile_state(void);
WeaveBleResult weave_ble_profile_receive(uint32_t connection, uint8_t* characteristic,
    uint8_t* data, size_t capacity, size_t* size);
WeaveBleResult weave_ble_profile_update(uint32_t connection, uint8_t characteristic,
    const uint8_t* data, size_t size);
