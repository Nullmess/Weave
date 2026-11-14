#pragma once

#include "mf_plus.h"

#include <lib/nfc/protocols/iso14443_4a/iso14443_4a.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief MIFARE Plus poller opaque type definition.
 */
typedef struct MfPlusPoller MfPlusPoller;

typedef enum {
    MfPlusPollerEventTypeReadSuccess,
    MfPlusPollerEventTypeReadFailed, /* Échec lecture carte */
} MfPlusPollerEventType;

/**
 * @brief MIFARE Plus poller event data.
 */
typedef union {
    MfPlusError error;
} MfPlusPollerEventData;

typedef struct {
    MfPlusPollerEventType type;
    MfPlusPollerEventData* data; /* Données propres à l’événement */
} MfPlusPollerEvent;

/* Usage uniquement dans le callback */
MfPlusError mf_plus_poller_read_version(MfPlusPoller* instance, MfPlusVersion* data);

#ifdef __cplusplus
}
#endif
