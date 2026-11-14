/**
 * @file furi_hal_subghz.h
 * SubGhz HAL API
 */

#pragma once

#include <lib/subghz/devices/preset.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <toolbox/level_duration.h>
#include <furi_hal_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Various subghz defines */
#define FURI_HAL_SUBGHZ_ASYNC_TX_BUFFER_FULL (256u)
#define FURI_HAL_SUBGHZ_ASYNC_TX_BUFFER_HALF (FURI_HAL_SUBGHZ_ASYNC_TX_BUFFER_FULL / 2)
#define FURI_HAL_SUBGHZ_ASYNC_TX_GUARD_TIME  (999u)

/** Switchable Radio Paths */
typedef enum {
    FuriHalSubGhzPathIsolate,
    FuriHalSubGhzPath433, /**< Center Frequency: 433MHz. Path 1: SW1RF1-SW2RF2, LCLCL */
    FuriHalSubGhzPath315, /**< Center Frequency: 315MHz. Path 2: SW1RF2-SW2RF1, LCLCLCL */
    FuriHalSubGhzPath868, /**< Center Frequency: 868MHz. Path 3: SW1RF3-SW2RF3, LCLC */
} FuriHalSubGhzPath;

/* Pin en sortie ; jamais reliée directement power/ground */
/* NULL désactive mirror async RX/TX */

void furi_hal_subghz_set_async_mirror_pin(const GpioPin* pin);

const GpioPin* furi_hal_subghz_get_data_gpio(void);

/* Réinitialise device sûr puis power save */

void furi_hal_subghz_init(void);

void furi_hal_subghz_sleep(void);

void furi_hal_subghz_dump_state(void);

void furi_hal_subghz_load_custom_preset(const uint8_t* preset_data);

void furi_hal_subghz_load_registers(const uint8_t* data);

/* PATABLE : huit valeurs uint8_t */

void furi_hal_subghz_load_patable(const uint8_t data[8]);

void furi_hal_subghz_write_packet(const uint8_t* data, uint8_t size);

bool furi_hal_subghz_rx_pipe_not_empty(void);

bool furi_hal_subghz_is_rx_data_crc_valid(void);

void furi_hal_subghz_read_packet(uint8_t* data, uint8_t* size);

/** Flush rx FIFO buffer
 */
void furi_hal_subghz_flush_rx(void);

/** Flush tx FIFO buffer
 */
void furi_hal_subghz_flush_tx(void);

/* SPWD efface contenu registres */

void furi_hal_subghz_shutdown(void);

/* Reset efface contenu registres */

void furi_hal_subghz_reset(void);

void furi_hal_subghz_idle(void);

void furi_hal_subghz_rx(void);

/* true si émission autorisée dans la région */

bool furi_hal_subghz_tx(void);

/* RSSI en dBm */

float furi_hal_subghz_get_rssi(void);

uint8_t furi_hal_subghz_get_lqi(void);

/* Fréquence en Hz ; true si valide */

bool furi_hal_subghz_is_frequency_valid(uint32_t value);

/* Weave optional out-of-default-band tuning; user opt-in only. */
void furi_hal_subghz_set_extended_range(bool enabled);
bool furi_hal_subghz_get_extended_range(void);

/* Fréquence Hz ; antenne sélectionnée automatiquement ; fréquence réelle renvoyée */

uint32_t furi_hal_subghz_set_frequency_and_path(uint32_t value);

uint32_t furi_hal_subghz_set_frequency(uint32_t value);

void furi_hal_subghz_set_path(FuriHalSubGhzPath path);

/* High Level API */

/** Signal Timings Capture callback */
typedef void (*FuriHalSubGhzCaptureCallback)(bool level, uint32_t duration, void* context);

/* Capture timings : initialise GPIO/TIM2 */

void furi_hal_subghz_start_async_rx(FuriHalSubGhzCaptureCallback callback, void* context);

/* Stop capture : reset GPIO/TIM2 */

void furi_hal_subghz_stop_async_rx(void);

/* Callback TX async : LevelDuration */

typedef LevelDuration (*FuriHalSubGhzAsyncTxCallback)(void* context);

/* TX async initialise GPIO/TIM2/DMA1 ; true si région autorise */

bool furi_hal_subghz_start_async_tx(FuriHalSubGhzAsyncTxCallback callback, void* context);

/* true si TX terminée */

bool furi_hal_subghz_is_async_tx_complete(void);

/* Stop async : cleanup GPIO/TIM2/DMA1 */

void furi_hal_subghz_stop_async_tx(void);

#ifdef __cplusplus
}
#endif
