/* Header interne exclu des API publiques */

#pragma once

#include "nfc_device_base.h"

#include <flipper_format.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef NfcDeviceData* (*NfcDeviceAlloc)(void);

typedef void (*NfcDeviceFree)(NfcDeviceData* data);

/* Reset défini par protocole */

typedef void (*NfcDeviceReset)(NfcDeviceData* data);

typedef void (*NfcDeviceCopy)(NfcDeviceData* data, const NfcDeviceData* other);

/* Obsolète : interdit dans les nouveaux protocoles */

typedef bool (*NfcDeviceVerify)(NfcDeviceData* data, const FuriString* device_type);

/* FlipperFormat initialisé et ouvert par appelant requis */

typedef bool (*NfcDeviceLoad)(NfcDeviceData* data, FlipperFormat* ff, uint32_t version);

/* FlipperFormat initialisé et ouvert par appelant requis */

typedef bool (*NfcDeviceSave)(const NfcDeviceData* data, FlipperFormat* ff);

typedef bool (*NfcDeviceEqual)(const NfcDeviceData* data, const NfcDeviceData* other);

/* String statique ; nom dépend de l’état et name_type */

typedef const char* (*NfcDeviceGetName)(const NfcDeviceData* data, NfcDeviceNameType name_type);

/* Longueur UID dépend du protocole */

typedef const uint8_t* (*NfcDeviceGetUid)(const NfcDeviceData* data, size_t* uid_len);

/* Longueur UID compatible protocole requise ; false sinon */

typedef bool (*NfcDeviceSetUid)(NfcDeviceData* data, const uint8_t* uid, size_t uid_len);

/* Protocole parent obligatoire */

typedef NfcDeviceData* (*NfcDeviceGetBaseData)(const NfcDeviceData* data);

/* Chaque protocole doit fournir ses implémentations */

typedef struct {
    const char*
        protocol_name; /* Nom protocole en string statique */
    NfcDeviceAlloc alloc;
    NfcDeviceFree free;
    NfcDeviceReset reset;
    NfcDeviceCopy copy;
    NfcDeviceVerify verify; /* Obsolète ; NULL dans les nouveaux protocoles */
    NfcDeviceLoad load;
    NfcDeviceSave save;
    NfcDeviceEqual is_equal;
    NfcDeviceGetName get_name;
    NfcDeviceGetUid get_uid;
    NfcDeviceSetUid set_uid;
    NfcDeviceGetBaseData get_base_data;
} NfcDeviceBase;

#ifdef __cplusplus
}
#endif
