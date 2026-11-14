#pragma once
#include "base.h"
#define SUBGHZ_PROTOCOL_DOORBELL32_NAME "Doorbell32"
extern const SubGhzProtocolDecoder subghz_protocol_doorbell32_decoder;
extern const SubGhzProtocolEncoder subghz_protocol_doorbell32_encoder;
extern const SubGhzProtocol subghz_protocol_doorbell32;
void* subghz_protocol_decoder_doorbell32_alloc(SubGhzEnvironment* env);
void subghz_protocol_decoder_doorbell32_free(void* ctx);
void subghz_protocol_decoder_doorbell32_reset(void* ctx);
void subghz_protocol_decoder_doorbell32_feed(void* ctx, bool level, uint32_t duration);
uint8_t subghz_protocol_decoder_doorbell32_get_hash_data(void* ctx);
SubGhzProtocolStatus subghz_protocol_decoder_doorbell32_serialize(void*,FlipperFormat*,SubGhzRadioPreset*);
SubGhzProtocolStatus subghz_protocol_decoder_doorbell32_deserialize(void*,FlipperFormat*);
void subghz_protocol_decoder_doorbell32_get_string(void*,FuriString*);
void* subghz_protocol_encoder_doorbell32_alloc(SubGhzEnvironment*);
void subghz_protocol_encoder_doorbell32_free(void*);
SubGhzProtocolStatus subghz_protocol_encoder_doorbell32_deserialize(void*,FlipperFormat*);
void subghz_protocol_encoder_doorbell32_stop(void*);
LevelDuration subghz_protocol_encoder_doorbell32_yield(void*);
