#pragma once

#include "felica.h"
#include <lib/nfc/nfc.h>

#include <nfc/nfc_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FelicaPoller FelicaPoller;

typedef enum {
    FelicaPollerEventTypeError, /* Erreur pendant l’activation */
    FelicaPollerEventTypeReady, /* Carte activée et entièrement lue */
    FelicaPollerEventTypeIncomplete, /* Carte activée et partiellement lue */
    FelicaPollerEventTypeRequestAuthContext, /* Contexte authentification demandé par poller */
} FelicaPollerEventType;

typedef union {
    FelicaError error; /* Cause de l’échec d’activation */
    FelicaAuthenticationContext* auth_context; /* Contexte authentification à remplir par utilisateur */
} FelicaPollerEventData;

typedef struct {
    FelicaPollerEventType type; /* Type d’événement émis */
    FelicaPollerEventData* data; /* Données propres à l’événement */
} FelicaPollerEvent;

/* Usage uniquement dans callback ; données FeliCa remplies si succès */
FelicaError felica_poller_activate(FelicaPoller* instance, FelicaData* data);

/* Indexes blocs selon Felica ; FelicaErrorNone si succès */

FelicaError felica_poller_read_blocks(
    FelicaPoller* instance,
    const uint8_t block_count,
    const uint8_t* const block_numbers,
    uint16_t service_code,
    FelicaPollerReadCommandResponse** const response_ptr);

#ifdef __cplusplus
}
#endif
