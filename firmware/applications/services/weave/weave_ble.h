/** Generic peripheral-mode GATT server. One lease, one service, up to four
 * characteristics. UUIDs use BLE little-endian wire order. No FAP callbacks
 * run on the radio thread. Pairing requires MITM authentication; no bonding.
 * Acquisition interrupts the default BLE RPC profile until release. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WEAVE_BLE_API_VERSION 1U
#define WEAVE_BLE_CHAR_MAX 4U
/* Conservative single ATT packet; no long/prepared writes. */
#define WEAVE_BLE_PACKET_MAX 20U
#define WEAVE_BLE_READ 1U
#define WEAVE_BLE_WRITE 2U
#define WEAVE_BLE_NOTIFY 4U
#define WEAVE_BLE_WRITE_COMMAND 8U
#define WEAVE_BLE_INDICATE 16U
typedef struct { uint8_t size; uint8_t bytes[16]; } WeaveBleUuid;
typedef struct { WeaveBleUuid uuid; uint8_t properties; uint8_t max_size; } WeaveBleCharacteristic;
typedef struct {
    WeaveBleUuid service;
    uint8_t count;
    WeaveBleCharacteristic characteristics[WEAVE_BLE_CHAR_MAX];
} WeaveBleConfig;
typedef enum {
    WeaveBleOk, WeaveBleInvalid, WeaveBleDenied, WeaveBleBusy,
    WeaveBleNotOwner, WeaveBleDisconnected, WeaveBleNoData,
    WeaveBleOverflow, WeaveBleUnavailable,
} WeaveBleResult;
typedef struct { uint32_t connection; bool connected; } WeaveBleState;
WeaveBleResult weave_ble_acquire(void* owner, const WeaveBleConfig* config);
WeaveBleResult weave_ble_release(void* owner);
WeaveBleResult weave_ble_state(void* owner, WeaveBleState* state);
/* Returns OVERFLOW once after dropping the queue. Treat that connection as
 * protocol-invalid; reset application state. No truncated packets returned. */
WeaveBleResult weave_ble_receive(void* owner, uint32_t connection, uint8_t* characteristic,
    uint8_t* data, size_t capacity, size_t* size);
/* READ is exclusive and for public static metadata only (controller caches values).
 * For readable values connection=0 is allowed while disconnected. For a
 * connected peer supply its current generation. A successful update is not
 * an indication acknowledgement. */
WeaveBleResult weave_ble_update(void* owner, uint32_t connection, uint8_t characteristic,
    const uint8_t* data, size_t size);
#ifdef __cplusplus
}
#endif
