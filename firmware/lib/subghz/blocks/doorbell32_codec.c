#include "doorbell32_codec.h"

#define DB32_SHORT 515U
#define DB32_LONG (DB32_SHORT * 3U)
#define DB32_GAP (DB32_SHORT * 14U)
#define DB32_BITS 32U

static bool within(uint32_t value, uint32_t expected, uint32_t margin) {
    return value >= expected - margin && value <= expected + margin;
}

void subghz_doorbell32_codec_reset(SubGhzDoorbell32Codec* codec) {
    if(!codec) return;
    codec->data = 0;
    codec->last_data = 0;
    codec->last_valid = false;
    codec->duration_sum = 0;
    codec->te = DB32_SHORT;
    codec->bits = 0;
    codec->stage = 0;
    codec->high = 0;
}

static void start_frame(SubGhzDoorbell32Codec* codec) {
    codec->data = 0;
    codec->bits = 0;
    codec->duration_sum = 0;
    codec->stage = 1;
}

bool subghz_doorbell32_codec_feed(
    SubGhzDoorbell32Codec* codec,
    bool level,
    uint32_t duration,
    uint32_t* key,
    uint32_t* te) {
    if(!codec || !key || !te) return false;
    switch(codec->stage) {
    case 0:
        if(!level && within(duration, DB32_GAP, 2000U)) {
            start_frame(codec);
        } else if(!level) {
            codec->last_valid = false;
        }
        return false;
    case 1:
        if(level) {
            codec->high = duration;
            codec->stage = 2;
        } else if(!within(duration, DB32_GAP, 2000U)) {
            codec->stage = 0;
            codec->last_valid = false;
        }
        return false;
    case 2:
        if(level) {
            codec->stage = 0;
            codec->last_valid = false;
            return false;
        }
        if(within(duration, DB32_GAP, 2000U)) {
            const bool valid = codec->bits == DB32_BITS &&
                               within(codec->high, DB32_SHORT, 200U) &&
                               codec->data != 0;
            /* Require the second matching frame before exposing a signal. */
            const bool repeated = valid && codec->last_valid && codec->data == codec->last_data;
            if(repeated) {
                *key = codec->data;
                *te = (codec->duration_sum + codec->high) / (DB32_BITS * 4U + 1U);
            }
            codec->last_valid = valid;
            codec->last_data = codec->data;
            start_frame(codec);
            return repeated;
        }
        const bool bit0 = within(codec->high, DB32_SHORT, 200U) &&
                          within(duration, DB32_LONG, 400U);
        const bool bit1 = within(codec->high, DB32_LONG, 400U) &&
                          within(duration, DB32_SHORT, 200U);
        if((!bit0 && !bit1) || codec->bits >= DB32_BITS) {
            codec->stage = 0;
            codec->last_valid = false;
            return false;
        }
        codec->data = (codec->data << 1U) | (bit1 ? 1U : 0U);
        codec->bits++;
        codec->duration_sum += codec->high + duration;
        codec->stage = 1;
        return false;
    default:
        codec->stage = 0;
        codec->last_valid = false;
        return false;
    }
}

size_t subghz_doorbell32_codec_encode(
    uint32_t key,
    uint32_t te,
    SubGhzDoorbell32Pulse* pulses,
    size_t capacity) {
    if(!pulses || capacity < 66U || te < 400U || te > 640U) return 0;
    for(size_t i = 0; i < DB32_BITS; ++i) {
        const bool bit = (key >> (31U - i)) & 1U;
        pulses[2U * i] = (SubGhzDoorbell32Pulse){true, bit ? te * 3U : te};
        pulses[2U * i + 1U] = (SubGhzDoorbell32Pulse){false, bit ? te : te * 3U};
    }
    pulses[64] = (SubGhzDoorbell32Pulse){true, te};
    pulses[65] = (SubGhzDoorbell32Pulse){false, te * 14U};
    return 66U;
}
