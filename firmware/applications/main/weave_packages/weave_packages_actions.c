#include "weave_packages.h"

void weave_packages_notice(WeavePackageApp* app, const char* text) {
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, "Weave Apps", 64, 5, AlignCenter, AlignTop);
    dialog_message_set_text(message, text, 64, 31, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, NULL, "OK", NULL);
    dialog_message_show(app->dialogs, message);
    dialog_message_free(message);
}

void weave_packages_uninstall(WeavePackageApp* app) {
    if(!app || !app->dialogs || !app->storage || !app->path ||
       !furi_string_size(app->path)) return;
    DialogMessage* dialog = dialog_message_alloc();
    dialog_message_set_header(dialog, "Uninstall app?", 64, 5, AlignCenter, AlignTop);
    dialog_message_set_text(dialog, "Remove selected FAP?", 64, 30, AlignCenter, AlignCenter);
    dialog_message_set_buttons(dialog, "Cancel", NULL, "Remove");
    bool confirmed = dialog_message_show(app->dialogs, dialog) == DialogMessageButtonRight;
    dialog_message_free(dialog);
    if(!confirmed) return;

    /* A separate decision is made about user data, after confirming the uninstall. */
    dialog = dialog_message_alloc();
    dialog_message_set_header(dialog, "App data", 64, 5, AlignCenter, AlignTop);
    dialog_message_set_text(dialog, "Keep app data?", 64, 30, AlignCenter, AlignCenter);
    dialog_message_set_buttons(dialog, "Delete", "Cancel", "Keep");
    DialogMessageButton choice = dialog_message_show(app->dialogs, dialog);
    dialog_message_free(dialog);
    if(choice != DialogMessageButtonLeft && choice != DialogMessageButtonRight) return;
    bool result = weave_packages_remove(app, choice == DialogMessageButtonLeft);
    weave_packages_notice(app, result ? "FAP uninstalled." : "Uninstall failed.");
    // Le refresh après input évite de libérer la liste pendant son callback.
    if(result) app->pending_list_refresh = true;
}
