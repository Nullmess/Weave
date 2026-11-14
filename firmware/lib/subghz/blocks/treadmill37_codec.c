#include "treadmill37_codec.h"

#define TM_SHORT 300U
#define TM_LONG 900U
#define TM_GAP 6000U
#define TM_DELTA 150U
#define TM_GAP_DELTA 600U
#define TM_BITS 37U

/* Absolute difference without signed/unsigned conversion pitfalls. */
static bool tm_near(uint32_t actual, uint32_t expected, uint32_t tolerance) {
    return actual >= expected - tolerance && actual <= expected + tolerance;
}

void subghz_treadmill37_codec_reset(SubGhzTreadmill37Codec* codec) {
    if(!codec) return;
    codec->data = 0;
    codec->bits = 0;
    codec->last_high = 0;
    codec->state = 0;
}

bool subghz_treadmill37_codec_feed(
    SubGhzTreadmill37Codec* codec,
    bool level,
    uint32_t duration,
    uint64_t* result) {
    if(!codec || !result) return false;
    switch(codec->state) {
    case 0: /* Synchronize after a frame gap. */
        if(!level && tm_near(duration, TM_GAP, TM_GAP_DELTA)) {
            codec->data = 0;
            codec->bits = 0;
            codec->state = 1;
        }
        return false;
    case 1: /* Rising mark. */
        if(level) {
            codec->last_high = duration;
            codec->state = 2;
        } else if(!tm_near(duration, TM_GAP, TM_GAP_DELTA)) {
            subghz_treadmill37_codec_reset(codec);
        }
        return false;
    case 2: /* Space determines the current symbol or terminates the frame. */
        if(level) {
            subghz_treadmill37_codec_reset(codec);
            return false;
        }
        if(tm_near(duration, TM_GAP, TM_GAP_DELTA)) {
            /* The final bit uses the normal mark but the space is a gap.
             * Retain the terminating symbol and reject malformed tails. */
            const bool is_zero = tm_near(codec->last_high, TM_SHORT, TM_DELTA);
            const bool is_one = tm_near(codec->last_high, TM_LONG, TM_DELTA);
            const uint8_t bit = is_one ? 1U : 0U;
            const bool valid = (is_zero || is_one) && codec->bits == TM_BITS - 1U && !bit;
            if(valid) *result = (codec->data << 1U) | bit;
            codec->data = 0;
            codec->bits = 0;
            codec->last_high = 0;
            codec->state = 1; /* Gap also starts the next frame. */
            return valid;
        }
        bool is_zero = tm_near(codec->last_high, TM_SHORT, TM_DELTA) &&
                       tm_near(duration, TM_LONG, TM_DELTA);
        bool is_one = tm_near(codec->last_high, TM_LONG, TM_DELTA) &&
                      tm_near(duration, TM_SHORT, TM_DELTA);
        if((!is_zero && !is_one) || codec->bits >= TM_BITS - 1U) {
            subghz_treadmill37_codec_reset(codec);
            return false;
        }
        codec->data = (codec->data << 1U) | (is_one ? 1U : 0U);
        codec->bits++;
        codec->state = 1;
        return false;
    default:
        subghz_treadmill37_codec_reset(codec);
        return false;
    }
}

size_t subghz_treadmill37_codec_encode(
    uint64_t key,
    uint8_t bits,
    SubGhzTreadmill37Pulse* output,
    size_t capacity) {
    if(!output || bits != TM_BITS || capacity < TM_BITS * 2U ||
       key >= (1ULL << TM_BITS) || (key & 1ULL)) return 0;
    for(uint8_t i = 0; i < TM_BITS; ++i) {
        bool one = (key >> (TM_BITS - 1U - i)) & 1ULL;
        output[i * 2U].level = true;
        output[i * 2U].duration = one ? TM_LONG : TM_SHORT;
        output[i * 2U + 1U].level = false;
        output[i * 2U + 1U].duration = i == TM_BITS - 1U ? TM_GAP : (one ? TM_SHORT : TM_LONG);
    }
    return TM_BITS * 2U;
}
