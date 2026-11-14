#include "weave_packages.h"
#include <string.h>

static bool weave_packages_back(void* context) {
    WeavePackageApp* app = context;
    /* Ne pas quitter le flux Install tant que le companion travaille. */
    if(app->catalog_install_request[0] &&
       app->catalog_install_state != WeaveInstallFailed) {
        if(app->page != WeavePackagePageCatalogInstall) {
            app->page = WeavePackagePageCatalogInstall;
            view_dispatcher_switch_to_view(app->dispatcher, 6);
        }
        return true;
    }
    switch(app->page) {
    case WeavePackagePageMain:
        view_dispatcher_stop(app->dispatcher);
        break;
    case WeavePackagePageCatalogSearch:
        app->page = WeavePackagePageCatalog;
        view_dispatcher_switch_to_view(app->dispatcher, 5);
        break;
    case WeavePackagePageCatalogInstall:
        if(app->catalog_install_remote) {
            app->catalog_install_remote = false;
            app->catalog_install_request[0] = 0;
            weave_packages_main(app);
        } else {
            app->page = WeavePackagePageCatalogDetail;
            view_dispatcher_switch_to_view(app->dispatcher, 3);
        }
        break;
    case WeavePackagePageCatalogDetail:
        app->page = WeavePackagePageCatalog;
        view_dispatcher_switch_to_view(app->dispatcher, 5);
        break;
    case WeavePackagePageCatalog:
        weave_packages_main(app);
        break;
    case WeavePackagePageCategories:
        weave_packages_main(app);
        break;
    case WeavePackagePageList:
        if(app->mode == WeavePackageBrowse) {
            weave_packages_categories(app, false);
        } else {
            weave_packages_main(app);
        }
        break;
    case WeavePackagePageActions:
    case WeavePackagePageConfirm:
        weave_packages_list(app);
        break;
    case WeavePackagePageDestination:
        weave_packages_list(app);
        break;
    case WeavePackagePageStorage:
        weave_packages_categories(app, true);
        break;
    }
    return true;
}

static void weave_packages_tick(void* context) {
    WeavePackageApp* app = context;
    if(app->pending_list_refresh) {
        app->pending_list_refresh = false;
        if(app->page == WeavePackagePageList) weave_packages_list(app);
    }
    /* Detect remote requests even if Weave Apps was already open on Flipper. */
    if(++app->catalog_remote_watch_ticks >= 5) {
        app->catalog_remote_watch_ticks = 0;
        weave_packages_catalog_remote_install_watch(app, true);
    }
    weave_packages_catalog_install_tick(app);
    /* Acknowledge ONLY inside the running GUI event loop, after the first
     * native tick. An App.Start RPC response alone does not prove visibility. */
    if(app->catalog_install_remote && !app->catalog_remote_ready_sent &&
       app->catalog_install_request[0])
        weave_packages_catalog_remote_ready(app);
    if(app->catalog_install_backlight_locked &&
       app->catalog_install_state == WeaveInstallFailed) {
        notification_message(app->notification, &sequence_display_backlight_enforce_auto);
        app->catalog_install_backlight_locked = false;
    }
    /* La page de progression reste la seule page accessible pendant l'install. */
    if(app->catalog_install_request[0] &&
       app->catalog_install_state != WeaveInstallDone &&
       app->catalog_install_state != WeaveInstallFailed &&
       app->page != WeavePackagePageCatalogInstall) {
        app->page = WeavePackagePageCatalogInstall;
        view_dispatcher_switch_to_view(app->dispatcher, 6);
    }
}

