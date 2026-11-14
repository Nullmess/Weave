/**
 * @file dialog_ex.h
 * GUI: DialogEx view module API
 */

#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Dialog anonymous structure */
typedef struct DialogEx DialogEx;

/** DialogEx result */
typedef enum {
    DialogExResultLeft,
    DialogExResultCenter,
    DialogExResultRight,
    DialogExPressLeft,
    DialogExPressCenter,
    DialogExPressRight,
    DialogExReleaseLeft,
    DialogExReleaseCenter,
    DialogExReleaseRight,
} DialogExResult;

/* Callback depuis thread GUI */

typedef void (*DialogExResultCallback)(DialogExResult result, void* context);

DialogEx* dialog_ex_alloc(void);

void dialog_ex_free(DialogEx* dialog_ex);

View* dialog_ex_get_view(DialogEx* dialog_ex);

void dialog_ex_set_result_callback(DialogEx* dialog_ex, DialogExResultCallback callback);

void dialog_ex_set_context(DialogEx* dialog_ex, void* context);

/* text NULL : header masqué ; multiline admis */

void dialog_ex_set_header(
    DialogEx* dialog_ex,
    const char* text,
    uint8_t x,
    uint8_t y,
    Align horizontal,
    Align vertical);

/* text NULL : texte masqué ; multiline admis */

void dialog_ex_set_text(
    DialogEx* dialog_ex,
    const char* text,
    uint8_t x,
    uint8_t y,
    Align horizontal,
    Align vertical);

/* x/y négatif : icône masquée */

void dialog_ex_set_icon(DialogEx* dialog_ex, uint8_t x, uint8_t y, const Icon* icon);

/* text NULL : bouton gauche masqué/inactif */

void dialog_ex_set_left_button_text(DialogEx* dialog_ex, const char* text);

/* text NULL : bouton central masqué/inactif */

void dialog_ex_set_center_button_text(DialogEx* dialog_ex, const char* text);

/* text NULL : bouton droit masqué/inactif */

void dialog_ex_set_right_button_text(DialogEx* dialog_ex, const char* text);

/** Clean dialog
 *
 * @param      dialog_ex  DialogEx instance
 */
void dialog_ex_reset(DialogEx* dialog_ex);

void dialog_ex_enable_extended_events(DialogEx* dialog_ex);

void dialog_ex_disable_extended_events(DialogEx* dialog_ex);

#ifdef __cplusplus
}
#endif
