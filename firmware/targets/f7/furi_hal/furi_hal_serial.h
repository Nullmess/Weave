/**
 * @file furi_hal_serial.h
 * 
 * Serial HAL API
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "furi_hal_serial_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Init défaut8N1 ; override via configure_framing */

void furi_hal_serial_init(FuriHalSerialHandle* handle, uint32_t baud);

/* Deinit : GPIO analogiques, callbacks retirés, hardware off */

void furi_hal_serial_deinit(FuriHalSerialHandle* handle);

/* Suspend conserve settings et callbacks */

void furi_hal_serial_suspend(FuriHalSerialHandle* handle);

/* Reprise hardware depuis état suspendu */

void furi_hal_serial_resume(FuriHalSerialHandle* handle);

bool furi_hal_serial_is_baud_rate_supported(FuriHalSerialHandle* handle, uint32_t baud);

/** Changes baud rate
 *
 * @param      handle  Serial handle
 * @param      baud    baud rate
 */
void furi_hal_serial_set_br(FuriHalSerialHandle* handle, uint32_t baud);

void furi_hal_serial_configure_framing(
    FuriHalSerialHandle* handle,
    FuriHalSerialDataBits data_bits,
    FuriHalSerialParity parity,
    FuriHalSerialStopBits stop_bits);

/* Retour quand tous octets dans pipe, transmission physique encore active */
/* Attendre tx_wait_complete si completion physique requise */

void furi_hal_serial_tx(FuriHalSerialHandle* handle, const uint8_t* buffer, size_t buffer_size);

void furi_hal_serial_tx_wait_complete(FuriHalSerialHandle* handle);

/** Serial RX events */
typedef enum {
    FuriHalSerialRxEventData = (1 << 0), /**< Data: new data available */
    FuriHalSerialRxEventIdle = (1 << 1), /**< Idle: bus idle detected */
    FuriHalSerialRxEventFrameError = (1 << 2), /* Erreur framing : frame incorrecte */
    FuriHalSerialRxEventNoiseError = (1 << 3), /* Noise error : bruit ligne détecté */
    FuriHalSerialRxEventOverrunError = (1 << 4), /* Erreur overrun : capacité réception insuffisante */
    FuriHalSerialRxEventParityError = (1 << 5), /* Erreur parité : bit reçu incorrect */
} FuriHalSerialRxEvent;

/* Callback en ISR ; thread safety à charge appelant */

typedef void (*FuriHalSerialAsyncRxCallback)(
    FuriHalSerialHandle* handle,
    FuriHalSerialRxEvent event,
    void* context);

/* Callback en ISR ; thread safety à charge appelant */

void furi_hal_serial_async_rx_start(
    FuriHalSerialHandle* handle,
    FuriHalSerialAsyncRxCallback callback,
    void* context,
    bool report_errors);

void furi_hal_serial_async_rx_stop(FuriHalSerialHandle* handle);

/* Appel uniquement depuis FuriHalSerialAsyncRxCallback */
bool furi_hal_serial_async_rx_available(FuriHalSerialHandle* handle);

/* Appel uniquement depuis FuriHalSerialAsyncRxCallback */
uint8_t furi_hal_serial_async_rx(FuriHalSerialHandle* handle);

/* DMA based Serial API */

#define FURI_HAL_SERIAL_DMA_BUFFER_SIZE (256u)

/* DMA callback en ISR ; thread safety à charge appelant */

typedef void (*FuriHalSerialDmaRxCallback)(
    FuriHalSerialHandle* handle,
    FuriHalSerialRxEvent event,
    size_t data_len,
    void* context);

/* Enable reprend pin via alternative function */

void furi_hal_serial_enable_direction(
    FuriHalSerialHandle* handle,
    FuriHalSerialDirection direction);

/* Disable rétablit pin initiale pour autre usage */

void furi_hal_serial_disable_direction(
    FuriHalSerialHandle* handle,
    FuriHalSerialDirection direction);

const GpioPin*
    furi_hal_serial_get_gpio_pin(FuriHalSerialHandle* handle, FuriHalSerialDirection direction);

void furi_hal_serial_dma_rx_start(
    FuriHalSerialHandle* handle,
    FuriHalSerialDmaRxCallback callback,
    void* context,
    bool report_errors);

void furi_hal_serial_dma_rx_stop(FuriHalSerialHandle* handle);

/* Appel uniquement depuis FuriHalSerialDmaRxCallback ; tailles en bytes */
size_t furi_hal_serial_dma_rx(FuriHalSerialHandle* handle, uint8_t* data, size_t len);

#ifdef __cplusplus
}
#endif
