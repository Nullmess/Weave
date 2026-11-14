/**
 * @file byte_input.h
 * GUI: ByteInput keyboard view module API
 */

#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Byte input anonymous structure  */
typedef struct ByteInput ByteInput;

/* Callback press Save */
typedef void (*ByteInputCallback)(void* context);

typedef void (*ByteChangedCallback)(void* context);

ByteInput* byte_input_alloc(void);

void byte_input_free(ByteInput* byte_input);

View* byte_input_get_view(ByteInput* byte_input);

void byte_input_set_result_callback(
    ByteInput* byte_input,
    ByteInputCallback input_callback,
    ByteChangedCallback changed_callback,
    void* callback_context,
    uint8_t* bytes,
    uint8_t bytes_count);

void byte_input_set_header_text(ByteInput* byte_input, const char* text);

#ifdef __cplusplus
}
#endif