int32_t weave_packages_app(void* context) {
    bool remote_install = context && !strcmp((const char*)context, "weave-install");
    WeavePackageApp* app = malloc(sizeof(WeavePackageApp));
    furi_check(app);
    memset(app, 0, sizeof(*app));
    app->gui = furi_record_open(RECORD_GUI);
    app->notification = furi_record_open(RECORD_NOTIFICATION);
    app->storage = furi_record_open(RECORD_STORAGE);
    weave_packages_catalog_migrate_storage(app);
    app->entries_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(app->entries_mutex);
    app->dialogs = furi_record_open(RECORD_DIALOGS);
    app->path = furi_string_alloc();
    app->menu = submenu_alloc();
    app->catalog_search_input = text_input_alloc();
    app->categories = menu_alloc();
    app->fap_list = view_alloc();
    view_set_context(app->fap_list, app);
    view_allocate_model(app->fap_list, ViewModelTypeLockFree, sizeof(WeavePackageApp*));
    WeavePackageApp** list_model = view_get_model(app->fap_list);
    *list_model = app;
    view_commit_model(app->fap_list, false);
    view_set_draw_callback(app->fap_list, weave_packages_list_draw);
    view_set_input_callback(app->fap_list, weave_packages_list_input);
    app->catalog_view = view_alloc();
    view_set_context(app->catalog_view, app);
    view_allocate_model(app->catalog_view, ViewModelTypeLockFree, sizeof(WeavePackageApp*));
    WeavePackageApp** shop_model = view_get_model(app->catalog_view);
    *shop_model = app;
    view_commit_model(app->catalog_view, false);
    view_set_draw_callback(app->catalog_view, weave_packages_catalog_draw);
    view_set_input_callback(app->catalog_view, weave_packages_catalog_input);
    app->catalog_detail = view_alloc();
    view_set_context(app->catalog_detail, app);
    view_allocate_model(app->catalog_detail, ViewModelTypeLockFree, sizeof(WeavePackageApp*));
    WeavePackageApp** detail_model = view_get_model(app->catalog_detail);
    *detail_model = app;
    view_commit_model(app->catalog_detail, false);
    view_set_draw_callback(app->catalog_detail, weave_packages_catalog_detail_draw);
    view_set_input_callback(app->catalog_detail, weave_packages_catalog_detail_input);
    app->catalog_install = view_alloc();
    view_set_context(app->catalog_install, app);
    view_allocate_model(app->catalog_install, ViewModelTypeLockFree, sizeof(WeavePackageApp*));
    WeavePackageApp** install_model = view_get_model(app->catalog_install);
    *install_model = app;
    view_commit_model(app->catalog_install, false);
    view_set_draw_callback(app->catalog_install, weave_packages_catalog_install_draw);
    view_set_input_callback(app->catalog_install, weave_packages_catalog_install_input);
    app->dispatcher = view_dispatcher_alloc();
    view_dispatcher_attach_to_gui(app->dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_set_event_callback_context(app->dispatcher, app);
    view_dispatcher_set_navigation_event_callback(app->dispatcher, weave_packages_back);
    view_dispatcher_set_tick_event_callback(app->dispatcher, weave_packages_tick, 200);
    view_dispatcher_add_view(app->dispatcher, 0, submenu_get_view(app->menu));
    view_dispatcher_add_view(app->dispatcher, 1, menu_get_view(app->categories));
    view_dispatcher_add_view(app->dispatcher, 2, app->fap_list);
    view_dispatcher_add_view(app->dispatcher, 3, app->catalog_detail);
    view_dispatcher_add_view(app->dispatcher, 4, text_input_get_view(app->catalog_search_input));
    view_dispatcher_add_view(app->dispatcher, 5, app->catalog_view);
    view_dispatcher_add_view(app->dispatcher, 6, app->catalog_install);
    weave_packages_main(app);
    if(remote_install) {
        weave_packages_catalog_remote_install_init(app);
    } else {
        /* Ignore any stale request present before a local app launch. */
        weave_packages_catalog_remote_install_watch(app, false);
    }
    view_dispatcher_run(app->dispatcher);
    furi_check(furi_mutex_acquire(app->entries_mutex, FuriWaitForever) == FuriStatusOk);
    weave_packages_clear_entries(app);
    furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
    if(app->catalog_install_backlight_locked)
        notification_message(app->notification, &sequence_display_backlight_enforce_auto);
    view_dispatcher_remove_view(app->dispatcher, 6);
    view_dispatcher_remove_view(app->dispatcher, 5);
    view_dispatcher_remove_view(app->dispatcher, 4);
    view_dispatcher_remove_view(app->dispatcher, 3);
    view_dispatcher_remove_view(app->dispatcher, 2);
    view_dispatcher_remove_view(app->dispatcher, 1);
    view_dispatcher_remove_view(app->dispatcher, 0);
    view_dispatcher_free(app->dispatcher);
    text_input_free(app->catalog_search_input);
    view_free(app->catalog_view);
    view_free(app->catalog_install);
    view_free(app->catalog_detail);
    view_free(app->fap_list);
    menu_free(app->categories);
    submenu_free(app->menu);
    furi_string_free(app->path);
    /* All native installation views have been removed. Confirm completion
     * separately from the RPC verification and the animated 100% frame. */
    weave_packages_catalog_remote_complete(app);
    furi_record_close(RECORD_DIALOGS);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_mutex_free(app->entries_mutex);
    free(app);
    return 0;
}
