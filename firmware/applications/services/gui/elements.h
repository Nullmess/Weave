

#pragma once

#include <stdint.h>
#include <furi.h>
#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ELEMENTS_MAX_LINES_NUM  (7)
#define ELEMENTS_BOLD_MARKER    '#'
#define ELEMENTS_MONO_MARKER    '*'
#define ELEMENTS_INVERSE_MARKER '!'

/* Progression comprise entre 0 et 1 */

void elements_progress_bar(Canvas* canvas, int32_t x, int32_t y, size_t width, float progress);

/* Progress entre 0 et 1 */

void elements_progress_bar_with_text(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    float progress,
    const char* text);

void elements_scrollbar_pos(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t height,
    size_t pos,
    size_t total);

/* Scrollbar : largeur 3 px, hauteur du canvas */

void elements_scrollbar(Canvas* canvas, size_t pos, size_t total);

/* x/y : coin supérieur gauche */

void elements_frame(Canvas* canvas, int32_t x, int32_t y, size_t width, size_t height);

void elements_button_left(Canvas* canvas, const char* str);

void elements_button_right(Canvas* canvas, const char* str);

void elements_button_up(Canvas* canvas, const char* str);

void elements_button_down(Canvas* canvas, const char* str);

void elements_button_center(Canvas* canvas, const char* str);

/* Coordonnées relatives aux alignements horizontal et vertical */

void elements_multiline_text_aligned(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    Align horizontal,
    Align vertical,
    const char* text);

/** Draw multiline text
 *
 * @param   canvas  Canvas instance
 * @param   x       top left corner coordinates
 * @param   y       top left corner coordinates
 * @param   text    string (possible multiline)
 */
void elements_multiline_text(Canvas* canvas, int32_t x, int32_t y, const char* text);

/** Draw framed multiline text
 *
 * @param   canvas  Canvas instance
 * @param   x       top left corner coordinates
 * @param   y       top left corner coordinates
 * @param   text    string (possible multiline)
 */
void elements_multiline_text_framed(Canvas* canvas, int32_t x, int32_t y, const char* text);

void elements_slightly_rounded_frame(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    size_t height);

void elements_slightly_rounded_box(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    size_t height);

void elements_bold_rounded_frame(Canvas* canvas, int32_t x, int32_t y, size_t width, size_t height);

void elements_bubble(Canvas* canvas, int32_t x, int32_t y, size_t width, size_t height);

void elements_bubble_str(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    const char* text,
    Align horizontal,
    Align vertical);

/* Troncature selon largeur maximale en pixels */

void elements_string_fit_width(Canvas* canvas, FuriString* string, size_t width);

/* scroll=0 fixe ; incrément pour déplacement ; ellipsis ajoute points */

void elements_scrollable_text_line(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    FuriString* string,
    size_t scroll,
    bool ellipsis);

/* Formats ESC : # bold, * monospace, ! inverted */
/* strip_to_dots : troncature en points si largeur insuffisante */

void elements_text_box(
    Canvas* canvas,
    int32_t x,
    int32_t y,
    size_t width,
    size_t height,
    Align horizontal,
    Align vertical,
    const char* text,
    bool strip_to_dots);

#ifdef __cplusplus
}
#endif
