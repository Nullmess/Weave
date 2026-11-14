#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 32-bit 1:3 OOK frames, with a 14*TE guard and a repeat check. */
typedef struct {
    uint32_t data;
    uint32_t last_data;
    uint32_t duration_sum;
    uint32_t te;
    uint8_t bits;
    uint8_t stage;
    bool last_valid;
    uint32_t high;
} SubGhzDoorbell32Codec;

typedef struct {
    bool level;
    uint32_t duration;
} SubGhzDoorbell32Pulse;

void subghz_doorbell32_codec_reset(SubGhzDoorbell32Codec* codec);
/* Returns true after two identical valid frames, providing key and nominal TE. */
bool subghz_doorbell32_codec_feed(
    SubGhzDoorbell32Codec* codec,
    bool level,
    uint32_t duration,
    uint32_t* key,
    uint32_t* te);
size_t subghz_doorbell32_codec_encode(
    uint32_t key,
    uint32_t te,
    SubGhzDoorbell32Pulse* pulses,
    size_t capacity);
