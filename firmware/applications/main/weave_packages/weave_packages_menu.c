#include "weave_packages.h"
#include <assets_icons.h>
#include <loader/loader.h>
#include <string.h>

const char* const weave_fap_category_names[WEAVE_FAP_CATEGORY_COUNT] = {
    "USB", "GPIO", "Tools", "Bluetooth", "NFC", "RFID",
    "Sub-GHz", "Infrared", "iButton", "Games", "Other",
};

/* The icons are all 16x16: consistent visual weight and true category meaning. */
static const Icon* const weave_category_icons[WEAVE_FAP_CATEGORY_COUNT] = {
    &I_weave_cat_usb_16x16,
    &I_weave_cat_gpio_16x16,
    &I_weave_cat_tools_16x16,
    &I_weave_cat_bluetooth_16x16,
    &I_weave_cat_nfc_16x16,
    &I_weave_cat_rfid_16x16,
    &I_weave_cat_subghz_16x16,
    &I_weave_cat_infrared_16x16,
    &I_weave_cat_ibutton_16x16,
    &I_weave_cat_games_16x16,
    &I_weave_cat_other_16x16,
};

/* The native category menu uses 16x16 artwork; compact rows use 10x10.
 * A catalog entry's remote category can contain aliases or mixed case. */
static const Icon* const weave_compact_category_icons[WEAVE_FAP_CATEGORY_COUNT] = {
    &I_weave_cat_usb_10x10,
    &I_weave_cat_gpio_10x10,
    &I_weave_cat_tools_10x10,
    &I_weave_cat_bluetooth_10x10,
    &I_weave_cat_nfc_10x10,
    &I_weave_cat_rfid_10x10,
    &I_weave_cat_subghz_10x10,
    &I_weave_cat_infrared_10x10,
    &I_weave_cat_ibutton_10x10,
    &I_weave_cat_games_10x10,
    &I_weave_cat_other_10x10,
};

static bool weave_category_contains(const char* text, const char* keyword) {
    if(!text) return false;
    for(const char* start = text; *start; ++start) {
        const char* a = start;
        const char* b = keyword;
        while(*a && *b) {
            const char ca = (*a >= 'A' && *a <= 'Z') ? *a + ('a' - 'A') : *a;
            const char cb = (*b >= 'A' && *b <= 'Z') ? *b + ('a' - 'A') : *b;
            if(ca != cb) break;
            ++a;
            ++b;
        }
        if(!*b) return true;
    }
    return false;
}

const Icon* weave_packages_category_icon(const char* category) {
    if(!category) return weave_compact_category_icons[WEAVE_FAP_CATEGORY_COUNT - 1];
    if(weave_category_contains(category, "sub-ghz") ||
       weave_category_contains(category, "subghz") ||
       weave_category_contains(category, "radio")) return weave_compact_category_icons[6];
    if(weave_category_contains(category, "ibutton") ||
       weave_category_contains(category, "onewire") ||
       weave_category_contains(category, "1-wire")) return weave_compact_category_icons[8];
    if(weave_category_contains(category, "infrared") ||
       weave_category_contains(category, "ir remote") ||
       !strcmp(category, "IR") || !strcmp(category, "ir"))
        return weave_compact_category_icons[7];
    if(weave_category_contains(category, "bluetooth") ||
       !strcmp(category, "BLE") || !strcmp(category, "ble"))
        return weave_compact_category_icons[3];
    if(weave_category_contains(category, "rfid") ||
       weave_category_contains(category, "125khz")) return weave_compact_category_icons[5];
    if(weave_category_contains(category, "nfc")) return weave_compact_category_icons[4];
    if(weave_category_contains(category, "gpio")) return weave_compact_category_icons[1];
    if(weave_category_contains(category, "usb")) return weave_compact_category_icons[0];
    if(weave_category_contains(category, "game")) return weave_compact_category_icons[9];
    if(weave_category_contains(category, "tool") ||
       weave_category_contains(category, "utility")) return weave_compact_category_icons[2];
    return weave_compact_category_icons[WEAVE_FAP_CATEGORY_COUNT - 1];
}

void weave_packages_main(WeavePackageApp* app) {
    app->page = WeavePackagePageMain;
    submenu_reset(app->menu);
    submenu_set_header(app->menu, "Apps");
    submenu_add_item(app->menu, "Installed FAPs", 0, weave_packages_select, app);
    submenu_add_item(app->menu, "Install from catalog", 1, weave_packages_select, app);
    submenu_add_item(app->menu, "Move FAP", 2, weave_packages_select, app);
    submenu_add_item(app->menu, "Uninstall FAP", 3, weave_packages_select, app);
    view_dispatcher_switch_to_view(app->dispatcher, 0);
}

