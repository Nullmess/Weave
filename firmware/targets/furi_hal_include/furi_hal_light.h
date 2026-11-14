
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <furi_hal_resources.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_light_init(void);

/* Luminosité dans [0,255] */
void furi_hal_light_set(Light light, uint8_t value);

/* Luminosité [0,255] ; on_time et period en ms */
void furi_hal_light_blink_start(Light light, uint8_t brightness, uint16_t on_time, uint16_t period);

void furi_hal_light_blink_stop(void);

void furi_hal_light_blink_set_color(Light light);

void furi_hal_light_sequence(const char* sequence);

#ifdef __cplusplus
}
#endif
