#pragma once

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/submenu.h>
#include <gui/modules/text_input.h>
#include <gui/modules/menu.h>
#include <gui/view.h>
#include <dialogs/dialogs.h>
#include <storage/storage.h>
#include <notification/notification_app.h>

#define WEAVE_FAP_ENTRY_LIMIT 48
#define WEAVE_FAP_CATEGORY_COUNT 11

/* The flash-backed /int filesystem is not implemented in this upstream tree. */
#define WEAVE_FAP_INTERNAL_AVAILABLE 0

typedef enum {
    WeavePackageBrowse,
    WeavePackageMove,
    WeavePackageRemove,
} WeavePackageMode;

typedef enum {
    WeavePackagePageMain,
    WeavePackagePageCatalog,
    WeavePackagePageCatalogDetail,
    WeavePackagePageCatalogInstall,
    WeavePackagePageCatalogSearch,
    WeavePackagePageCategories,
    WeavePackagePageList,
    WeavePackagePageActions,
    WeavePackagePageDestination,
    WeavePackagePageStorage,
    WeavePackagePageConfirm,
} WeavePackagePage;

typedef enum {
    WeavePackageResultOk,
    WeavePackageResultUnavailable,
    WeavePackageResultNoSpace,
    WeavePackageResultExists,
    WeavePackageResultCopiedOnly,
    WeavePackageResultFailed,
} WeavePackageResult;

typedef enum {
    WeaveInstallWaiting,
    WeaveInstallDownloading,
    WeaveInstallInstalling,
    WeaveInstallDone,
    WeaveInstallFailed,
} WeaveInstallState;

typedef struct {
    Gui* gui;
    NotificationApp* notification;
    bool catalog_install_backlight_locked;
    ViewDispatcher* dispatcher;
    Submenu* menu;
    Menu* categories;
    View* fap_list;
    View* catalog_detail;
    View* catalog_install;
    WeaveInstallState catalog_install_state;
    char catalog_install_request[112];
    char catalog_install_title[33];
    char catalog_install_id[81];
    char catalog_install_version[33];
    char catalog_install_path[128];
    uint32_t catalog_install_total_bytes;
    uint32_t catalog_install_last_bytes;
    uint32_t catalog_install_initial_bytes;
    bool catalog_install_new_file;
    bool catalog_install_remote;
    uint32_t catalog_install_transfer_tick;
    uint32_t catalog_install_eta_seconds;
    uint8_t catalog_install_percent;
    bool catalog_install_percent_valid;
    uint32_t catalog_install_ticks;
    uint32_t catalog_install_last_activity_tick;
    uint8_t catalog_install_spinner;
    uint8_t catalog_install_visual_percent;
    uint8_t catalog_install_done_ticks;
    View* catalog_view;
    uint8_t catalog_selected;  /* 0 = search; 1..N = selected app in cache */
    uint8_t catalog_scroll;    /* first visible app in current cache window */
    bool catalog_has_more;
    TextInput* catalog_search_input;
    char catalog_query[65];
    char catalog_query_draft[65];
    uint32_t catalog_detail_index;
    bool catalog_detail_installed;
    bool catalog_detail_update;
    char catalog_detail_installed_path[160];
    uint8_t catalog_detail_offset;
    DialogsApp* dialogs;
    Storage* storage;
    /* Protect entries shared between the GUI renderer and dispatcher worker. */
    FuriMutex* entries_mutex;
    FuriString* path;
    FuriString* entries[WEAVE_FAP_ENTRY_LIMIT];
    FuriString* labels[WEAVE_FAP_ENTRY_LIMIT];
    FuriString* catalog_descriptions[WEAVE_FAP_ENTRY_LIMIT];
    FuriString* catalog_versions[WEAVE_FAP_ENTRY_LIMIT];
    uint32_t catalog_offset;
    uint8_t* icons[WEAVE_FAP_ENTRY_LIMIT];
    uint8_t entry_category[WEAVE_FAP_ENTRY_LIMIT];
    uint32_t entry_count;
    uint32_t category;
    uint32_t list_selected;
    uint32_t category_selected; /* installed-FAP categories, independent of FAP selection */
    uint32_t destination_category;
    uint32_t destination_storage;
    char catalog_remote_last_seen[112];
    uint8_t catalog_remote_watch_ticks;
    bool catalog_remote_ready_sent;
    char catalog_remote_completion_token[112];
    bool pending_list_refresh;
    WeavePackageMode mode;
    WeavePackagePage page;
} WeavePackageApp;

extern const char* const weave_fap_category_names[WEAVE_FAP_CATEGORY_COUNT];
const Icon* weave_packages_category_icon(const char* category);

void weave_packages_main(WeavePackageApp* app);
void weave_packages_categories(WeavePackageApp* app, bool destination);
void weave_packages_list(WeavePackageApp* app);
void weave_packages_list_draw(Canvas* canvas, void* model);
bool weave_packages_list_input(InputEvent* event, void* context);
void weave_packages_storage_menu(WeavePackageApp* app);
void weave_packages_storage_choice(void* context, uint32_t index);
void weave_packages_select(void* context, uint32_t action);
void weave_packages_category(void* context, uint32_t category);
void weave_packages_entry(void* context, uint32_t index);
void weave_packages_action(void* context, uint32_t action);
void weave_packages_uninstall(WeavePackageApp* app);
void weave_packages_notice(WeavePackageApp* app, const char* message);
void weave_packages_clear_entries(WeavePackageApp* app);
WeavePackageResult weave_packages_store(WeavePackageApp* app);
bool weave_packages_remove(WeavePackageApp* app, bool remove_data);
void weave_packages_shop_forget_path(WeavePackageApp* app, const char* path);
void weave_packages_shop_moved_path(WeavePackageApp* app, const char* old_path, const char* new_path);

void weave_packages_catalog(WeavePackageApp* app);
void weave_packages_catalog_draw(Canvas* canvas, void* model);
bool weave_packages_catalog_input(InputEvent* event, void* context);
void weave_packages_catalog_entry(void* context, uint32_t index);

void weave_packages_catalog_detail_draw(Canvas* canvas, void* model);
bool weave_packages_catalog_detail_input(InputEvent* event, void* context);

void weave_packages_catalog_install_draw(Canvas* canvas, void* model);
bool weave_packages_catalog_install_input(InputEvent* event, void* context);
void weave_packages_catalog_install_tick(WeavePackageApp* app);

/* Open the shared native install screen for an Android-initiated transfer. */
bool weave_packages_catalog_remote_install_init(WeavePackageApp* app);
void weave_packages_catalog_remote_install_watch(WeavePackageApp* app, bool accept_new);
void weave_packages_catalog_remote_ready(WeavePackageApp* app);
/* Emitted after the native progress GUI has closed and reached app teardown. */
void weave_packages_catalog_remote_complete(WeavePackageApp* app);
void weave_packages_catalog_migrate_storage(WeavePackageApp* app);
