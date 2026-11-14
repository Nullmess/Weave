/**
 * @file button_menu.h
 * GUI: ButtonMenu view module API
 */

#pragma once

#include <stdint.h>
#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ButtonMenu anonymous structure */
typedef struct ButtonMenu ButtonMenu;

/** ButtonMenuItem anonymous structure */
typedef struct ButtonMenuItem ButtonMenuItem;

typedef void (*ButtonMenuItemCallback)(void* context, int32_t index, InputType type);

typedef enum {
    ButtonMenuItemTypeCommon,
    ButtonMenuItemTypeControl,
} ButtonMenuItemType;

View* button_menu_get_view(ButtonMenu* button_menu);

/** Clean button menu
 *
 * @param      button_menu  ButtonMenu instance
 */
void button_menu_reset(ButtonMenu* button_menu);

ButtonMenuItem* button_menu_add_item(
    ButtonMenu* button_menu,
    const char* label,
    int32_t index,
    ButtonMenuItemCallback callback,
    ButtonMenuItemType type,
    void* callback_context);

ButtonMenu* button_menu_alloc(void);

void button_menu_free(ButtonMenu* button_menu);

void button_menu_set_header(ButtonMenu* button_menu, const char* header);

void button_menu_set_selected_item(ButtonMenu* button_menu, uint32_t index);

#ifdef __cplusplus
}
#endif
