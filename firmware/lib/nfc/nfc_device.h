/* Conteneur de données protocolaires ; aucun traitement signal */

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "protocols/nfc_device_base.h"
#include "protocols/nfc_protocol.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NfcDevice NfcDevice;

/* Load/save callback : state true si succès, false sinon */
typedef void (*NfcLoadingCallback)(void* context, bool state);

/* Instance initialement invalide, sans données */

NfcDevice* nfc_device_alloc(void);

void nfc_device_free(NfcDevice* instance);

/* Efface les données ; instance devient invalide */

void nfc_device_clear(NfcDevice* instance);

/* Reset protocolaire ; instance reste valide */

void nfc_device_reset(NfcDevice* instance);

/* Instance invalide : NfcProtocolInvalid */

NfcProtocol nfc_device_get_protocol(const NfcDevice* instance);

/* Protocole requis dans la hiérarchie ; sinon crash */
/* Renvoie les données du niveau demandé */

const NfcDeviceData* nfc_device_get_data(const NfcDevice* instance, NfcProtocol protocol);

/* String statique, indépendante de l’instance */

const char* nfc_device_get_protocol_name(NfcProtocol protocol);

/* String statique ; nom dépend de l’état et name_type */

const char* nfc_device_get_name(const NfcDevice* instance, NfcDeviceNameType name_type);

/* Longueur UID dépend du protocole */

const uint8_t* nfc_device_get_uid(const NfcDevice* instance, size_t* uid_len);

/* Longueur UID compatible protocole requise ; false sinon */

bool nfc_device_set_uid(NfcDevice* instance, const uint8_t* uid, size_t uid_len);

/* Supprime toutes les données précédentes */

void nfc_device_set_data(
    NfcDevice* instance,
    NfcProtocol protocol,
    const NfcDeviceData* protocol_data);

/* Protocole exactement identique requis ; sinon crash */

void nfc_device_copy_data(
    const NfcDevice* instance,
    NfcProtocol protocol,
    NfcDeviceData* protocol_data);

/* true uniquement si protocole et données correspondent */

bool nfc_device_is_equal_data(
    const NfcDevice* instance,
    NfcProtocol protocol,
    const NfcDeviceData* protocol_data);

bool nfc_device_is_equal(const NfcDevice* instance, const NfcDevice* other);

/* Callback appelé à la fin du chargement */

void nfc_device_set_loading_callback(
    NfcDevice* instance,
    NfcLoadingCallback callback,
    void* context);

/* Chemin complet requis ; true si sauvegarde réussie */

bool nfc_device_save(NfcDevice* instance, const char* path);

/* Chemin complet requis ; true si chargement réussi */

bool nfc_device_load(NfcDevice* instance, const char* path);

#ifdef __cplusplus
}
#endif
