
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define INFRARED_MAX_FREQUENCY 1000000
#define INFRARED_MIN_FREQUENCY 10000

typedef enum {
    FuriHalInfraredTxPinInternal,
    FuriHalInfraredTxPinExtPA7,
    FuriHalInfraredTxPinMax,
} FuriHalInfraredTxPin;

typedef enum {
    FuriHalInfraredTxGetDataStateOk,
    FuriHalInfraredTxGetDataStateDone, /* Fin du paquet courant */
    FuriHalInfraredTxGetDataStateLastDone, /* Fin du paquet ; aucune donnée restante */
} FuriHalInfraredTxGetDataState;

typedef FuriHalInfraredTxGetDataState (
    *FuriHalInfraredTxGetDataISRCallback)(void* context, uint32_t* duration, bool* level);

/* Callback DMA : deux timings restent à émettre */
/* Ne pas arrêter TX ici : signal suivant déjà chargé */
typedef void (*FuriHalInfraredTxSignalSentISRCallback)(void* context);

/* Durée du niveau reçu en µs */
typedef void (*FuriHalInfraredRxCaptureCallback)(void* ctx, bool level, uint32_t duration);

typedef void (*FuriHalInfraredRxTimeoutCallback)(void* ctx);

/* Interrupt à chaque front RX avec mesure de durée */
void furi_hal_infrared_async_rx_start(void);

void furi_hal_infrared_async_rx_stop(void);

/* Timeout silence en µs ; associer avec timeout_irq_set_callback */
void furi_hal_infrared_async_rx_set_timeout(uint32_t timeout_us);

/* RX interrupt préalablement initialisée requise */
void furi_hal_infrared_async_rx_set_capture_isr_callback(
    FuriHalInfraredRxCaptureCallback callback,
    void* ctx);

/* Appeler setup_rx_timeout_irq avant enregistrement du callback */
void furi_hal_infrared_async_rx_set_timeout_isr_callback(
    FuriHalInfraredRxTimeoutCallback callback,
    void* ctx);

bool furi_hal_infrared_is_busy(void);

/* Appel obligatoire avant async_tx_start */
void furi_hal_infrared_async_tx_set_data_isr_callback(
    FuriHalInfraredTxGetDataISRCallback callback,
    void* context);

/* Arrêt via async_tx_stop ou réponse LastDone du callback */
/* Stop ou wait_termination obligatoire pour libérer les ressources */
void furi_hal_infrared_async_tx_start(uint32_t freq, float duty_cycle);

/* Arrêt et libération à la prochaine fin de paquet */
void furi_hal_infrared_async_tx_stop(void);

/* Attente LastDone puis libération des ressources */
void furi_hal_infrared_async_tx_wait_termination(void);

void furi_hal_infrared_async_tx_set_signal_sent_isr_callback(
    FuriHalInfraredTxSignalSentISRCallback callback,
    void* context);

/* Détection module par weak pull-up ; tension <=0,9 V requise */
/* Sans module : retour TxPinInternal */
FuriHalInfraredTxPin furi_hal_infrared_detect_tx_output(void);

void furi_hal_infrared_set_tx_output(FuriHalInfraredTxPin tx_pin);

#ifdef __cplusplus
}
#endif
