/**
 * @file submenu.h
 * GUI: SubMenu view module API
 */

#pragma once

#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Submenu anonymous structure */
typedef struct Submenu Submenu;
typedef void (*SubmenuItemCallback)(void* context, uint32_t index);
typedef void (*SubmenuItemCallbackEx)(void* context, InputType input_type, uint32_t index);

Submenu* submenu_alloc(void);

void submenu_free(Submenu* submenu);

View* submenu_get_view(Submenu* submenu);

/* Index callback peut être partagé par plusieurs items */

void submenu_add_item(
    Submenu* submenu,
    const char* label,
    uint32_t index,
    SubmenuItemCallback callback,
    void* callback_context);

/* Index callback peut être partagé ; events press étendus */

void submenu_add_item_ex(
    Submenu* submenu,
    const char* label,
    uint32_t index,
    SubmenuItemCallbackEx callback,
    void* callback_context);

void submenu_change_item_label(Submenu* submenu, uint32_t index, const char* label);

/* Index partagé : retirer uniquement premier item correspondant */

void submenu_remove_item(Submenu* submenu, uint32_t index);

void submenu_reset(Submenu* submenu);

uint32_t submenu_get_selected_item(Submenu* submenu);

void submenu_set_selected_item(Submenu* submenu, uint32_t index);

void submenu_set_header(Submenu* submenu, const char* header);

#ifdef __cplusplus
}
#endif
