#pragma once

#include "iso15693_3.h"

#include <nfc/nfc_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Iso15693_3Poller Iso15693_3Poller;

typedef enum {
    Iso15693_3PollerEventTypeError, /* Erreur pendant l’activation */
    Iso15693_3PollerEventTypeReady,
} Iso15693_3PollerEventType;

typedef union {
    Iso15693_3Error error; /* Cause de l’échec d’activation */
} Iso15693_3PollerEventData;

typedef struct {
    Iso15693_3PollerEventType type; /* Type d’événement émis */
    Iso15693_3PollerEventData* data; /* Données propres à l’événement */
} Iso15693_3PollerEvent;

/* Usage uniquement dans callback ; fwt en cycles carrier */
Iso15693_3Error iso15693_3_poller_send_frame(
    Iso15693_3Poller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Usage uniquement dans callback ; données ISO15693 remplies si succès */
Iso15693_3Error iso15693_3_poller_activate(Iso15693_3Poller* instance, Iso15693_3Data* data);

/* Usage uniquement dans le callback */
Iso15693_3Error iso15693_3_poller_inventory(Iso15693_3Poller* instance, uint8_t* uid);

/* Usage uniquement dans le callback */
Iso15693_3Error
    iso15693_3_poller_get_system_info(Iso15693_3Poller* instance, Iso15693_3SystemInfo* data);

/* Usage uniquement dans le callback */
Iso15693_3Error iso15693_3_poller_read_block(
    Iso15693_3Poller* instance,
    uint8_t* data,
    uint8_t block_number,
    uint8_t block_size);

/* Usage uniquement dans le callback */
Iso15693_3Error iso15693_3_poller_read_blocks(
    Iso15693_3Poller* instance,
    uint8_t* data,
    uint16_t block_count,
    uint8_t block_size);

/* Usage uniquement dans le callback */
Iso15693_3Error iso15693_3_poller_get_blocks_security(
    Iso15693_3Poller* instance,
    uint8_t* data,
    uint16_t block_count);

#ifdef __cplusplus
}
#endif
