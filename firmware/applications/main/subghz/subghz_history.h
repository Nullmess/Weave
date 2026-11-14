
#pragma once

#include <math.h>
#include <furi.h>
#include <furi_hal.h>
#include <lib/flipper_format/flipper_format.h>
#include <lib/subghz/types.h>

typedef struct SubGhzHistory SubGhzHistory;

SubGhzHistory* subghz_history_alloc(void);

void subghz_history_free(SubGhzHistory* instance);

void subghz_history_reset(SubGhzHistory* instance);

uint32_t subghz_history_get_frequency(SubGhzHistory* instance, uint16_t idx);

SubGhzRadioPreset* subghz_history_get_radio_preset(SubGhzHistory* instance, uint16_t idx);

const char* subghz_history_get_preset(SubGhzHistory* instance, uint16_t idx);

uint16_t subghz_history_get_item(SubGhzHistory* instance);

uint8_t subghz_history_get_type_protocol(SubGhzHistory* instance, uint16_t idx);

const char* subghz_history_get_protocol_name(SubGhzHistory* instance, uint16_t idx);

void subghz_history_get_text_item_menu(SubGhzHistory* instance, FuriString* output, uint16_t idx);

/* Renvoie true si history pleine */

bool subghz_history_get_text_space_left(SubGhzHistory* instance, FuriString* output);

bool subghz_history_add_to_history(
    SubGhzHistory* instance,
    void* context,
    SubGhzRadioPreset* preset);

FlipperFormat* subghz_history_get_raw_data(SubGhzHistory* instance, uint16_t idx);
