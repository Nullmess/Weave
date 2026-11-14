#pragma once

#include "slix.h"

#include <nfc/nfc_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SlixPoller SlixPoller;

typedef enum {
    SlixPollerEventTypeError, /**< An error occured while reading card. */
    SlixPollerEventTypePrivacyUnlockRequest,
    SlixPollerEventTypeReady,
} SlixPollerEventType;

/**
 * @brief Slix poller privacy unlock context data.
 */
typedef struct {
    SlixPassword password; /**< Privacy password. */
    bool password_set; /* Flag password configuré */
} SlixPollerEventDataPrivacyUnlockContext;

typedef union {
    SlixError error;
    SlixPollerEventDataPrivacyUnlockContext privacy_password; /**< Privacy unlock event context. */
} SlixPollerEventData;

typedef struct {
    SlixPollerEventType type; /* Type d’événement émis */
    SlixPollerEventData* data; /* Données propres à l’événement */
} SlixPollerEvent;

/* Usage uniquement dans callback ; fwt en cycles carrier */
SlixError slix_poller_send_frame(
    SlixPoller* instance,
    const BitBuffer* tx_data,
    BitBuffer* rx_data,
    uint32_t fwt);

/* Usage uniquement dans le callback */
SlixError slix_poller_get_nxp_system_info(SlixPoller* instance, SlixSystemInfo* data);

/* Usage uniquement dans le callback */
SlixError slix_poller_read_signature(SlixPoller* instance, SlixSignature* data);

/* Usage uniquement dans le callback */
SlixError slix_poller_get_random_number(SlixPoller* instance, SlixRandomNumber* data);

/* Usage uniquement dans le callback */
SlixError slix_poller_set_password(
    SlixPoller* instance,
    SlixPasswordType type,
    SlixPassword password,
    SlixRandomNumber random_number);

#ifdef __cplusplus
}
#endif
