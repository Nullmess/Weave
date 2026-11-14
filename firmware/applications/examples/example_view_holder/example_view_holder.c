/* TextBox scrollable ; Back pour sortie */

#include <gui/gui.h>
#include <gui/view_holder.h>
#include <gui/modules/text_box.h>

#include <api_lock.h>

static void example_view_holder_back_callback(void* context) {
    FuriApiLock exit_lock = context;

    api_lock_unlock(exit_lock);
}

int32_t example_view_holder_app(void* arg) {
    UNUSED(arg);

    Gui* gui = furi_record_open(RECORD_GUI);
    /* GUI accepte uniquement des ViewPorts */

    TextBox* text_box = text_box_alloc();

    text_box_set_text(
        text_box,
        "ViewHolder is being used\n"
        "to show this TextBox view.\n\n"
        "Scroll down to see more.\n\n\n"
        "Press \"Back\" to exit.");

    /* ViewHolder adapte View vers ViewPort pour GUI */

    ViewHolder* view_holder = view_holder_alloc();

    view_holder_attach_to_gui(view_holder, gui);

    view_holder_set_view(view_holder, text_box_get_view(text_box));

    /* Thread application maintenu actif pour conserver l’affichage */

    /* Exemple utilisant FuriApiLock ; event loop préférable en application */

    /* FuriApiLock initialement verrouillé */
    FuriApiLock exit_lock = api_lock_alloc_locked();
    /* Callback Back avec exit lock comme context */

    view_holder_set_back_callback(view_holder, example_view_holder_back_callback, exit_lock);

    /* Thread bloqué jusqu’au déverrouillage par callback Back */

    api_lock_wait_unlock_and_free(exit_lock);

    /* Retirer la vue avant suppression du ViewHolder */
    view_holder_set_view(view_holder, NULL);

    view_holder_free(view_holder);
    text_box_free(text_box);

    furi_record_close(RECORD_GUI);

    return 0;
}
