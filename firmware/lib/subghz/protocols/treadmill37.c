#include "treadmill37.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/treadmill37_codec.h"

#define TM37_BITS 37U
#define TM37_PULSES (TM37_BITS * 2U)

typedef struct {
    SubGhzProtocolDecoderBase base;
    SubGhzBlockGeneric generic;
    SubGhzBlockDecoder decoder;
    SubGhzTreadmill37Codec codec;
} Treadmill37Decoder;

typedef struct {
    SubGhzProtocolEncoderBase base;
    SubGhzBlockGeneric generic;
    SubGhzProtocolBlockEncoder encoder;
} Treadmill37Encoder;

const SubGhzProtocolDecoder subghz_protocol_treadmill37_decoder = {
    .alloc = subghz_protocol_decoder_treadmill37_alloc,
    .free = subghz_protocol_decoder_treadmill37_free,
    .feed = subghz_protocol_decoder_treadmill37_feed,
    .reset = subghz_protocol_decoder_treadmill37_reset,
    .get_hash_data = subghz_protocol_decoder_treadmill37_get_hash_data,
    .serialize = subghz_protocol_decoder_treadmill37_serialize,
    .deserialize = subghz_protocol_decoder_treadmill37_deserialize,
    .get_string = subghz_protocol_decoder_treadmill37_get_string,
};
const SubGhzProtocolEncoder subghz_protocol_treadmill37_encoder = {
    .alloc = subghz_protocol_encoder_treadmill37_alloc,
    .free = subghz_protocol_encoder_treadmill37_free,
    .deserialize = subghz_protocol_encoder_treadmill37_deserialize,
    .stop = subghz_protocol_encoder_treadmill37_stop,
    .yield = subghz_protocol_encoder_treadmill37_yield,
};
const SubGhzProtocol subghz_protocol_treadmill37 = {
    .name = SUBGHZ_PROTOCOL_TREADMILL37_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable |
            SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,
    .decoder = &subghz_protocol_treadmill37_decoder,
    .encoder = &subghz_protocol_treadmill37_encoder,
};

void* subghz_protocol_decoder_treadmill37_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    Treadmill37Decoder* self = malloc(sizeof(Treadmill37Decoder));
    if(!self) return NULL;
    memset(self, 0, sizeof(*self));
    self->base.protocol = &subghz_protocol_treadmill37;
    self->generic.protocol_name = self->base.protocol->name;
    subghz_treadmill37_codec_reset(&self->codec);
    return self;
}

void subghz_protocol_decoder_treadmill37_free(void* context) { free(context); }

void subghz_protocol_decoder_treadmill37_reset(void* context) {
    furi_assert(context);
    Treadmill37Decoder* self = context;
    subghz_treadmill37_codec_reset(&self->codec);
    self->decoder.decode_count_bit = 0;
    self->decoder.decode_data = 0;
}

void subghz_protocol_decoder_treadmill37_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    Treadmill37Decoder* self = context;
    uint64_t key = 0;
    bool received = subghz_treadmill37_codec_feed(&self->codec, level, duration, &key);
    self->decoder.decode_data = self->codec.data;
    self->decoder.decode_count_bit = self->codec.bits;
    if(received) {
        self->generic.data = key;
        self->generic.data_count_bit = TM37_BITS;
        self->generic.serial = (uint32_t)(key >> 17U);
        self->generic.cnt = (uint32_t)((key >> 1U) & 0xFFFFU);
        if(self->base.callback) self->base.callback(&self->base, self->base.context);
    }
}

uint8_t subghz_protocol_decoder_treadmill37_get_hash_data(void* context) {
    furi_assert(context);
    Treadmill37Decoder* self = context;
    return subghz_protocol_blocks_get_hash_data(&self->decoder, self->decoder.decode_count_bit / 8U + 1U);
}

