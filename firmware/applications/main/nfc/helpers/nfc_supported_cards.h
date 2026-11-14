

#pragma once

#include <core/string.h>

#include <nfc/nfc.h>
#include <nfc/nfc_device.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NfcSupportedCards NfcSupportedCards;

NfcSupportedCards* nfc_supported_cards_alloc(void);

void nfc_supported_cards_free(NfcSupportedCards* instance);

/* Charger cache plugins avant read/parse */

void nfc_supported_cards_load_cache(NfcSupportedCards* instance);

/* Plugins compatibles essayés successivement ; arrêt au premier succès lecture */

bool nfc_supported_cards_read(NfcSupportedCards* instance, NfcDevice* device, Nfc* nfc);

/* Plugins compatibles essayés successivement ; arrêt au premier succès parsing */

bool nfc_supported_cards_parse(
    NfcSupportedCards* instance,
    NfcDevice* device,
    FuriString* parsed_data);

#ifdef __cplusplus
}
#endif
