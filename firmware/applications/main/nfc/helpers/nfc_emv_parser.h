#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <storage/storage.h>

/* Nom AID ; true si trouvé */

bool nfc_emv_parser_get_aid_name(
    Storage* storage,
    uint8_t* aid,
    uint8_t aid_len,
    FuriString* aid_name);

/* Nom du pays ISO 3166 ; true si trouvé */

bool nfc_emv_parser_get_country_name(
    Storage* storage,
    uint16_t country_code,
    FuriString* country_name);

/* Nom de devise ; true si trouvée */

bool nfc_emv_parser_get_currency_name(
    Storage* storage,
    uint16_t currency_code,
    FuriString* currency_name);
