#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include <furi_hal_gpio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_READER_NO_EDGE   (0xFFFFFFFFUL)
#define PULSE_READER_LOST_EDGE (0xFFFFFFFEUL)
#define F_TIM2                 (64000000UL)

/* Unité durées edges renvoyées */

typedef enum {
    PulseReaderUnit64MHz,
    PulseReaderUnitPicosecond,
    PulseReaderUnitNanosecond,
    PulseReaderUnitMicrosecond,
} PulseReaderUnit;

/* using an anonymous type */
typedef struct PulseReader PulseReader;

/* GPIO configuré entrée ; size : nombre edges buffered */

PulseReader* pulse_reader_alloc(const GpioPin* gpio, uint32_t size);

void pulse_reader_free(PulseReader* signal);

/* Capture DMA1/TIM2/DMAMUX_REQ_GEN_0 ; IRQ toujours actives requises */

void pulse_reader_start(PulseReader* signal);

/* Stop libère DMA1/TIM2/DMAMUX_REQ_GEN_0 */

void pulse_reader_stop(PulseReader* signal);

/* Défaut picosecondes ; bittime configuré : nombre bits arrondi ; timeout µs */

uint32_t pulse_reader_receive(PulseReader* signal, int timeout_us);

uint32_t pulse_reader_samples(PulseReader* signal);

/* Timebase 64 MHz ou picosecondes */

void pulse_reader_set_timebase(PulseReader* signal, PulseReaderUnit unit);

/* bit_time unités par bit ; 1 rétablit retour durée */

void pulse_reader_set_bittime(PulseReader* signal, uint32_t bit_time);

/* Pull par défaut GpioPullNo */

void pulse_reader_set_pull(PulseReader* signal, GpioPull pull);

#ifdef __cplusplus
}
#endif
