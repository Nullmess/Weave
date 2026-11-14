#pragma once

#include <lib/nfc/protocols/iso14443_3b/iso14443_3b_poller.h>

#include "iso14443_4b.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Iso14443_4bPoller Iso14443_4bPoller;

typedef enum {
    Iso14443_4bPollerEventTypeError, /* Erreur pendant l’activation */
    Iso14443_4bPollerEventTypeReady,
} Iso14443_4bPollerEventType;

typedef union {
    Iso14443_4bError error; /* Cause de l’échec d’activation */
} Iso14443_4bPollerEventData;

typedef struct {
    Iso14443_4bPollerEventType type; /* Type d’événement émis */
    Iso14443_4bPollerEventData* data; /* Données propres à l’événement */
} Iso14443_4bPollerEvent;

/* Usage uniquement dans callback ; fwt calculé pendant activation */
Iso14443_4bError iso14443_4b_poller_send_block(
    Iso14443_4bPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans callback ; carte Halt, poller Idle */
Iso14443_4bError iso14443_4b_poller_halt(Iso14443_4bPoller* instance);

#ifdef __cplusplus
}
#endif
