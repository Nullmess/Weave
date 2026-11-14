#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* QH-433 / Treadmill37 static OOK: 37 bits, 300/900 us pulses, 6 ms gap.
 * A small transport-independent codec allows host-side regression tests. */
typedef struct {
    uint64_t data;
    uint8_t bits;
    uint8_t state;
    uint32_t last_high;
} SubGhzTreadmill37Codec;

typedef struct {
    bool level;
    uint32_t duration;
} SubGhzTreadmill37Pulse;

void subghz_treadmill37_codec_reset(SubGhzTreadmill37Codec* codec);
/* Returns true only on a complete, valid frame. */
bool subghz_treadmill37_codec_feed(
    SubGhzTreadmill37Codec* codec,
    bool level,
    uint32_t duration,
    uint64_t* result);
/* Writes 74 pulse entries; returns zero for invalid key/size/capacity. */
size_t subghz_treadmill37_codec_encode(
    uint64_t key,
    uint8_t bits,
    SubGhzTreadmill37Pulse* output,
    size_t capacity);
