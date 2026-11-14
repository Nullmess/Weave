#pragma once

#include "iso14443_3b.h"
#include <lib/nfc/nfc.h>

#include <nfc/nfc_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Iso14443_3bPoller Iso14443_3bPoller;

typedef enum {
    Iso14443_3bPollerEventTypeError, /* Erreur pendant l’activation */
    Iso14443_3bPollerEventTypeReady,
} Iso14443_3bPollerEventType;

typedef union {
    Iso14443_3bError error; /* Cause de l’échec d’activation */
} Iso14443_3bPollerEventData;

typedef struct {
    Iso14443_3bPollerEventType type; /* Type d’événement émis */
    Iso14443_3bPollerEventData* data; /* Données propres à l’événement */
} Iso14443_3bPollerEvent;

/* Usage uniquement dans callback ; fwt en cycles carrier */
Iso14443_3bError iso14443_3b_poller_send_frame(
    Iso14443_3bPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);

/* Usage uniquement dans callback ; données ISO14443-3B si succès */
Iso14443_3bError iso14443_3b_poller_activate(Iso14443_3bPoller* instance, Iso14443_3bData* data);

/* Usage uniquement dans callback ; carte Halt, poller Idle */
Iso14443_3bError iso14443_3b_poller_halt(Iso14443_3bPoller* instance);

#ifdef __cplusplus
}
#endif
