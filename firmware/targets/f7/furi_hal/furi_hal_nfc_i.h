/* Header interne exclu des API publiques */

#pragma once

#include <furi.h>
#include <furi_hal_nfc.h>
#include <furi_hal_spi.h>

#include <drivers/st25r3916.h>
#include <drivers/st25r3916_reg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Compensation commune FDT poller */
#define FURI_HAL_NFC_POLLER_FDT_COMP_FC (-500)
/* Compensation commune FWT poller */
#define FURI_HAL_NFC_POLLER_FWT_COMP_FC (FURI_HAL_NFC_POLLER_FDT_COMP_FC)

typedef enum {
    FuriHalNfcEventInternalTypeAbort = (1U << 0), /* Annule attente events hardware */
    FuriHalNfcEventInternalTypeIrq = (1U << 1),
    FuriHalNfcEventInternalTypeTimerFwtExpired =
        (1U << 2), /* Timeout FWT écoulé */
    FuriHalNfcEventInternalTypeTimerBlockTxExpired =
        (1U << 3), /* Timeout bloc transmission écoulé */
    FuriHalNfcEventInternalTypeTransparentDataReceived =
        (1U << 4), /* Données reçues en transparent mode */
} FuriHalNfcEventInternalType;

/* Bitmask regroupant tous les events internes */
#define FURI_HAL_NFC_EVENT_INTERNAL_ALL                                   \
    ((FuriHalNfcEventInternalTypeAbort | FuriHalNfcEventInternalTypeIrq | \
      FuriHalNfcEventInternalTypeTimerFwtExpired |                        \
      FuriHalNfcEventInternalTypeTimerBlockTxExpired |                    \
      FuriHalNfcEventInternalTypeTransparentDataReceived))

typedef struct {
    FuriThreadId thread; /* Thread destinataire des events */
    void* context;
} FuriHalNfcEventInternal;

typedef struct {
    FuriMutex* mutex;
    FuriHalNfcMode mode;
    FuriHalNfcTech tech;
} FuriHalNfc;

extern FuriHalNfc furi_hal_nfc;

void furi_hal_nfc_event_init(void);

/* Bitmask d’un ou plusieurs events */

void furi_hal_nfc_event_set(FuriHalNfcEventInternalType event);

void furi_hal_nfc_init_gpio_isr(void);

void furi_hal_nfc_deinit_gpio_isr(void);

void furi_hal_nfc_timers_init(void);

void furi_hal_nfc_timers_deinit(void);

/* Renvoie bitmask IRQ survenues */

uint32_t furi_hal_nfc_get_irq(const FuriHalSpiBusHandle* handle);

/* timeout_ms en millisecondes ; true si IRQ masquées survenues */

bool furi_hal_nfc_event_wait_for_specific_irq(
    const FuriHalSpiBusHandle* handle,
    uint32_t mask,
    uint32_t timeout_ms);

/* Toutes technologies ; timeout_ms en ms ; retour bitmask events */

FuriHalNfcEvent furi_hal_nfc_wait_event_common(uint32_t timeout_ms);

FuriHalNfcError furi_hal_nfc_common_listener_rx_start(const FuriHalSpiBusHandle* handle);

/* FIFO chip : tx_bits en bits */

FuriHalNfcError furi_hal_nfc_common_fifo_tx(
    const FuriHalSpiBusHandle* handle,
    const uint8_t* tx_data,
    size_t tx_bits);

/* FIFO chip : capacité octets, longueur reçue bits */

FuriHalNfcError furi_hal_nfc_common_fifo_rx(
    const FuriHalSpiBusHandle* handle,
    uint8_t* rx_data,
    size_t rx_data_size,
    size_t* rx_bits);

/* Toutes technologies : tx_bits en bits */

FuriHalNfcError furi_hal_nfc_poller_tx_common(
    const FuriHalSpiBusHandle* handle,
    const uint8_t* tx_data,
    size_t tx_bits);

#ifdef __cplusplus
}
#endif
