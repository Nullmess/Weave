#pragma once

#include <furi_hal_spi_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Preset ST25R916 */
extern const LL_SPI_InitTypeDef furi_hal_spi_preset_2edge_low_8m;

/* Preset CC1101 */
extern const LL_SPI_InitTypeDef furi_hal_spi_preset_1edge_low_8m;

/* Preset écran ST7567 */
extern const LL_SPI_InitTypeDef furi_hal_spi_preset_1edge_low_4m;

/* Preset SD rapide */
extern const LL_SPI_InitTypeDef furi_hal_spi_preset_1edge_low_16m;

/* Preset SD lent */
extern const LL_SPI_InitTypeDef furi_hal_spi_preset_1edge_low_2m;

/** Furi Hal Spi Bus R (External) */
extern FuriHalSpiBus furi_hal_spi_bus_r;

/** Furi Hal Spi Bus D (Display, SdCard) */
extern FuriHalSpiBus furi_hal_spi_bus_d;

/* Bus SPI R externe, preset 1edge_low_2m */
/* MISO PA6, MOSI PA7, SCK PB3, CS logiciel PA4 */
/* Init handle explicite requise ; pins flottantes inactives, CS haut après init */

extern const FuriHalSpiBusHandle furi_hal_spi_bus_handle_external;

/* Écran ST7567 sur furi_hal_spi_bus_d */
extern const FuriHalSpiBusHandle furi_hal_spi_bus_handle_display;

/* SD rapide sur furi_hal_spi_bus_d */
extern const FuriHalSpiBusHandle furi_hal_spi_bus_handle_sd_fast;

/* SD lente sur furi_hal_spi_bus_d */
extern const FuriHalSpiBusHandle furi_hal_spi_bus_handle_sd_slow;

#ifdef __cplusplus
}
#endif
