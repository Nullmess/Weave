/* ADC 12 bits, environ 10 bits effectifs ; SMPS bruité */
/* VREF interne 2,048 ou 2,5 V ; single-ended uniquement */
/* Pas d’API DMA/interrupt ; modes additionnels via contrôles bas niveau */
/* GPIO Analog, acquire, configure, read puis release */

#pragma once

#include <furi.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriHalAdcHandle FuriHalAdcHandle;

typedef enum {
    FuriHalAdcScale2048, /* Échelle 2,048 V */
    FuriHalAdcScale2500, /* Échelle 2,5 V */
} FuriHalAdcScale;

typedef enum {
    FuriHalAdcClockSync16, /* 16 MHz synchrone */
    FuriHalAdcClockSync32, /* 32 MHz synchrone */
    FuriHalAdcClockSync64, /* 64 MHz synchrone */
} FuriHalAdcClock;

typedef enum {
    FuriHalAdcOversample2,
    FuriHalAdcOversample4,
    FuriHalAdcOversample8,
    FuriHalAdcOversample16,
    FuriHalAdcOversample32,
    FuriHalAdcOversample64,
    FuriHalAdcOversample128,
    FuriHalAdcOversample256,
    FuriHalAdcOversampleNone,
} FuriHalAdcOversample;

typedef enum {
    FuriHalAdcSamplingtime2_5, /* Sampling : 2,5 cycles ADC */
    FuriHalAdcSamplingtime6_5, /* Sampling : 6,5 cycles ADC */
    FuriHalAdcSamplingtime12_5, /* Sampling : 12,5 cycles ADC */
    FuriHalAdcSamplingtime24_5, /* Sampling : 24,5 cycles ADC */
    FuriHalAdcSamplingtime47_5, /* Sampling : 47,5 cycles ADC */
    FuriHalAdcSamplingtime92_5, /* Sampling : 92,5 cycles ADC */
    FuriHalAdcSamplingtime247_5, /* Sampling : 247,5 cycles ADC */
    FuriHalAdcSamplingtime640_5, /* Sampling : 640,5 cycles ADC */
} FuriHalAdcSamplingTime;

typedef enum {
    /* Canaux 0..5 rapides */
    FuriHalAdcChannel0, /* Canal interne : VREFINT */
    FuriHalAdcChannel1, /* Canal 1p */
    FuriHalAdcChannel2, /* Canal 2p ou 1n */
    FuriHalAdcChannel3, /* Canal 3p ou 2n */
    FuriHalAdcChannel4, /* Canal 4p ou 3n */
    FuriHalAdcChannel5, /* Canal 5p ou 4n */
    /* Canaux 6..18 lents */
    FuriHalAdcChannel6, /* Canal 6p ou 5n */
    FuriHalAdcChannel7, /* Canal 7p ou 6n */
    FuriHalAdcChannel8, /* Canal 8p ou 7n */
    FuriHalAdcChannel9, /* Canal 9p ou 8n */
    FuriHalAdcChannel10, /* Canal 10p ou 9n */
    FuriHalAdcChannel11, /* Canal 11p ou 10n */
    FuriHalAdcChannel12, /* Canal 12p ou 11n */
    FuriHalAdcChannel13, /* Canal 13p ou 12n */
    FuriHalAdcChannel14, /* Canal 14p ou 13n */
    FuriHalAdcChannel15, /* Canal 15p ou 14n */
    FuriHalAdcChannel16, /* Canal 16p ou 15n */
    FuriHalAdcChannel17, /* Canal interne : TEMPSENSOR */
    FuriHalAdcChannel18, /* Canal interne : VBAT */
    /* Canal 0..18 combiné avec un périphérique interne */
    FuriHalAdcChannelVREFINT, /* VREFINT : calibration et auto-test */
    FuriHalAdcChannelTEMPSENSOR, /* TEMPSENSOR : sampling >=5 µs requis */
    FuriHalAdcChannelVBAT, /* VBAT/3 : sampling >=12 µs requis */
    /* Sentinelle : pin non connecté à l’ADC */
    FuriHalAdcChannelNone,
} FuriHalAdcChannel;

void furi_hal_adc_init(void);

/* Acquisition active les domaines power et clocks */
FuriHalAdcHandle* furi_hal_adc_acquire(void);

void furi_hal_adc_release(FuriHalAdcHandle* handle);

/* Défauts : VREF=2,048 V, clock=64 MHz, oversample=64 */
/* Sampling 247,5 cycles =3,8671875 µs ; lecture totale 260 µs */
/* Signal stable dans [0,2,048 V] ; impédance <10 kΩ */
/* Précision visée ~0,1 % ; contrôler distorsion à l’oscilloscope */
void furi_hal_adc_configure(FuriHalAdcHandle* handle);

/* Référence STM32WB requise ; canaux internes : settings spécifiques */
void furi_hal_adc_configure_ex(
    FuriHalAdcHandle* handle,
    FuriHalAdcScale scale,
    FuriHalAdcClock clock,
    FuriHalAdcOversample oversample,
    FuriHalAdcSamplingTime sampling_time);

/* Résultat ADC sur 12 bits */
uint16_t furi_hal_adc_read(FuriHalAdcHandle* handle, FuriHalAdcChannel channel);

/* Tension en mV */
float furi_hal_adc_convert_to_voltage(FuriHalAdcHandle* handle, uint16_t value);

/* Valeur VREFINT ; tension en mV */
float furi_hal_adc_convert_vref(FuriHalAdcHandle* handle, uint16_t value);

/* Valeur TEMPSENSOR ; température en °C */
float furi_hal_adc_convert_temp(FuriHalAdcHandle* handle, uint16_t value);

/* Valeur VBAT ; tension en mV */
float furi_hal_adc_convert_vbat(FuriHalAdcHandle* handle, uint16_t value);

#ifdef __cplusplus
}
#endif
