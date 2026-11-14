#pragma once

#include "protocol_dallas_base.h"

typedef enum {
    iButtonProtocolDS1990,
    iButtonProtocolDS1992,
    iButtonProtocolDS1996,
    iButtonProtocolDS1971,
    /* Add new 1-Wire protocols here */

    /* Protocole 1-Wire générique par défaut */
    iButtonProtocolDSGeneric,
    iButtonProtocolDSMax,
} iButtonProtocolDallas;

extern const iButtonProtocolDallasBase* const ibutton_protocols_dallas[];
