

#pragma once

#include <gui/modules/widget.h>

#include "nfc/nfc_app.h"
#include "nfc/nfc_app_i.h"

/**
 * @brief Common submenu indices.
 */
enum {
    SubmenuIndexCommonSave,
    SubmenuIndexCommonEmulate, /**< Emulate menu option. */
    SubmenuIndexCommonWrite,
    SubmenuIndexCommonEdit, /**< Edit menu option. */
    SubmenuIndexCommonInfo, /**< Info menu option. */
    SubmenuIndexCommonRename, /**< Rename menu option. */
    SubmenuIndexCommonDelete, /**< Delete menu option. */
    SubmenuIndexCommonRestore, /**< Restore menu option. */
    SubmenuIndexCommonMax, /**< Special value, internal use. */
};

void nfc_protocol_support_common_submenu_callback(void* context, uint32_t index);

void nfc_protocol_support_common_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context);

void nfc_protocol_support_common_byte_input_done_callback(void* context);

void nfc_protocol_support_common_text_input_done_callback(void* context);

void nfc_protocol_support_common_on_enter_empty(NfcApp* instance);

/* Handler vide renvoie toujours true */

bool nfc_protocol_support_common_on_event_empty(NfcApp* instance, SceneManagerEvent event);
