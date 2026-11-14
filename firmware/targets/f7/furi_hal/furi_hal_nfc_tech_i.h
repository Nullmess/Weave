/* Header interne exclu des API publiques ; ajout technologie ici */

#pragma once

#include <furi_hal_nfc.h>
#include <furi_hal_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef FuriHalNfcError (*FuriHalNfcChipConfig)(const FuriHalSpiBusHandle* handle);

/* Framing/timing protocolaire ; tx_bits en bits */

typedef FuriHalNfcError (
    *FuriHalNfcTx)(const FuriHalSpiBusHandle* handle, const uint8_t* tx_data, size_t tx_bits);

/* Framing/timing protocolaire ; capacité octets, longueur reçue bits */

typedef FuriHalNfcError (*FuriHalNfcRx)(
    const FuriHalSpiBusHandle* handle,
    uint8_t* rx_data,
    size_t rx_data_size,
    size_t* rx_bits);

/* timeout_ms en millisecondes ; retour bitmask events */

typedef FuriHalNfcEvent (*FuriHalNfcWaitEvent)(uint32_t timeout_ms);

/* Passive target : Sleep/Halt */

typedef FuriHalNfcError (*FuriHalNfcSleep)(const FuriHalSpiBusHandle* handle);

/* Passive target : Sense/Idle */

typedef FuriHalNfcError (*FuriHalNfcIdle)(const FuriHalSpiBusHandle* handle);

/* Compensation soustraite au timer : positive réduit timeout, négative augmente */

typedef struct {
    int32_t fdt; /* Compensation FDT en cycles carrier */
    int32_t fwt; /* Compensation FWT en cycles carrier */
} FuriHalNfcPollerCompensation;

typedef struct {
    FuriHalNfcPollerCompensation compensation;
    FuriHalNfcChipConfig init;
    FuriHalNfcChipConfig deinit;
    FuriHalNfcWaitEvent wait_event;
    FuriHalNfcTx tx;
    FuriHalNfcRx rx;
} FuriHalNfcTechPollerBase;

/* Compensation listener selon mêmes règles que poller */

typedef struct {
    int32_t fdt; /* Compensation FDT en cycles carrier */
} FuriHalNfcListenerCompensation;

/* Technologie sans listener : remplir structure de zéros */

typedef struct {
    FuriHalNfcListenerCompensation compensation;
    FuriHalNfcChipConfig init;
    FuriHalNfcChipConfig deinit;
    FuriHalNfcWaitEvent wait_event;
    FuriHalNfcTx tx;
    FuriHalNfcRx rx;
    FuriHalNfcSleep sleep;
    FuriHalNfcIdle idle;
} FuriHalNfcTechListenerBase;

/* Chaque technologie doit fournir fonctions et constantes */

typedef struct {
    FuriHalNfcTechPollerBase poller;
    FuriHalNfcTechListenerBase listener;
} FuriHalNfcTechBase;

extern const FuriHalNfcTechBase furi_hal_nfc_iso14443a;

extern const FuriHalNfcTechBase furi_hal_nfc_iso14443b;

extern const FuriHalNfcTechBase furi_hal_nfc_iso15693;

extern const FuriHalNfcTechBase furi_hal_nfc_felica;
/* Déclarer les nouvelles technologies ici */

/* Ajouter toute nouvelle technologie au tableau dans furi_hal_nfc.c */

extern const FuriHalNfcTechBase* const furi_hal_nfc_tech[];

#ifdef __cplusplus
}
#endif
