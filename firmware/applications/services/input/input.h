/**
 * @file input.h
 * Input: main API
 */

#pragma once

#include <furi_hal_resources.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_INPUT_EVENTS            "input_events"
#define INPUT_SEQUENCE_SOURCE_HARDWARE (0u)
#define INPUT_SEQUENCE_SOURCE_SOFTWARE (1u)

typedef enum {
    InputTypePress, /* Press après debounce */
    InputTypeRelease, /* Release après debounce */
    InputTypeShort, /* Short : release dans intervalle INPUT_LONG_PRESS */
    InputTypeLong, /* Long après INPUT_LONG_PRESS_COUNTS, asynchrone à Release */
    InputTypeRepeat, /* Repeat : période INPUT_LONG_PRESS_COUNTS après Long */
    InputTypeMAX,
} InputType;

/* Input events via FuriPubSub */
typedef struct {
    union {
        uint32_t sequence;
        struct {
            uint8_t sequence_source   : 2;
            uint32_t sequence_counter : 30;
        };
    };
    InputKey key;
    InputType type;
} InputEvent;

const char* input_get_key_name(InputKey key);

const char* input_get_type_name(InputType type);

#ifdef __cplusplus
}
#endif
