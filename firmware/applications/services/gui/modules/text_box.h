/**
 * @file text_box.h
 * GUI: TextBox view module API
 */

#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/** TextBox anonymous structure */
typedef struct TextBox TextBox;

typedef enum {
    TextBoxFontText,
    TextBoxFontHex,
} TextBoxFont;

typedef enum {
    TextBoxFocusStart,
    TextBoxFocusEnd,
} TextBoxFocus;

TextBox* text_box_alloc(void);

void text_box_free(TextBox* text_box);

View* text_box_get_view(TextBox* text_box);

/** Clean text_box
 *
 * @param      text_box  TextBox instance
 */
void text_box_reset(TextBox* text_box);

void text_box_set_text(TextBox* text_box, const char* text);

void text_box_set_font(TextBox* text_box, TextBoxFont font);

/* Focus au début ou à la fin */

void text_box_set_focus(TextBox* text_box, TextBoxFocus focus);

#ifdef __cplusplus
}
#endif
