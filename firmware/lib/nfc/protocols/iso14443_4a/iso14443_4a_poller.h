#pragma once

#include <lib/nfc/protocols/iso14443_3a/iso14443_3a_poller.h>

#include "iso14443_4a.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Iso14443_4aPoller Iso14443_4aPoller;

typedef enum {
    Iso14443_4aPollerEventTypeError, /* Erreur pendant l’activation */
    Iso14443_4aPollerEventTypeReady,
} Iso14443_4aPollerEventType;

typedef union {
    Iso14443_4aError error; /* Cause de l’échec d’activation */
} Iso14443_4aPollerEventData;

typedef struct {
    Iso14443_4aPollerEventType type; /* Type d’événement émis */
    Iso14443_4aPollerEventData* data; /* Données propres à l’événement */
} Iso14443_4aPollerEvent;

/* Usage uniquement dans callback ; fwt calculé pendant activation */
Iso14443_4aError iso14443_4a_poller_send_block(
    Iso14443_4aPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans callback ; fwt calculé pendant activation */
/* Bits PCB modifiés automatiquement puis restaurés */
Iso14443_4aError iso14443_4a_poller_send_chain_block(
    Iso14443_4aPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans callback ; R-block sans données */
/* TX limité à CID puis NAD, deux bytes ; acknowledged true ACK, false NAK */
Iso14443_4aError iso14443_4a_poller_send_receive_ready_block(
    Iso14443_4aPoller* instance,
    bool acknowledged,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans callback ; S-block contrôle */
/* Deselect false : WTX avec INF 1 byte ; true : DESELECT sans INF */
Iso14443_4aError iso14443_4a_poller_send_supervisory_block(
    Iso14443_4aPoller* instance,
    bool deselect,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans callback ; carte Halt, poller Idle */
Iso14443_4aError iso14443_4a_poller_halt(Iso14443_4aPoller* instance);

/* Usage uniquement dans callback ; RATS puis parsing ATS */
Iso14443_4aError
    iso14443_4a_poller_read_ats(Iso14443_4aPoller* instance, Iso14443_4aAtsData* data);

#ifdef __cplusplus
}
#endif
