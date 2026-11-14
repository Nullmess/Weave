/* Activation/lecture protocolaire ; events remis au callback */

#pragma once

#include <nfc/protocols/nfc_generic_event.h>
#include <nfc/protocols/nfc_device_base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NfcPoller NfcPoller;

/* Event parent : cast selon protocole ; sans parent, origine Nfc */

typedef struct {
    NfcGenericInstance* poller;
    NfcGenericEventData*
        parent_event_data  ;
} NfcGenericEventEx;

/* Callback renvoie la commande à exécuter */

typedef NfcCommand (*NfcGenericCallbackEx)(NfcGenericEventEx event, void* context);

NfcPoller* nfc_poller_alloc(Nfc* nfc, NfcProtocol protocol);

void nfc_poller_free(NfcPoller* instance);

/* Retour callback décide poursuite ou arrêt */

void nfc_poller_start(NfcPoller* instance, NfcGenericCallback callback, void* context);

/* Mode étendu : events parent et instance pour state machine personnalisée */

void nfc_poller_start_ex(NfcPoller* instance, NfcGenericCallbackEx callback, void* context);

/* Arrêt explicite ou via retour callback */

void nfc_poller_stop(NfcPoller* instance);

/* Détection protocolaire appelée automatiquement par NfcScanner */

bool nfc_poller_detect(NfcPoller* instance);

NfcProtocol nfc_poller_get_protocol(const NfcPoller* instance);

const NfcDeviceData* nfc_poller_get_data(const NfcPoller* instance);

#ifdef __cplusplus
}
#endif
