/**
 * @file expansion_settings.h
 * @brief Expansion module support settings.
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Expansion module support settings storage type.
 */
typedef struct {

    uint8_t uart_index;
} ExpansionSettings;

void expansion_settings_load(ExpansionSettings* settings);

void expansion_settings_save(const ExpansionSettings* settings);

#ifdef __cplusplus
}
#endif
