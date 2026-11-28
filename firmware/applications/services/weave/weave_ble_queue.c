#include "weave_ble_queue.h"
#include <string.h>
static bool uuid_valid(const WeaveBleUuid* uuid) {
    if(uuid->size != 2 && uuid->size != 16) return false;
    uint8_t nonzero = 0;
    for(size_t i = 0; i < uuid->size; i++) nonzero |= uuid->bytes[i];
    return nonzero != 0;
}
bool weave_ble_config_valid(const WeaveBleConfig* config) {
    if(!config || !uuid_valid(&config->service) || !config->count ||
       config->count > WEAVE_BLE_CHAR_MAX) return false;
    for(size_t i = 0; i < config->count; i++) {
        const WeaveBleCharacteristic* c = &config->characteristics[i];
        if(!uuid_valid(&c->uuid) || !c->max_size || c->max_size > WEAVE_BLE_PACKET_MAX ||
           ((c->properties & WEAVE_BLE_READ) && c->properties != WEAVE_BLE_READ) ||
           ((c->properties & WEAVE_BLE_NOTIFY) && (c->properties & WEAVE_BLE_INDICATE)) ||
           !c->properties || (c->properties & ~(WEAVE_BLE_READ | WEAVE_BLE_WRITE |
             WEAVE_BLE_NOTIFY | WEAVE_BLE_WRITE_COMMAND | WEAVE_BLE_INDICATE))) return false;
        for(size_t j = 0; j < i; j++) {
            const WeaveBleUuid* prev = &config->characteristics[j].uuid;
            if(prev->size == c->uuid.size && !memcmp(prev->bytes, c->uuid.bytes, prev->size))
                return false;
        }
    }
    return true;
}
static void clear_packets(WeaveBleQueue* q) {
    volatile uint8_t* p = (volatile uint8_t*)q->queue;
    for(size_t n = sizeof(q->queue); n; n--) *p++ = 0;
    q->count = 0;
}
void weave_ble_queue_connection(WeaveBleQueue* q, bool connected) {
    clear_packets(q);
    q->overflow = false;
    if(++q->state.connection == 0) ++q->state.connection;
    q->state.connected = connected;
}
void weave_ble_queue_push(WeaveBleQueue* q, uint8_t index, const uint8_t* data, size_t size) {
    if(!q->state.connected || q->overflow) return;
    if(index >= WEAVE_BLE_CHAR_MAX || !data || !size || size > WEAVE_BLE_PACKET_MAX ||
       q->count >= WEAVE_BLE_QUEUE_MAX) {
        clear_packets(q);
        q->overflow = true;
        return;
    }
    WeaveBlePacket* p = &q->queue[q->count++];
    p->index = index; p->size = size; memcpy(p->data, data, size);
}
WeaveBleResult weave_ble_queue_pop(WeaveBleQueue* q, uint32_t generation, uint8_t* index,
    uint8_t* data, size_t capacity, size_t* size) {
    if(!size || !data || !index) return WeaveBleInvalid;
    *size = 0;
    if(!q->state.connected || generation != q->state.connection) return WeaveBleDisconnected;
    if(q->overflow) { q->overflow = false; return WeaveBleOverflow; }
    if(!q->count) return WeaveBleNoData;
    const WeaveBlePacket* p = &q->queue[0];
    if(capacity < p->size) return WeaveBleInvalid;
    *index = p->index; *size = p->size; memcpy(data, p->data, p->size);
    memmove(q->queue, q->queue + 1, (q->count - 1U) * sizeof(*p));
    volatile uint8_t* end = (volatile uint8_t*)&q->queue[--q->count];
    for(size_t n = sizeof(*p); n; n--) *end++ = 0;
    return WeaveBleOk;
}
