#pragma once
#include "weave_ble.h"
#define WEAVE_BLE_QUEUE_MAX 4U
typedef struct { uint8_t index, size, data[WEAVE_BLE_PACKET_MAX]; } WeaveBlePacket;
typedef struct {
    WeaveBleState state;
    WeaveBlePacket queue[WEAVE_BLE_QUEUE_MAX];
    uint8_t count;
    bool overflow;
} WeaveBleQueue;
/* Pure core. Caller serializes access. */
bool weave_ble_config_valid(const WeaveBleConfig* config);
void weave_ble_queue_connection(WeaveBleQueue* q, bool connected);
void weave_ble_queue_push(WeaveBleQueue* q, uint8_t index, const uint8_t* data, size_t size);
WeaveBleResult weave_ble_queue_pop(WeaveBleQueue* q, uint32_t generation, uint8_t* index,
    uint8_t* data, size_t capacity, size_t* size);
