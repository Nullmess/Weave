#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t start;
    uint32_t end;
    int8_t power_limit;
    uint8_t duty_cycle;
} FuriHalRegionBand;

typedef struct {
    char country_code[4];
    uint16_t bands_count;
    FuriHalRegionBand bands[];
} FuriHalRegion;

void furi_hal_region_init(void);

/* Région en Flash ou RAM ; vérifier avant libération */
const FuriHalRegion* furi_hal_region_get(void);

void furi_hal_region_set(FuriHalRegion* region);

bool furi_hal_region_is_provisioned(void);

/* Weave advanced radio options. Disabled at boot until explicitly loaded from SD. */
void furi_hal_region_set_tx_bypass(bool enabled);
bool furi_hal_region_get_tx_bypass(void);

/* ISO 3166 ; 00 developer, WW défaut, -- non provisionnée */
const char* furi_hal_region_get_name(void);

/* Fréquence en Hz ; retour true si émission autorisée */
bool furi_hal_region_is_frequency_allowed(uint32_t frequency);

const FuriHalRegionBand* furi_hal_region_get_band(uint32_t frequency);

#ifdef __cplusplus
}
#endif
