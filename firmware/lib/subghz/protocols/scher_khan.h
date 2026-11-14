#pragma once

#include "base.h"

#define SUBGHZ_PROTOCOL_SCHER_KHAN_NAME "Scher-Khan"

typedef struct SubGhzProtocolDecoderScherKhan SubGhzProtocolDecoderScherKhan;
typedef struct SubGhzProtocolEncoderScherKhan SubGhzProtocolEncoderScherKhan;

extern const SubGhzProtocolDecoder subghz_protocol_scher_khan_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_scher_khan_encoder;
extern const SubGhzProtocol subghz_protocol_scher_khan;

void* subghz_protocol_decoder_scher_khan_alloc(SubGhzEnvironment* environment);

void subghz_protocol_decoder_scher_khan_free(void* context);

void subghz_protocol_decoder_scher_khan_reset(void* context);

/* Level true=haut ; durée en µs */
void subghz_protocol_decoder_scher_khan_feed(void* context, bool level, uint32_t duration);

uint8_t subghz_protocol_decoder_scher_khan_get_hash_data(void* context);

SubGhzProtocolStatus subghz_protocol_decoder_scher_khan_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset);

SubGhzProtocolStatus
    subghz_protocol_decoder_scher_khan_deserialize(void* context, FlipperFormat* flipper_format);

void subghz_protocol_decoder_scher_khan_get_string(void* context, FuriString* output);
