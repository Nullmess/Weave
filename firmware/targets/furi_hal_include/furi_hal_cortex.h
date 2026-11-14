
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Timer hardware précis d’expiration */
typedef struct {
    uint32_t start;
    uint32_t value;
} FuriHalCortexTimer;

void furi_hal_cortex_init_early(void);

/* Délai en µs */
void furi_hal_cortex_delay_us(uint32_t microseconds);

/* Nombre d’instructions par µs */
uint32_t furi_hal_cortex_instructions_per_microsecond(void);

/* Timeout en µs */
FuriHalCortexTimer furi_hal_cortex_timer_get(uint32_t timeout_us);

bool furi_hal_cortex_timer_is_expired(FuriHalCortexTimer cortex_timer);

void furi_hal_cortex_timer_wait(FuriHalCortexTimer cortex_timer);

typedef enum {
    FuriHalCortexComp0,
    FuriHalCortexComp1,
    FuriHalCortexComp2,
    FuriHalCortexComp3,
} FuriHalCortexComp;

typedef enum {
    FuriHalCortexCompSizeWord = 0b10,
    FuriHalCortexCompSizeHalfWord = 0b01,
    FuriHalCortexCompSizeByte = 0b00,
} FuriHalCortexCompSize;

typedef enum {
    FuriHalCortexCompFunctionPC = 0b100,
    FuriHalCortexCompFunctionRead = 0b101,
    FuriHalCortexCompFunctionWrite = 0b110,
    FuriHalCortexCompFunctionReadWrite = 0b110,
} FuriHalCortexCompFunction;

/* Comparateurs DWT : ARM DDI0403, Debug Architecture */
void furi_hal_cortex_comp_enable(
    FuriHalCortexComp comp,
    FuriHalCortexCompFunction function,
    uint32_t value,
    uint32_t mask,
    FuriHalCortexCompSize size);

void furi_hal_cortex_comp_reset(FuriHalCortexComp comp);

#ifdef __cplusplus
}
#endif
