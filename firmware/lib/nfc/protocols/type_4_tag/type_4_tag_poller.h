#pragma once

#include "type_4_tag.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Type4TagPoller Type4TagPoller;

typedef enum {
    Type4TagPollerEventTypeRequestMode,
    Type4TagPollerEventTypeReadSuccess,
    Type4TagPollerEventTypeReadFailed,
    Type4TagPollerEventTypeWriteSuccess, /**< Poller wrote card successfully. */
    Type4TagPollerEventTypeWriteFailed,
} Type4TagPollerEventType;

typedef enum {
    Type4TagPollerModeRead, /* Mode défaut : lecture seule */
    Type4TagPollerModeWrite, /* Écrire carte sauvegardée vers autre carte */
} Type4TagPollerMode;

/* À remplir sur event RequestMode */

typedef struct {
    Type4TagPollerMode mode; /* Mode du poller */
    const Type4TagData* data; /* Données du poller */
} Type4TagPollerEventDataRequestMode;

typedef union {
    Type4TagError error;
    Type4TagPollerEventDataRequestMode poller_mode; /**< Poller mode context. */
} Type4TagPollerEventData;

typedef struct {
    Type4TagPollerEventType type; /* Type d’événement émis */
    Type4TagPollerEventData* data; /* Données propres à l’événement */
} Type4TagPollerEvent;

#ifdef __cplusplus
}
#endif
