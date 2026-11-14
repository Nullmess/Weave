/* Privilégier les APIs nfc.h et nfc_protocol.h au HAL */
#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fréquence carrier en Hz */
#define FURI_HAL_NFC_CARRIER_HZ (13560000UL)

/* Sentinelle d’attente sans timeout */
#define FURI_HAL_NFC_EVENT_WAIT_FOREVER (0xFFFFFFFFU)

typedef enum {
    FuriHalNfcEventOscOn = (1U << 0),
    FuriHalNfcEventFieldOn = (1U << 1),
    FuriHalNfcEventFieldOff = (1U << 2),
    FuriHalNfcEventListenerActive = (1U << 3),
    FuriHalNfcEventTxStart = (1U << 4),
    FuriHalNfcEventTxEnd = (1U << 5),
    FuriHalNfcEventRxStart = (1U << 6),
    FuriHalNfcEventRxEnd = (1U << 7),
    FuriHalNfcEventCollision = (1U << 8),
    FuriHalNfcEventTimerFwtExpired = (1U << 9),
    FuriHalNfcEventTimerBlockTxExpired = (1U << 10),
    FuriHalNfcEventTimeout =
        (1U << 11),
    FuriHalNfcEventAbortRequest =
        (1U << 12),
} FuriHalNfcEvent;

typedef enum {
    FuriHalNfcErrorNone,
    FuriHalNfcErrorBusy,
    FuriHalNfcErrorCommunication, /* Réponse hardware absente ou inattendue */
    FuriHalNfcErrorOscillator,
    FuriHalNfcErrorCommunicationTimeout,
    FuriHalNfcErrorBufferOverflow, /* Buffer RX insuffisant */
    FuriHalNfcErrorIncompleteFrame, /* Données insuffisantes pour une trame */
    FuriHalNfcErrorDataFormat, /* Données de trame invalides ou inattendues */
} FuriHalNfcError;

typedef enum {
    FuriHalNfcModePoller,
    FuriHalNfcModeListener,

    FuriHalNfcModeNum, /* Sentinelle interne : nombre de modes */
} FuriHalNfcMode;

typedef enum {
    FuriHalNfcTechIso14443a,
    FuriHalNfcTechIso14443b,
    FuriHalNfcTechIso15693,
    FuriHalNfcTechFelica,

    FuriHalNfcTechNum, /* Sentinelle interne : nombre de technologies */
    FuriHalNfcTechInvalid, /* État interne non configuré */
} FuriHalNfcTech;

/* Initialisation automatique au démarrage firmware */
FuriHalNfcError furi_hal_nfc_init(void);

FuriHalNfcError furi_hal_nfc_is_hal_ready(void);

/* Acquisition exclusive requise avant tout accès NFC */
FuriHalNfcError furi_hal_nfc_acquire(void);

/* Release depuis le thread ayant acquis le NFC */
FuriHalNfcError furi_hal_nfc_release(void);

/* Passer en low-power entre les utilisations NFC */
FuriHalNfcError furi_hal_nfc_low_power_mode_start(void);

/* Sortir de low-power avant toute utilisation NFC */
FuriHalNfcError furi_hal_nfc_low_power_mode_stop(void);

/* Mode listener non disponible pour toutes les technologies */
FuriHalNfcError furi_hal_nfc_set_mode(FuriHalNfcMode mode, FuriHalNfcTech tech);

FuriHalNfcError furi_hal_nfc_reset_mode(void);

FuriHalNfcError furi_hal_nfc_field_detect_start(void);

FuriHalNfcError furi_hal_nfc_field_detect_stop(void);

bool furi_hal_nfc_field_is_present(void);

/* Carrier non modulé avant démarrage explicite de transmission */
FuriHalNfcError furi_hal_nfc_poller_field_on(void);

/* Timeout en ms */
FuriHalNfcEvent furi_hal_nfc_poller_wait_event(uint32_t timeout_ms);

/* Timeout en ms */
FuriHalNfcEvent furi_hal_nfc_listener_wait_event(uint32_t timeout_ms);

