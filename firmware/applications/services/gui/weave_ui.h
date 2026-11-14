#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <gui/canvas.h>

/* WEAVE UI: one lightweight, monochrome layout for system screens. */
#define WEAVE_UI_HEADER_HEIGHT 15

static inline void weave_ui_header(Canvas* canvas, const char* title) {
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 11, title);
    canvas_draw_line(canvas, 0, 14, 127, 14);
    canvas_draw_box(canvas, 0, 0, 2, 13);
}

static inline void weave_ui_row(
    Canvas* canvas, uint8_t top, const char* label, bool selected, uint8_t left) {
    canvas_set_color(canvas, ColorBlack);
    if(selected) {
        canvas_draw_rbox(canvas, 2, top, 124, 14, 2);
        canvas_set_color(canvas, ColorWhite);
    }
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, left, top + 10, label);
    if(selected) canvas_draw_str(canvas, 117, top + 10, ">");
    canvas_set_color(canvas, ColorBlack);
}
