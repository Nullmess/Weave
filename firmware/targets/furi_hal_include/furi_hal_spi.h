#pragma once

#include <furi_hal_spi_config.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_spi_config_init_early(void);

void furi_hal_spi_config_deinit_early(void);

void furi_hal_spi_config_init(void);

void furi_hal_spi_dma_init(void);

void furi_hal_spi_bus_init(FuriHalSpiBus* bus);

void furi_hal_spi_bus_deinit(FuriHalSpiBus* bus);

void furi_hal_spi_bus_handle_init(const FuriHalSpiBusHandle* handle);

void furi_hal_spi_bus_handle_deinit(const FuriHalSpiBusHandle* handle);

/* Bloquant ; crash si erreur ; transition CS par handler */
void furi_hal_spi_acquire(const FuriHalSpiBusHandle* handle);

/* Crash si erreur ; transition CS par handler */
void furi_hal_spi_release(const FuriHalSpiBusHandle* handle);

/* Timeout en ms ; true si succès */
bool furi_hal_spi_bus_rx(
    const FuriHalSpiBusHandle* handle,
    uint8_t* buffer,
    size_t size,
    uint32_t timeout);

/* Timeout en ms ; true si succès */
bool furi_hal_spi_bus_tx(
    const FuriHalSpiBusHandle* handle,
    const uint8_t* buffer,
    size_t size,
    uint32_t timeout);

/* Timeout en ms ; true si succès */
bool furi_hal_spi_bus_trx(
    const FuriHalSpiBusHandle* handle,
    const uint8_t* tx_buffer,
    uint8_t* rx_buffer,
    size_t size,
    uint32_t timeout);

/* Timeout en ms ; true si succès */
bool furi_hal_spi_bus_trx_dma(
    const FuriHalSpiBusHandle* handle,
    uint8_t* tx_buffer,
    uint8_t* rx_buffer,
    size_t size,
    uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
