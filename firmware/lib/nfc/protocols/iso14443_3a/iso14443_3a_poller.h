#pragma once

#include "iso14443_3a.h"
#include <lib/nfc/nfc.h>

#include <nfc/nfc_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Iso14443_3aPoller Iso14443_3aPoller;

typedef enum {
    Iso14443_3aPollerEventTypeError, /* Erreur pendant l’activation */
    Iso14443_3aPollerEventTypeReady,
} Iso14443_3aPollerEventType;

typedef union {
    Iso14443_3aError error; /* Cause de l’échec d’activation */
} Iso14443_3aPollerEventData;

typedef struct {
    Iso14443_3aPollerEventType type; /* Type d’événement émis */
    Iso14443_3aPollerEventData* data; /* Données propres à l’événement */
} Iso14443_3aPollerEvent;

/* Usage uniquement dans callback ; fwt en cycles carrier */
Iso14443_3aError iso14443_3a_poller_txrx(
    Iso14443_3aPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Usage uniquement dans callback ; fwt en cycles carrier */
Iso14443_3aError iso14443_3a_poller_send_standard_frame(
    Iso14443_3aPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Usage uniquement dans callback ; fwt en cycles carrier */
/* TX : parité fournie ; RX : données et parité reçues */
Iso14443_3aError iso14443_3a_poller_txrx_custom_parity(
    Iso14443_3aPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Usage uniquement dans callback ; ErrorNone si carte présente */
Iso14443_3aError iso14443_3a_poller_check_presence(Iso14443_3aPoller* instance);

/* Usage uniquement dans callback ; données ISO14443-3A si succès */
Iso14443_3aError
    iso14443_3a_poller_activate(Iso14443_3aPoller* instance, Iso14443_3aData* iso14443_3a_data);

/* Usage uniquement dans callback ; carte Halt, poller Idle */
Iso14443_3aError iso14443_3a_poller_halt(Iso14443_3aPoller* instance);

#ifdef __cplusplus
}
#endif
