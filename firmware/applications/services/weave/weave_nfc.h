/** Generic NFC ISO14443-4A (ISO-DEP) peripheral transport.
 * The service owns the radio on a separate thread; no FAP callback is retained.
 * Bounded ISO-DEP chaining and WTX, no CID/NAD or reader mode.
 * FAP must poll promptly and respond within the advertised time budget.
 * This is an experimental source interface pending ARM and RF validation. */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WEAVE_NFC_API_VERSION 1U
#define WEAVE_NFC_PACKET_MAX 1024U
#define WEAVE_NFC_RESPONSE_MS 15000U
typedef enum {
    WeaveNfcOk, WeaveNfcInvalid, WeaveNfcDenied, WeaveNfcBusy, WeaveNfcNotOwner,
    WeaveNfcNoData, WeaveNfcExpired, WeaveNfcUnavailable,
} WeaveNfcResult;
typedef struct {
    uint32_t id;
    size_t size;
    size_t response_max;
} WeaveNfcRequest;
/* A random non-unique 4-byte UID is generated per lease. */
WeaveNfcResult weave_nfc_acquire(void* owner);
WeaveNfcResult weave_nfc_release(void* owner);
/* Receive once per request; buffers too small do not consume it. */
WeaveNfcResult weave_nfc_receive(void* owner, WeaveNfcRequest* request, uint8_t* data, size_t capacity);
/* A successful call queues the response; it is not proof of RF delivery. */
WeaveNfcResult weave_nfc_respond(void* owner, uint32_t id, const uint8_t* data, size_t size);
#ifdef __cplusplus
}
#endif
