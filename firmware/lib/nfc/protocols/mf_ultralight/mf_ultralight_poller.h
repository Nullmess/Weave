#pragma once

#include "mf_ultralight.h"
#include <lib/nfc/protocols/iso14443_3a/iso14443_3a_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MfUltralightPoller MfUltralightPoller;

typedef enum {
    MfUltralightPollerEventTypeRequestMode,
    MfUltralightPollerEventTypeAuthRequest,
    MfUltralightPollerEventTypeAuthSuccess, /**< Authentication succeeded. */
    MfUltralightPollerEventTypeAuthFailed, /**< Authentication failed. */
    MfUltralightPollerEventTypeReadSuccess, /**< Poller read card successfully. */
    MfUltralightPollerEventTypeReadFailed,
    MfUltralightPollerEventTypeRequestWriteData,
    MfUltralightPollerEventTypeCardMismatch, /* Type carte présenté incompatible avec écriture */
    MfUltralightPollerEventTypeCardLocked, /* Carte verrouillée par password/AUTH0/lock bytes */
    MfUltralightPollerEventTypeWriteSuccess, /**< Poller wrote card successfully. */
    MfUltralightPollerEventTypeWriteFail,
    MfUltralightPollerEventTypeRequestKey,
} MfUltralightPollerEventType;

typedef enum {
    MfUltralightPollerModeRead, /* Mode défaut : lecture seule */
    MfUltralightPollerModeWrite, /* Écrire carte sauvegardée vers autre carte */
    MfUltralightPollerModeDictAttack, /* Dictionary attack */
} MfUltralightPollerMode;

/**
 * @brief MfUltralight poller authentication context.
 */
typedef struct {
    MfUltralightAuthPassword password;
    MfUltralightC3DesAuthKey tdes_key;
    MfUltralightAuthPack pack; /* PACK reçu après authentification réussie */
    bool auth_success; /* true si auth réussie */
    bool skip_auth; /* true pour ignorer auth */
} MfUltralightPollerAuthContext;

/**
 * @brief MfUltralight poller key request data.
 */
typedef struct {
    MfUltralightC3DesAuthKey key;
    bool key_provided; /* true si clé fournie ; false arrête attaque */
} MfUltralightPollerKeyRequestData;

typedef union {
    MfUltralightPollerAuthContext auth_context; /**< Authentication context. */
    MfUltralightError error;
    const MfUltralightData* write_data;
    MfUltralightPollerMode poller_mode;
    MfUltralightPollerKeyRequestData key_request_data; /**< Key request data. */
} MfUltralightPollerEventData;

typedef struct {
    MfUltralightPollerEventType type;
    MfUltralightPollerEventData* data; /* Données propres à l’événement */
} MfUltralightPollerEvent;

/* Usage uniquement dans le callback */
MfUltralightError mf_ultralight_poller_auth_pwd(
    MfUltralightPoller* instance,
    MfUltralightPollerAuthContext* data);

/* Usage uniquement dans callback ; RndA généré aléatoirement requis */
MfUltralightError mf_ultralight_poller_authenticate_start(
    MfUltralightPoller* instance,
    const uint8_t* RndA,
    uint8_t* output);

/* Ultralight C : request RndA + RndB déchiffré et décalé */
/* RndB reçu de la carte à l’étape précédente */
/* Réponse : RndA chiffré/décalé, déchiffré ici */

MfUltralightError mf_ultralight_poller_authenticate_end(
    MfUltralightPoller* instance,
    const uint8_t* RndB,
    const uint8_t* request,
    uint8_t* response);

/* Usage uniquement dans callback ; réponse de quatre pages depuis start_page */
MfUltralightError mf_ultralight_poller_read_page(
    MfUltralightPoller* instance,
    uint8_t start_page,
    MfUltralightPageReadCommandData* data);

/* Usage uniquement dans callback ; commande dédiée aux tags NTAGI2C */
MfUltralightError mf_ultralight_poller_read_page_from_sector(
    MfUltralightPoller* instance,
    uint8_t sector,
    uint8_t tag,
    MfUltralightPageReadCommandData* data);

/* Usage uniquement dans le callback */
MfUltralightError mf_ultralight_poller_write_page(
    MfUltralightPoller* instance,
    uint8_t page,
    const MfUltralightPage* data);

/* Usage uniquement dans le callback */
MfUltralightError
    mf_ultralight_poller_read_version(MfUltralightPoller* instance, MfUltralightVersion* data);

/* Usage uniquement dans le callback */
MfUltralightError
    mf_ultralight_poller_read_signature(MfUltralightPoller* instance, MfUltralightSignature* data);

/* Usage uniquement dans le callback */
MfUltralightError mf_ultralight_poller_read_counter(
    MfUltralightPoller* instance,
    uint8_t counter_num,
    MfUltralightCounter* data);

/* Usage uniquement dans le callback */
MfUltralightError mf_ultralight_poller_read_tearing_flag(
    MfUltralightPoller* instance,
    uint8_t tearing_falg_num,
    MfUltralightTearingFlag* data);

#ifdef __cplusplus
}
#endif
