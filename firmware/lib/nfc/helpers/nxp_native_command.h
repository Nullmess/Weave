#pragma once

#include "nxp_native_command_mode.h"

#include <lib/nfc/protocols/iso14443_4a/iso14443_4a_poller.h>

// ISO 7816 command wrapping
#define NXP_NATIVE_COMMAND_ISO_CLA (0x90)
#define NXP_NATIVE_COMMAND_ISO_P1  (0x00)
#define NXP_NATIVE_COMMAND_ISO_P2  (0x00)
#define NXP_NATIVE_COMMAND_ISO_LE  (0x00)
// ISO 7816 status wrapping
#define NXP_NATIVE_COMMAND_ISO_SW1 (0x91)

// Successful operation
#define NXP_NATIVE_COMMAND_STATUS_OPERATION_OK                (0x00)
/* Backup inchangé : CommitTransaction/AbortTransaction inutiles */
#define NXP_NATIVE_COMMAND_STATUS_NO_CHANGES                  (0x0C)
/* Mémoire NV insuffisante pour commande */
#define NXP_NATIVE_COMMAND_STATUS_OUT_OF_EEPROM_ERROR         (0x0E)
/* Command code non supporté */
#define NXP_NATIVE_COMMAND_STATUS_ILLEGAL_COMMAND_CODE        (0x1C)
/* CRC/MAC incompatible ou padding invalide */
#define NXP_NATIVE_COMMAND_STATUS_INTEGRITY_ERROR             (0x1E)
// Invalid key number specified
#define NXP_NATIVE_COMMAND_STATUS_NO_SUCH_KEY                 (0x40)
/* Longueur commande invalide */
#define NXP_NATIVE_COMMAND_STATUS_LENGTH_ERROR                (0x7E)
/* Configuration/état incompatible avec la commande */
#define NXP_NATIVE_COMMAND_STATUS_PERMISSION_DENIED           (0x9D)
/* Paramètres invalides */
#define NXP_NATIVE_COMMAND_STATUS_PARAMETER_ERROR             (0x9E)
/* AID demandé absent de PICC */
#define NXP_NATIVE_COMMAND_STATUS_APPLICATION_NOT_FOUND       (0xA0)
/* Erreur irrécupérable application : application désactivée */
#define NXP_NATIVE_COMMAND_STATUS_APPL_INTEGRITY_ERROR        (0xA1)
/* Auth actuellement interdite ; attendre délai complet */
#define NXP_NATIVE_COMMAND_STATUS_STATUS_AUTHENTICATION_DELAY (0xAD)
/* Authentification insuffisante pour la commande */
#define NXP_NATIVE_COMMAND_STATUS_AUTHENTICATION_ERROR        (0xAE)
/* Frame data supplémentaire attendue */
#define NXP_NATIVE_COMMAND_STATUS_ADDITIONAL_FRAME            (0xAF)
/* Accès hors limites fichier/record */
/* Limites value file dépassées */
#define NXP_NATIVE_COMMAND_STATUS_BOUNDARY_ERROR              (0xBE)
/* Erreur irrécupérable PICC : PICC désactivée */
#define NXP_NATIVE_COMMAND_STATUS_PICC_INTEGRITY_ERROR        (0xC1)
/* Commande incomplète : frames PCD manquantes */
#define NXP_NATIVE_COMMAND_STATUS_COMMAND_ABORTED             (0xCA)
/* PICC désactivée après erreur irrécupérable */
#define NXP_NATIVE_COMMAND_STATUS_PICC_DISABLED_ERROR         (0xCD)
/* Limite 28 applications ; CreateApplication impossible */
#define NXP_NATIVE_COMMAND_STATUS_COUNT_ERROR                 (0xCE)
/* Fichier/application avec ce numéro déjà présent */
#define NXP_NATIVE_COMMAND_STATUS_DUBLICATE_ERROR             (0xDE)
/* Écriture NV interrompue par power loss ; rollback interne activé */
#define NXP_NATIVE_COMMAND_STATUS_EEPROM_ERROR                (0xEE)
/* Numéro fichier absent */
#define NXP_NATIVE_COMMAND_STATUS_FILE_NOT_FOUND              (0xF0)
/* Erreur irrécupérable fichier : fichier désactivé */
#define NXP_NATIVE_COMMAND_STATUS_FILE_INTEGRITY_ERROR        (0xF1)

typedef uint8_t NxpNativeCommandStatus;

/* Usage uniquement dans callback ; TX/RX buffers de travail */
/* Succès : Iso14443_4aErrorNone et STATUS_OPERATION_OK */
Iso14443_4aError nxp_native_command_iso14443_4a_poller(
    Iso14443_4aPoller* iso14443_4a_poller,
    NxpNativeCommandStatus* status_code,
    const BitBuffer* input_buffer,
    BitBuffer* result_buffer,
    NxpNativeCommandMode command_mode,
    BitBuffer* tx_buffer,
    BitBuffer* rx_buffer);