void weave_packages_select(void* context, uint32_t action) {
    WeavePackageApp* app = context;
    if(action == 1) {
        app->catalog_offset = 0;
        app->catalog_selected = 0;
        app->catalog_scroll = 0;
        app->catalog_query[0] = '\0';
        weave_packages_catalog(app);
        return;
    }
    if(action > 3) return;
    app->mode = action == 2 ? WeavePackageMove :
        action == 3 ? WeavePackageRemove : WeavePackageBrowse;
    /* Browsing starts with categories; Move and Uninstall keep a flat FAP list. */
    if(app->mode == WeavePackageBrowse) {
        app->category = 0;
        app->category_selected = 0;
        weave_packages_categories(app, false);
    } else {
        weave_packages_list(app);
    }
}

void weave_packages_categories(WeavePackageApp* app, bool destination) {
    if(!destination) {
        /* The Installed FAPs view always starts at category level. Keep it in
         * the same explicit list view as FAP entries, not the destination menu. */
        app->mode = WeavePackageBrowse;
        app->page = WeavePackagePageCategories;
        if(app->category_selected >= WEAVE_FAP_CATEGORY_COUNT) app->category_selected = 0;
        view_dispatcher_switch_to_view(app->dispatcher, 2);
        view_commit_model(app->fap_list, true);
        return;
    }
    app->page = WeavePackagePageDestination;
    menu_reset(app->categories);
    for(uint32_t i = 0; i < WEAVE_FAP_CATEGORY_COUNT; i++) {
        menu_add_item(app->categories, weave_fap_category_names[i],
            weave_category_icons[i], i, weave_packages_category, app);
    }
    view_dispatcher_switch_to_view(app->dispatcher, 1);
}

void weave_packages_category(void* context, uint32_t category) {
    WeavePackageApp* app = context;
    if(category >= WEAVE_FAP_CATEGORY_COUNT) return;
    if(app->page == WeavePackagePageDestination) {
        app->destination_category = category;
        weave_packages_storage_menu(app);
    } else {
        app->category = category;
        weave_packages_list(app);
    }
}

void weave_packages_entry(void* context, uint32_t index) {
    WeavePackageApp* app = context;
    if(index >= app->entry_count) return;
    furi_string_set(app->path, app->entries[index]);
    if(app->mode == WeavePackageMove) {
        weave_packages_categories(app, true);
        return;
    }
    if(app->mode == WeavePackageRemove) {
        weave_packages_uninstall(app);
        return;
    }
    /* Selecting an installed app launches it immediately. */
    weave_packages_action(app, 0);
}

void weave_packages_action(void* context, uint32_t action) {
    WeavePackageApp* app = context;
    if(action == 0) {
        Loader* loader = furi_record_open(RECORD_LOADER);
        /* Detached launch owns a copy of the path, then the Apps UI exits. */
        loader_start_detached_with_gui_error(loader, furi_string_get_cstr(app->path), NULL);
        furi_record_close(RECORD_LOADER);
        view_dispatcher_stop(app->dispatcher);
    } else if(action == 1) {
        app->mode = WeavePackageMove;
        weave_packages_categories(app, true);
    } else if(action == 2) {
        app->mode = WeavePackageRemove;
        weave_packages_uninstall(app);
    }
}

void weave_packages_storage_menu(WeavePackageApp* app) {
    app->page = WeavePackagePageStorage;
    submenu_reset(app->menu);
    submenu_set_header(app->menu, "Storage destination");
    submenu_add_item(app->menu, "SD Card", 1, weave_packages_storage_choice, app);
    submenu_add_item(app->menu, "Internal Storage", 0, weave_packages_storage_choice, app);
    view_dispatcher_switch_to_view(app->dispatcher, 0);
}

void weave_packages_storage_choice(void* context, uint32_t index) {
    WeavePackageApp* app = context;
    if(index > 1) return;
    app->destination_storage = index;
    WeavePackageResult result = weave_packages_store(app);
    switch(result) {
    case WeavePackageResultOk:
        weave_packages_notice(app, "FAP moved.");
        app->mode = WeavePackageBrowse;
        weave_packages_categories(app, false);
        return;
    case WeavePackageResultNoSpace:
        weave_packages_notice(app, "Not enough space.\nChoose another storage.");
        break;
    case WeavePackageResultUnavailable:
        weave_packages_notice(app, "Internal Storage is\nnot supported yet.");
        break;
    case WeavePackageResultExists:
        weave_packages_notice(app, "A FAP with this name\nalready exists here.");
        break;
    case WeavePackageResultCopiedOnly:
        weave_packages_notice(app, "Copied; original remains.\nRemove it manually.");
        weave_packages_main(app);
        return;
    default:
        weave_packages_notice(app, "Move failed.\nOriginal FAP preserved.");
        break;
    }
    weave_packages_storage_menu(app);
}