SubGhzProtocolStatus subghz_protocol_decoder_treadmill37_serialize(
    void* context, FlipperFormat* flipper_format, SubGhzRadioPreset* preset) {
    furi_assert(context);
    return subghz_block_generic_serialize(&((Treadmill37Decoder*)context)->generic, flipper_format, preset);
}

SubGhzProtocolStatus subghz_protocol_decoder_treadmill37_deserialize(
    void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    return subghz_block_generic_deserialize_check_count_bit(
        &((Treadmill37Decoder*)context)->generic, flipper_format, TM37_BITS);
}

void subghz_protocol_decoder_treadmill37_get_string(void* context, FuriString* output) {
    furi_assert(context);
    const Treadmill37Decoder* self = context;
    furi_string_cat_printf(output,
        "%s 37b\r\nKey: 0x%010llX\r\nSerial: 0x%05lX\r\nBtn: 0x%04lX",
        self->generic.protocol_name,
        (unsigned long long)self->generic.data,
        (unsigned long)self->generic.serial,
        (unsigned long)self->generic.cnt);
}

void* subghz_protocol_encoder_treadmill37_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    Treadmill37Encoder* self = malloc(sizeof(Treadmill37Encoder));
    if(!self) return NULL;
    memset(self, 0, sizeof(*self));
    self->base.protocol = &subghz_protocol_treadmill37;
    self->generic.protocol_name = self->base.protocol->name;
    self->encoder.repeat = 10;
    self->encoder.upload = malloc(sizeof(LevelDuration) * TM37_PULSES);
    if(!self->encoder.upload) { free(self); return NULL; }
    self->encoder.size_upload = TM37_PULSES;
    return self;
}

void subghz_protocol_encoder_treadmill37_free(void* context) {
    if(!context) return;
    Treadmill37Encoder* self = context;
    free(self->encoder.upload);
    free(self);
}

SubGhzProtocolStatus subghz_protocol_encoder_treadmill37_deserialize(
    void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    Treadmill37Encoder* self = context;
    self->encoder.is_running = false;
    SubGhzProtocolStatus status = subghz_block_generic_deserialize_check_count_bit(
        &self->generic, flipper_format, TM37_BITS);
    if(status != SubGhzProtocolStatusOk) return status;
    uint32_t repeat = 10;
    if(flipper_format_read_uint32(flipper_format, "Repeat", &repeat, 1)) {
        if(repeat == 0 || repeat > 100) return SubGhzProtocolStatusError;
    }
    self->encoder.repeat = repeat;
    SubGhzTreadmill37Pulse pulses[TM37_PULSES];
    size_t count = subghz_treadmill37_codec_encode(
        self->generic.data, (uint8_t)self->generic.data_count_bit, pulses, TM37_PULSES);
    if(count != TM37_PULSES) return SubGhzProtocolStatusErrorEncoderGetUpload;
    for(size_t i = 0; i < count; ++i) {
        self->encoder.upload[i] = level_duration_make(pulses[i].level, pulses[i].duration);
    }
    self->encoder.size_upload = count;
    self->encoder.front = 0;
    self->encoder.is_running = true;
    return SubGhzProtocolStatusOk;
}

void subghz_protocol_encoder_treadmill37_stop(void* context) {
    furi_assert(context);
    Treadmill37Encoder* self = context;
    self->encoder.is_running = false;
    self->encoder.front = 0;
}

LevelDuration subghz_protocol_encoder_treadmill37_yield(void* context) {
    furi_assert(context);
    Treadmill37Encoder* self = context;
    if(!self->encoder.is_running || self->encoder.repeat == 0 || !self->encoder.size_upload) {
        self->encoder.is_running = false;
        return level_duration_reset();
    }
    LevelDuration pulse = self->encoder.upload[self->encoder.front++];
    if(self->encoder.front == self->encoder.size_upload) {
        self->encoder.front = 0;
        --self->encoder.repeat;
    }
    return pulse;
}
