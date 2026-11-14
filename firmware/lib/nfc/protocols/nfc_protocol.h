/* Vérifier support hardware NfcTech avant création d’un protocole */
/* Protocole basé sur un existant : enfant ; sinon protocole base */
/* Headers publics : détails privés dans *_i.h uniquement */
/* Préférer données opaques ; implémenter interface NfcDevice */
/* Enfant propriétaire des données parent : création/libération requises */
/* Poller base reçoit Nfc ; enfant reçoit poller parent à l’allocation */
/* Pollers implémentent NfcPollerBase */
/* Listener optionnel : données empruntées, jamais libérées par lui */
/* Sync API optionnelle, bloquante, crée instances/callbacks automatiquement */
/* Enregistrement enum dans nfc_protocol.h */
/* Liens parent/enfants dans nfc_protocol.c et nfc_protocol_nodes[] */
/* Device defs dans nfc_device_defs.c, poller defs dans nfc_poller_defs.c */
/* Listener optionnel : defs enregistrées dans nfc_listener_defs.c */
/* Exporter headers protocole/poller/listener/sync dans SDK_HEADERS */
/* Intégration nfc_protocol_support obligatoire ; sinon crash lecture */
/* Plugin carte optionnel via nfc_supported_card_plugin.h */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Ajouter tout nouveau protocole avant NfcProtocolNum */

typedef enum {
    NfcProtocolIso14443_3a,
    NfcProtocolIso14443_3b,
    NfcProtocolIso14443_4a,
    NfcProtocolIso14443_4b,
    NfcProtocolIso15693_3,
    NfcProtocolFelica,
    NfcProtocolMfUltralight,
    NfcProtocolMfClassic,
    NfcProtocolMfPlus,
    NfcProtocolMfDesfire,
    NfcProtocolSlix,
    NfcProtocolSt25tb,
    NfcProtocolNtag4xx,
    NfcProtocolType4Tag,
    /* Add new protocols here */

    NfcProtocolNum, /* Nombre de protocoles disponibles */

    NfcProtocolInvalid, /**< Special value representing an invalid state. */
} NfcProtocol;

/* Sans parent : NfcProtocolInvalid */

NfcProtocol nfc_protocol_get_parent(NfcProtocol protocol);

/* Parcourt toute la hiérarchie, contrairement à get_parent */

bool nfc_protocol_has_parent(NfcProtocol protocol, NfcProtocol parent_protocol);

#ifdef __cplusplus
}
#endif
