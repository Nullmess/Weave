#pragma once

#include "../types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* SecPlusV2 : serial32, bouton8, counter28 */

bool subghz_protocol_secplus_v2_create_data(
    void* context,
    FlipperFormat* flipper_format,
    uint32_t serial,
    uint8_t btn,
    uint32_t cnt,
    SubGhzRadioPreset* preset);

/* Keeloq : serial28, bouton4, counter16 */

bool subghz_protocol_keeloq_create_data(
    void* context,
    FlipperFormat* flipper_format,
    uint32_t serial,
    uint8_t btn,
    uint16_t cnt,
    const char* manufacture_name,
    SubGhzRadioPreset* preset);

typedef struct SubGhzProtocolDecoderBinRAW SubGhzProtocolDecoderBinRAW;

void subghz_protocol_decoder_bin_raw_data_input_rssi(
    SubGhzProtocolDecoderBinRAW* instance,
    float rssi);

/* Validation parties fixes SecPlus V1 ; true si réussi */

bool subghz_protocol_secplus_v1_check_fixed(uint32_t fixed);

#ifdef __cplusplus
}
#endif
