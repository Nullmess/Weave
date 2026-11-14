#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Pure, bounded UPDATE BINARY planner. APDUs can update either NLEN byte
 * independently. Never allow a malformed NLEN to allocate more than the
 * advertised NDEF capacity (including on 32-bit targets). */
typedef struct {
    size_t new_size;
    size_t data_offset;
    size_t skip;
    size_t count;
} Type4TagNdefWritePlan;

static inline bool type_4_tag_ndef_write_plan(
    size_t previous_size,
    size_t max_size,
    size_t file_offset,
    const uint8_t* data,
    size_t length,
    Type4TagNdefWritePlan* out) {
    if(!out || !data || length == 0 || previous_size > max_size ||
       max_size > UINT16_MAX || file_offset > max_size + 2 ||
       length > max_size + 2 - file_offset) return false;

    size_t new_size = previous_size;
    size_t skip = 0;
    size_t body_offset = file_offset;
    if(file_offset < 2) {
        skip = length < 2 - file_offset ? length : 2 - file_offset;
        uint8_t nlen[2] = {(uint8_t)(previous_size >> 8), (uint8_t)previous_size};
        for(size_t i = 0; i < skip; ++i) nlen[file_offset + i] = data[i];
        new_size = ((size_t)nlen[0] << 8) | nlen[1];
        body_offset = 2;
    }
    body_offset -= 2;
    size_t count = length - skip;
    if(new_size > max_size || body_offset > max_size || count > max_size - body_offset)
        return false;
    if(new_size < body_offset + count) new_size = body_offset + count;
    *out = (Type4TagNdefWritePlan){new_size, body_offset, skip, count};
    return true;
}
