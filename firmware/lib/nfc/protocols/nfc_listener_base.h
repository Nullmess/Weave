/* Header interne exclu des API publiques */

#pragma once

#include "nfc_generic_event.h"
#include "nfc_device_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* base_listener : Nfc à la racine ; listener parent sinon */
/* Données copiées puis modifiables pendant émulation */
/* Données modifiées récupérables via nfc_listener_get_data */

typedef NfcGenericInstance* (
    *NfcListenerAlloc)(NfcGenericInstance* base_listener, NfcDeviceData* data);

typedef void (*NfcListenerFree)(NfcGenericInstance* instance);

typedef void (*NfcListenerSetCallback)(
    NfcGenericInstance* listener,
    NfcGenericCallback callback,
    void* context);

/* Event parent ; context listener concret ; retour commande parent */

typedef NfcCommand (*NfcListenerRun)(NfcGenericEvent event, void* context);

typedef const NfcDeviceData* (*NfcListenerGetData)(const NfcGenericInstance* instance);

/* Chaque protocole doit fournir ses implémentations */

typedef struct {
    NfcListenerAlloc alloc;
    NfcListenerFree free;
    NfcListenerSetCallback set_callback;
    NfcListenerRun run;
    NfcListenerGetData get_data;
} NfcListenerBase;

#ifdef __cplusplus
}
#endif
