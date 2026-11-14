/* Header interne exclu des API publiques */

#pragma once

#include "nfc_generic_event.h"
#include "nfc_device_base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* base_poller : Nfc à la racine ; poller parent sinon */

typedef NfcGenericInstance* (*NfcPollerAlloc)(NfcGenericInstance* base_poller);

typedef void (*NfcPollerFree)(NfcGenericInstance* instance);

typedef void (
    *NfcPollerSetCallback)(NfcGenericInstance* poller, NfcGenericCallback callback, void* context);

/* Appelé automatiquement par parent poller via hiérarchie NFC */
/* Propagation des événements Nfc vers pollers parents puis enfants */
/* Retour commande à exécuter par le parent */
typedef NfcCommand (*NfcPollerRun)(NfcGenericEvent event, void* context);

/* Détection non ambiguë appelée automatiquement par NfcPoller */

typedef bool (*NfcPollerDetect)(NfcGenericEvent event, void* context);

typedef const NfcDeviceData* (*NfcPollerGetData)(const NfcGenericInstance* instance);

/* Chaque protocole doit fournir ses implémentations */

typedef struct {
    NfcPollerAlloc alloc;
    NfcPollerFree free;
    NfcPollerSetCallback set_callback;
    NfcPollerRun run;
    NfcPollerDetect detect;
    NfcPollerGetData get_data;
} NfcPollerBase;

#ifdef __cplusplus
}
#endif
