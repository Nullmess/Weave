#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FURI_HAL_RANDOM_MAX 0xFFFFFFFFU

void furi_hal_random_init(void);

/* Random 32 bits <= FURI_HAL_RANDOM_MAX ; rand/random <= RAND_MAX */
uint32_t furi_hal_random_get(void);

void furi_hal_random_fill_buf(uint8_t* buf, uint32_t len);

#ifdef __cplusplus
}
#endif
