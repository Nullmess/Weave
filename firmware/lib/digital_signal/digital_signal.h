/* Niveau initial puis alternance à chaque frontière période */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// DigitalSignal uses 10 picosecond time units (1 tick = 10 ps).
/* Convertir unités via macros suivantes */

#define DIGITAL_SIGNAL_MS(x) ((x) * 100000000UL)
#define DIGITAL_SIGNAL_US(x) ((x) * 100000UL)
#define DIGITAL_SIGNAL_NS(x) ((x) * 100UL)
#define DIGITAL_SIGNAL_PS(x) ((x) / 10UL)

typedef struct DigitalSignal DigitalSignal;

/* max_size : nombre maximal périodes */

DigitalSignal* digital_signal_alloc(uint32_t max_size);

void digital_signal_free(DigitalSignal* signal);

/* Ticks en unités de 10 ps */

void digital_signal_add_period(DigitalSignal* signal, uint32_t ticks);

/* Même niveau final : prolonger période ; ticks unités 10 ps */

void digital_signal_add_period_with_level(DigitalSignal* signal, uint32_t ticks, bool level);

/* Niveau initial false par défaut */

bool digital_signal_get_start_level(const DigitalSignal* signal);

void digital_signal_set_start_level(DigitalSignal* signal, bool level);

uint32_t digital_signal_get_size(const DigitalSignal* signal);

#ifdef __cplusplus
}
#endif