/* Longueur TX en bits */
FuriHalNfcError furi_hal_nfc_poller_tx(const uint8_t* tx_data, size_t tx_bits);

/* Buffer RX dimensionné en bytes ; longueur reçue en bits */
FuriHalNfcError furi_hal_nfc_poller_rx(uint8_t* rx_data, size_t rx_data_size, size_t* rx_bits);

/* Longueur TX en bits */
FuriHalNfcError furi_hal_nfc_listener_tx(const uint8_t* tx_data, size_t tx_bits);

/* Buffer RX dimensionné en bytes ; longueur reçue en bits */
FuriHalNfcError furi_hal_nfc_listener_rx(uint8_t* rx_data, size_t rx_data_size, size_t* rx_bits);

/* Listener : état Sleep/Halt */
FuriHalNfcError furi_hal_nfc_listener_sleep(void);

/* Listener : état Sense/Idle */
FuriHalNfcError furi_hal_nfc_listener_idle(void);

FuriHalNfcError furi_hal_nfc_listener_enable_rx(void);

/* Reset des états et arrêt des activités TX/RX */
FuriHalNfcError furi_hal_nfc_trx_reset(void);

/* Appel depuis le même thread que wait_event */
FuriHalNfcError furi_hal_nfc_event_start(void);

/* Appel autorisé depuis n’importe quel thread */
FuriHalNfcError furi_hal_nfc_event_stop(void);

FuriHalNfcError furi_hal_nfc_abort(void);

/* Délai en cycles carrier */
void furi_hal_nfc_timer_fwt_start(uint32_t time_fc);

void furi_hal_nfc_timer_fwt_stop(void);

/* Délai en cycles carrier */
void furi_hal_nfc_timer_block_tx_start(uint32_t time_fc);

/* Délai en µs */
void furi_hal_nfc_timer_block_tx_start_us(uint32_t time_us);

void furi_hal_nfc_timer_block_tx_stop(void);

bool furi_hal_nfc_timer_block_tx_is_running(void);

/* Hardware NFC : implémentations partielles nécessitant des APIs spécifiques */

typedef enum {
    FuriHalNfcaShortFrameAllReq,
    FuriHalNfcaShortFrameSensReq,
} FuriHalNfcaShortFrame;

FuriHalNfcError furi_hal_nfc_iso14443a_poller_trx_short_frame(FuriHalNfcaShortFrame frame);

/* Longueur TX en bits */
FuriHalNfcError furi_hal_nfc_iso14443a_tx_sdd_frame(const uint8_t* tx_data, size_t tx_bits);

/* Buffer RX dimensionné en bytes ; longueur reçue en bits */
FuriHalNfcError
    furi_hal_nfc_iso14443a_rx_sdd_frame(uint8_t* rx_data, size_t rx_data_size, size_t* rx_bits);

/* Parité fournie par le caller, pas calculée automatiquement */
FuriHalNfcError
    furi_hal_nfc_iso14443a_poller_tx_custom_parity(const uint8_t* tx_data, size_t tx_bits);

/* UID en bytes, longueur compatible protocole requise */
FuriHalNfcError furi_hal_nfc_iso14443a_listener_set_col_res_data(
    uint8_t* uid,
    uint8_t uid_len,
    uint8_t* atqa,
    uint8_t sak);

/* Parité bit-packed ; longueur TX en bits */
FuriHalNfcError furi_hal_nfc_iso14443a_listener_tx_custom_parity(
    const uint8_t* tx_data,
    const uint8_t* tx_parity,
    size_t tx_bits);

FuriHalNfcError furi_hal_nfc_iso15693_listener_tx_sof(void);

/* IDm/PMm en bytes ; sys_code issu de SYS_C */
FuriHalNfcError furi_hal_nfc_felica_listener_set_sensf_res_data(
    const uint8_t* idm,
    const uint8_t idm_len,
    const uint8_t* pmm,
    const uint8_t pmm_len,
    const uint16_t sys_code);

#ifdef __cplusplus
}
#endif
