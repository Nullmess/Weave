#include "doorbell32.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/doorbell32_codec.h"

#define DB32_PULSES 66U
#define DB32_BITS 32U

typedef struct {
    SubGhzProtocolDecoderBase base;
    SubGhzBlockGeneric generic;
    SubGhzBlockDecoder decoder;
    SubGhzDoorbell32Codec codec;
    uint32_t te;
} Doorbell32Decoder;

typedef struct {
    SubGhzProtocolEncoderBase base;
    SubGhzBlockGeneric generic;
    SubGhzProtocolBlockEncoder encoder;
    uint32_t te;
} Doorbell32Encoder;

const SubGhzProtocolDecoder subghz_protocol_doorbell32_decoder = {
    .alloc = subghz_protocol_decoder_doorbell32_alloc,
    .free = subghz_protocol_decoder_doorbell32_free,
    .feed = subghz_protocol_decoder_doorbell32_feed,
    .reset = subghz_protocol_decoder_doorbell32_reset,
    .get_hash_data = subghz_protocol_decoder_doorbell32_get_hash_data,
    .serialize = subghz_protocol_decoder_doorbell32_serialize,
    .deserialize = subghz_protocol_decoder_doorbell32_deserialize,
    .get_string = subghz_protocol_decoder_doorbell32_get_string,
};
const SubGhzProtocolEncoder subghz_protocol_doorbell32_encoder = {
    .alloc = subghz_protocol_encoder_doorbell32_alloc,
    .free = subghz_protocol_encoder_doorbell32_free,
    .deserialize = subghz_protocol_encoder_doorbell32_deserialize,
    .stop = subghz_protocol_encoder_doorbell32_stop,
    .yield = subghz_protocol_encoder_doorbell32_yield,
};
const SubGhzProtocol subghz_protocol_doorbell32 = {
    .name = SUBGHZ_PROTOCOL_DOORBELL32_NAME,
    .type = SubGhzProtocolTypeStatic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable |
            SubGhzProtocolFlag_Load | SubGhzProtocolFlag_Save | SubGhzProtocolFlag_Send,
    .decoder = &subghz_protocol_doorbell32_decoder,
    .encoder = &subghz_protocol_doorbell32_encoder,
};

