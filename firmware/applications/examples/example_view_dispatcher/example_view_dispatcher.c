/* Widget/Submenu : changement via actions, Back pour sortie */

#include <gui/gui.h>
#include <gui/view_dispatcher.h>

#include <gui/modules/widget.h>
#include <gui/modules/submenu.h>

typedef enum {
    ViewIndexWidget,
    ViewIndexSubmenu,
    ViewIndexCount,
} ViewIndex;

typedef enum {
    SubmenuIndexNothing,
    SubmenuIndexSwitchView,
} SubmenuIndex;

typedef struct {
    ViewDispatcher* view_dispatcher;
    Widget* widget;
    Submenu* submenu;
} ExampleViewDispatcherApp;

static bool example_view_dispatcher_app_navigation_callback(void* context) {
    furi_assert(context);
    ExampleViewDispatcherApp* app = context;

    view_dispatcher_stop(app->view_dispatcher);
    return true;
}

static bool example_view_dispatcher_app_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    ExampleViewDispatcherApp* app = context;
    /* Convention numérique des événements définie par l’application */
    /* Valeur événement utilisée comme index de vue */
    furi_assert(event < ViewIndexCount);

    view_dispatcher_switch_to_view(app->view_dispatcher, event);

    return true;
}

static void example_view_dispatcher_app_button_callback(
    GuiButtonType button_type,
    InputType input_type,
    void* context) {
    furi_assert(context);
    ExampleViewDispatcherApp* app = context;
    /* Changement de vue uniquement sur Center court */
    if(button_type == GuiButtonTypeCenter && input_type == InputTypeShort) {
        /* Changement Submenu via queue custom events */
        view_dispatcher_send_custom_event(app->view_dispatcher, ViewIndexSubmenu);
    }
}

static void example_view_dispatcher_app_submenu_callback(void* context, uint32_t index) {
    furi_assert(context);
    ExampleViewDispatcherApp* app = context;
    /* Changement uniquement sur item Switch View */
    if(index == SubmenuIndexSwitchView) {
        /* Changement Widget via queue custom events */
        view_dispatcher_send_custom_event(app->view_dispatcher, ViewIndexWidget);
    }
}

static ExampleViewDispatcherApp* example_view_dispatcher_app_alloc() {
    ExampleViewDispatcherApp* app = malloc(sizeof(ExampleViewDispatcherApp));

    Gui* gui = furi_record_open(RECORD_GUI);

    app->widget = widget_alloc();
    widget_add_string_multiline_element(
        app->widget, 64, 32, AlignCenter, AlignCenter, FontSecondary, "Press the Button below");
    widget_add_button_element(
        app->widget,
        GuiButtonTypeCenter,
        "Switch View",
        example_view_dispatcher_app_button_callback,
        app);

    app->submenu = submenu_alloc();
    submenu_add_item(app->submenu, "Do Nothing", SubmenuIndexNothing, NULL, NULL);
    submenu_add_item(
        app->submenu,
        "Switch View",
        SubmenuIndexSwitchView,
        example_view_dispatcher_app_submenu_callback,
        app);

    app->view_dispatcher = view_dispatcher_alloc();

    view_dispatcher_attach_to_gui(app->view_dispatcher, gui, ViewDispatcherTypeFullscreen);
    /* Enregistrement seul des vues : aucun affichage */
    /* Index unique requis pour chaque vue */
    view_dispatcher_add_view(app->view_dispatcher, ViewIndexWidget, widget_get_view(app->widget));
    view_dispatcher_add_view(
        app->view_dispatcher, ViewIndexSubmenu, submenu_get_view(app->submenu));

    view_dispatcher_set_custom_event_callback(
        app->view_dispatcher, example_view_dispatcher_app_custom_event_callback);
    /* Callback Back si événement non traité par la vue */

    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, example_view_dispatcher_app_navigation_callback);
    /* Context transmis aux callbacks */
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);

    return app;
}

static void example_view_dispatcher_app_free(ExampleViewDispatcherApp* app) {
    /* Retirer toutes les vues avant suppression du ViewDispatcher */

    view_dispatcher_remove_view(app->view_dispatcher, ViewIndexWidget);
    view_dispatcher_remove_view(app->view_dispatcher, ViewIndexSubmenu);

    view_dispatcher_free(app->view_dispatcher);

    widget_free(app->widget);
    submenu_free(app->submenu);

    furi_record_close(RECORD_GUI);

    free(app);
}

static void example_view_dispatcher_app_run(ExampleViewDispatcherApp* app) {

    view_dispatcher_switch_to_view(app->view_dispatcher, ViewIndexWidget);
    /* Bloquant jusqu’à view_dispatcher_stop */
    /* FuriEventLoop utilisée en interne */
    view_dispatcher_run(app->view_dispatcher);
}

int32_t example_view_dispatcher_app(void* arg) {
    UNUSED(arg);

    ExampleViewDispatcherApp* app = example_view_dispatcher_app_alloc();
    example_view_dispatcher_app_run(app);
    example_view_dispatcher_app_free(app);

    return 0;
}
