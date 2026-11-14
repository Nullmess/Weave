#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <furi_config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriWaitForever = 0xFFFFFFFFU,
} FuriWait;

typedef enum {
    FuriFlagWaitAny = 0x00000000U, /* Attente d’au moins un flag par défaut */
    FuriFlagWaitAll = 0x00000001U, /* Attente de tous les flags */
    FuriFlagNoClear = 0x00000002U, /* Conservation des flags attendus après réception */

    FuriFlagError = 0x80000000U,
    FuriFlagErrorUnknown = 0xFFFFFFFFU,
    FuriFlagErrorTimeout = 0xFFFFFFFEU,
    FuriFlagErrorResource = 0xFFFFFFFDU,
    FuriFlagErrorParameter = 0xFFFFFFFCU,
    FuriFlagErrorISR = 0xFFFFFFFAU,
} FuriFlag;

typedef enum {
    FuriStatusOk = 0,
    FuriStatusError =
        -1, /* Erreur RTOS sans catégorie plus précise */
    FuriStatusErrorTimeout = -2,
    FuriStatusErrorResource = -3,
    FuriStatusErrorParameter = -4,
    FuriStatusErrorNoMemory =
        -5, /* Allocation ou réservation mémoire impossible */
    FuriStatusErrorISR =
        -6, /* Appel interdit depuis ISR */
    FuriStatusReserved = 0x7FFFFFFF /* Empêche réduction de la taille enum par le compiler */
} FuriStatus;

typedef enum {
    FuriSignalExit, /* Demande de sortie propre */

    FuriSignalCustom = 100, /* Début des valeurs de signaux personnalisés */
} FuriSignal;

#ifdef __cplusplus
}
#endif
