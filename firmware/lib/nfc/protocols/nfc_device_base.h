

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    NfcDeviceNameTypeFull, /**< Display full(verbose) name. */
    NfcDeviceNameTypeShort, /**< Display shortened name. */
} NfcDeviceNameType;

/* Type opaque des données NFC spécifiques au protocole */

typedef void NfcDeviceData;

#ifdef __cplusplus
}
#endif
