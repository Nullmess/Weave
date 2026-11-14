/* Plugin : fichier C unique dans supported_cards */
/* parse obligatoire ; verify/read selon besoins */
/* Enregistrer dans application.fam NFC pour déploiement automatique */
/* APPID obligatoirement suffixé _parser */

#pragma once

#include <furi/core/string.h>

#include <nfc/nfc.h>
#include <nfc/nfc_device.h>

#define NFC_SUPPORTED_CARD_PLUGIN_APP_ID "NfcSupportedCardPlugin"

/**
 * @brief Currently supported plugin API version.
 */
#define NFC_SUPPORTED_CARD_PLUGIN_API_VERSION 1

/* Vérification rapide optionnelle du type carte */

typedef bool (*NfcSupportedCardPluginVerify)(Nfc* nfc);

/* read custom si clés/ordre opérations hors poller standard */

typedef bool (*NfcSupportedCardPluginRead)(Nfc* nfc, NfcDevice* device);

/* Données complètes disponibles via device avant parsing */
/* Rendu libre ; lignes séparées par LF */

typedef bool (*NfcSupportedCardPluginParse)(const NfcDevice* device, FuriString* parsed_data);

/* parse seul suffit pour plugin minimal */

typedef struct {
    NfcProtocol protocol;
    NfcSupportedCardPluginVerify verify;
    NfcSupportedCardPluginRead read;
    NfcSupportedCardPluginParse parse;
} NfcSupportedCardsPlugin;
