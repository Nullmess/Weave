/* Émulation modifie les données selon commandes lecteur */
/* Events protocolaires remis au callback */

#pragma once

#include <nfc/protocols/nfc_generic_event.h>
#include <nfc/protocols/nfc_device_base.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NfcListener NfcListener;

NfcListener* nfc_listener_alloc(Nfc* nfc, NfcProtocol protocol, const NfcDeviceData* data);

void nfc_listener_free(NfcListener* instance);

/* Retour callback décide poursuite ou arrêt */

void nfc_listener_start(NfcListener* instance, NfcGenericCallback callback, void* context);

/* Arrêt explicite ou via retour callback */

void nfc_listener_stop(NfcListener* instance);

NfcProtocol nfc_listener_get_protocol(const NfcListener* instance);

/* Protocole exactement identique requis ; sinon crash */

const NfcDeviceData* nfc_listener_get_data(const NfcListener* instance, NfcProtocol protocol);

#ifdef __cplusplus
}
#endif