void* subghz_protocol_decoder_doorbell32_alloc(SubGhzEnvironment* env) {
    UNUSED(env);
    Doorbell32Decoder* self = malloc(sizeof(*self));
    if(!self) return NULL;
    memset(self, 0, sizeof(*self));
    self->base.protocol = &subghz_protocol_doorbell32;
    self->generic.protocol_name = self->base.protocol->name;
    subghz_doorbell32_codec_reset(&self->codec);
    return self;
}
void subghz_protocol_decoder_doorbell32_free(void* ctx) { free(ctx); }
void subghz_protocol_decoder_doorbell32_reset(void* ctx) {
    furi_assert(ctx);
    Doorbell32Decoder* self = ctx;
    subghz_doorbell32_codec_reset(&self->codec);
    self->decoder.decode_data = 0;
    self->decoder.decode_count_bit = 0;
}
void subghz_protocol_decoder_doorbell32_feed(void* ctx, bool level, uint32_t duration) {
    furi_assert(ctx);
    Doorbell32Decoder* self = ctx;
    uint32_t key = 0;
    uint32_t te = 0;
    bool ready = subghz_doorbell32_codec_feed(&self->codec, level, duration, &key, &te);
    self->decoder.decode_data = self->codec.data;
    self->decoder.decode_count_bit = self->codec.bits;
    if(ready) {
        self->generic.data = key;
        self->generic.data_count_bit = DB32_BITS;
        self->te = te;
        if(self->base.callback) self->base.callback(&self->base, self->base.context);
    }
}
uint8_t subghz_protocol_decoder_doorbell32_get_hash_data(void* ctx) {
    furi_assert(ctx);
    Doorbell32Decoder* self = ctx;
    return subghz_protocol_blocks_get_hash_data(&self->decoder, self->decoder.decode_count_bit / 8U + 1U);
}
SubGhzProtocolStatus subghz_protocol_decoder_doorbell32_serialize(
    void* ctx, FlipperFormat* fmt, SubGhzRadioPreset* preset) {
    furi_assert(ctx);
    Doorbell32Decoder* self = ctx;
    SubGhzProtocolStatus status = subghz_block_generic_serialize(&self->generic, fmt, preset);
    if(status != SubGhzProtocolStatusOk) return status;
    if(!flipper_format_write_uint32(fmt, "TE", &self->te, 1)) return SubGhzProtocolStatusError;
    return SubGhzProtocolStatusOk;
}
SubGhzProtocolStatus subghz_protocol_decoder_doorbell32_deserialize(void* ctx, FlipperFormat* fmt) {
    furi_assert(ctx);
    Doorbell32Decoder* self = ctx;
    SubGhzProtocolStatus status = subghz_block_generic_deserialize_check_count_bit(
        &self->generic, fmt, DB32_BITS);
    if(status == SubGhzProtocolStatusOk) {
        self->te = 515;
        flipper_format_read_uint32(fmt, "TE", &self->te, 1);
    }
    return status;
}
void subghz_protocol_decoder_doorbell32_get_string(void* ctx,FuriString* output) {
    furi_assert(ctx);
    const Doorbell32Decoder* self=ctx;
    furi_string_cat_printf(output, "%s 32b\r\nKey: %08lX\r\nTe: %lu us",
        self->generic.protocol_name,
        (unsigned long)(self->generic.data & 0xFFFFFFFFU), (unsigned long)self->te);
}
void* subghz_protocol_encoder_doorbell32_alloc(SubGhzEnvironment* env) {
    UNUSED(env);
    Doorbell32Encoder* self = malloc(sizeof(*self));
    if(!self) return NULL;
    memset(self,0,sizeof(*self));
    self->base.protocol = &subghz_protocol_doorbell32;
    self->generic.protocol_name = self->base.protocol->name;
    self->encoder.repeat = 10;
    self->encoder.upload = malloc(sizeof(LevelDuration) * DB32_PULSES);
    if(!self->encoder.upload) {free(self);return NULL;}
    self->encoder.size_upload = DB32_PULSES;
    return self;
}
void subghz_protocol_encoder_doorbell32_free(void* ctx) {
    if(!ctx) return;
    Doorbell32Encoder* self=ctx;
    free(self->encoder.upload);free(self);
}
SubGhzProtocolStatus subghz_protocol_encoder_doorbell32_deserialize(void* ctx,FlipperFormat* fmt) {
    furi_assert(ctx);
    Doorbell32Encoder* self=ctx;
    self->encoder.is_running=false;
    SubGhzProtocolStatus status=subghz_block_generic_deserialize_check_count_bit(
        &self->generic, fmt, DB32_BITS);
    if(status!=SubGhzProtocolStatusOk) return status;
    uint32_t repeat=10;
    if(flipper_format_read_uint32(fmt,"Repeat",&repeat,1) && (repeat==0 || repeat>100))
        return SubGhzProtocolStatusError;
    self->encoder.repeat=repeat;
    self->te=515;
    flipper_format_read_uint32(fmt,"TE",&self->te,1);
    SubGhzDoorbell32Pulse pulses[DB32_PULSES];
    size_t count = subghz_doorbell32_codec_encode(
        (uint32_t)self->generic.data,self->te,pulses,DB32_PULSES);
    if(count!=DB32_PULSES) return SubGhzProtocolStatusErrorEncoderGetUpload;
    for(size_t i=0;i<count;i++) self->encoder.upload[i]=level_duration_make(pulses[i].level,pulses[i].duration);
    self->encoder.size_upload=count;
    self->encoder.front=0;
    self->encoder.is_running=true;
    return SubGhzProtocolStatusOk;
}
void subghz_protocol_encoder_doorbell32_stop(void* ctx) {
    furi_assert(ctx);
    Doorbell32Encoder* self=ctx;
    self->encoder.is_running=false;
    self->encoder.front=0;
}
LevelDuration subghz_protocol_encoder_doorbell32_yield(void* ctx) {
    furi_assert(ctx);
    Doorbell32Encoder* self=ctx;
    if(!self->encoder.is_running || self->encoder.repeat==0 || !self->encoder.size_upload) {
        self->encoder.is_running=false;
        return level_duration_reset();
    }
    LevelDuration pulse=self->encoder.upload[self->encoder.front++];
    if(self->encoder.front==self->encoder.size_upload) {
        self->encoder.front=0;
        --self->encoder.repeat;
    }
    return pulse;
}
