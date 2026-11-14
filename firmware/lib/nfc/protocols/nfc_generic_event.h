/* Souscrire via start(callback, context) */

#pragma once

#include "nfc_protocol.h"
#include <nfc/nfc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Cast requis vers Nfc/poller/listener concret */

typedef void NfcGenericInstance;

/* Cast requis vers event protocolaire concret */

typedef void NfcGenericEventData;

/* NfcProtocolInvalid : origine Nfc ; sinon poller/listener concret */
/* event_data nécessite cast selon protocole */

typedef struct {
    NfcProtocol protocol;
    NfcGenericInstance*
        instance;
    NfcGenericEventData* event_data;
} NfcGenericEvent;

/* Event passé par valeur ; callback renvoie commande à exécuter */

typedef NfcCommand (*NfcGenericCallback)(NfcGenericEvent event, void* context);

#ifdef __cplusplus
}
#endif
