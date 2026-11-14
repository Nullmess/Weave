/**
 * @file menu.h
 * GUI: Menu view module API
 */

#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Menu anonymous structure */
typedef struct Menu Menu;

/** Menu Item Callback */
typedef void (*MenuItemCallback)(void* context, uint32_t index);

Menu* menu_alloc(void);

void menu_free(Menu* menu);

View* menu_get_view(Menu* menu);

void menu_add_item(
    Menu* menu,
    const char* label,
    const Icon* icon,
    uint32_t index,
    MenuItemCallback callback,
    void* context);

/* Vide menu sans libérer instance */

void menu_reset(Menu* menu);

void menu_set_selected_item(Menu* menu, uint32_t index);

#ifdef __cplusplus
}
#endif
