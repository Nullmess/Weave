#include "weave_packages.h"
#include <toolbox/path.h>
#include <flipper_application/flipper_application.h>
#include <assets_icons.h>
#include <gui/canvas.h>
#include <gui/weave_ui.h>
#include <stdio.h>
#include <string.h>

/* Caller holds entries_mutex while a GUI frame may be in progress. */
void weave_packages_clear_entries(WeavePackageApp* app) {
    const uint32_t count = app->entry_count;
    app->entry_count = 0;
    for(uint32_t i = 0; i < count; i++) {
        if(app->entries[i]) furi_string_free(app->entries[i]);
        if(app->labels[i]) furi_string_free(app->labels[i]);
        if(app->catalog_descriptions[i]) furi_string_free(app->catalog_descriptions[i]);
        app->catalog_descriptions[i] = NULL;
        if(app->catalog_versions[i]) furi_string_free(app->catalog_versions[i]);
        app->catalog_versions[i] = NULL;
        free(app->icons[i]);
        app->entries[i] = NULL;
        app->labels[i] = NULL;
        app->icons[i] = NULL;
    }
}

static void weave_packages_scan_folder(
    WeavePackageApp* app, const char* volume, const char* folder) {
    FuriString* dir = furi_string_alloc_printf("%s/apps/%s", volume, folder);
    File* handle = storage_file_alloc(app->storage);
    bool opened = storage_dir_open(handle, furi_string_get_cstr(dir));
    if(opened) {
        char name[128];
        FileInfo info;
        while(app->entry_count < WEAVE_FAP_ENTRY_LIMIT &&
              storage_dir_read(handle, &info, name, sizeof(name))) {
            const size_t len = strlen(name);
            if(!file_info_is_dir(&info) && len > 4 &&
               strcmp(name + len - 4, ".fap") == 0) {
                uint32_t index = app->entry_count;
                app->entry_category[index] = app->category;
                app->entries[index] = furi_string_alloc_printf(
                    "%s/%s", furi_string_get_cstr(dir), name);
                app->labels[index] = furi_string_alloc();
                app->icons[index] = calloc(32, 1);
                uint8_t* data = app->icons[index];
                bool loaded = data && flipper_application_load_name_and_icon(
                    app->entries[index], app->storage, &data, app->labels[index]);
                if(!loaded || !furi_string_size(app->labels[index])) {
                    path_extract_filename(app->entries[index], app->labels[index], true);
                }
                /* A FAP without an icon receives the stock fallback. */
                bool has_icon = false;
                if(loaded) for(uint8_t b = 0; b < 32; b++)
                    if(app->icons[index][b]) has_icon = true;
                if(!has_icon) {
                    free(app->icons[index]);
                    app->icons[index] = NULL;
                }
                app->entry_count++;
            }
        }
    }
    if(opened) storage_dir_close(handle);
    storage_file_free(handle);
    furi_string_free(dir);
}

static void weave_packages_scan(WeavePackageApp* app, const char* volume) {
    weave_packages_scan_folder(app, volume, weave_fap_category_names[app->category]);
    if(app->category == WEAVE_FAP_CATEGORY_COUNT - 1) {
        /* Older and third-party installers also use these directories.
         * They appear in Others without moving or renaming the FAPs. */
        weave_packages_scan_folder(app, volume, "Others");
        weave_packages_scan_folder(app, volume, "Media");
    }
}

/* Keep filesystem category IDs unchanged. Only their browsing order differs. */
static const uint8_t weave_packages_browse_order[WEAVE_FAP_CATEGORY_COUNT] = {
    0, 4, 7, 6, 5, 3, 2, 10, 9, 1, 8,
};

static uint32_t weave_packages_browse_category(uint32_t row) {
    return row < WEAVE_FAP_CATEGORY_COUNT ? weave_packages_browse_order[row] : 10;
}

void weave_packages_list_draw(Canvas* canvas, void* context) {
    WeavePackageApp* app = *(WeavePackageApp**)context;
    furi_check(furi_mutex_acquire(app->entries_mutex, FuriWaitForever) == FuriStatusOk);
    const bool show_categories = app->page == WeavePackagePageCategories;
    const uint32_t count = show_categories ? WEAVE_FAP_CATEGORY_COUNT : app->entry_count;
    const uint32_t selected = show_categories ? app->category_selected : app->list_selected;
    canvas_clear(canvas);
    weave_ui_header(canvas, show_categories ? "Installed FAPs" :
        app->mode == WeavePackageRemove ? "Uninstall FAP" :
        app->mode == WeavePackageMove ? "Move FAP" : "Installed FAPs");
    if(!count) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 4, 36, "No FAP in category");
        furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
        return;
    }
    uint32_t first = (selected / 4) * 4;
    canvas_set_font(canvas, FontSecondary);
    for(uint32_t row = 0; row < 4 && first + row < count; row++) {
        const uint32_t i = first + row;
        const int32_t y = 15 + row * 12;
        if(i == selected) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_rbox(canvas, 1, y, 125, 12, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        if(show_categories) {
            const uint32_t category = weave_packages_browse_category(i);
            canvas_draw_icon(canvas, 3, y + 1,
                weave_packages_category_icon(weave_fap_category_names[category]));
            canvas_draw_str(canvas, 18, y + 9,
                category == 10 ? "Others" : weave_fap_category_names[category]);
        } else {
            if(app->icons[i]) canvas_draw_bitmap(canvas, 3, y + 1, 11, 10, app->icons[i]);
            else canvas_draw_icon(canvas, 3, y + 1,
                weave_packages_category_icon(weave_fap_category_names[app->entry_category[i]]));
            canvas_draw_str(canvas, 18, y + 9, furi_string_get_cstr(app->labels[i]));
        }
        canvas_set_color(canvas, ColorBlack);
    }
    if(count > 4) {
        uint32_t bar_height = 48 * 4 / count;
        if(bar_height < 4) bar_height = 4;
        uint32_t bar_y = 15 + (48 - bar_height) * selected / (count - 1);
        canvas_draw_frame(canvas, 126, 15, 2, 48);
        canvas_draw_box(canvas, 126, bar_y, 2, bar_height);
    }
    furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
}

bool weave_packages_list_input(InputEvent* event, void* context) {
    WeavePackageApp* app = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return false;
    const bool show_categories = app->page == WeavePackagePageCategories;
    const uint32_t count = show_categories ? WEAVE_FAP_CATEGORY_COUNT : app->entry_count;
    uint32_t* selected = show_categories ? &app->category_selected : &app->list_selected;
    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        if(count) {
            if(event->key == InputKeyDown) *selected = (*selected + 1) % count;
            else *selected = (*selected + count - 1) % count;
            view_commit_model(app->fap_list, true);
        }
        return true;
    }
    if(event->key == InputKeyOk && event->type == InputTypeShort) {
        if(show_categories) {
            weave_packages_category(app, weave_packages_browse_category(*selected));
        } else if(app->entry_count) {
            weave_packages_entry(app, *selected);
        }
        return true;
    }
    return false;
}

void weave_packages_list(WeavePackageApp* app) {
    furi_check(furi_mutex_acquire(app->entries_mutex, FuriWaitForever) == FuriStatusOk);
    weave_packages_clear_entries(app);
    /* Browse only the selected category. Move and Uninstall show all FAPs. */
    if(app->mode == WeavePackageBrowse) {
        if(app->category < WEAVE_FAP_CATEGORY_COUNT) {
            weave_packages_scan(app, STORAGE_EXT_PATH_PREFIX);
#if WEAVE_FAP_INTERNAL_AVAILABLE
            weave_packages_scan(app, STORAGE_INT_PATH_PREFIX);
#endif
        }
    } else {
        for(uint32_t category = 0; category < WEAVE_FAP_CATEGORY_COUNT; category++) {
            app->category = category;
            weave_packages_scan(app, STORAGE_EXT_PATH_PREFIX);
#if WEAVE_FAP_INTERNAL_AVAILABLE
            weave_packages_scan(app, STORAGE_INT_PATH_PREFIX);
#endif
        }
    }
    app->list_selected = 0;
    app->page = WeavePackagePageList;
    furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
    view_dispatcher_switch_to_view(app->dispatcher, 2);
    /* Both category and FAP lists share view 2: a same-view switch does not
     * necessarily invalidate the frame. Force the new entries to be drawn. */
    view_commit_model(app->fap_list, true);
}

WeavePackageResult weave_packages_store(WeavePackageApp* app) {
    if(app->destination_storage == 0) {
#if !WEAVE_FAP_INTERNAL_AVAILABLE
        return WeavePackageResultUnavailable;
#endif
    }
    const char* volume = app->destination_storage == 0 ?
        STORAGE_INT_PATH_PREFIX : STORAGE_EXT_PATH_PREFIX;
    if(app->destination_category >= WEAVE_FAP_CATEGORY_COUNT) {
        return WeavePackageResultFailed;
    }
    const char* source = furi_string_get_cstr(app->path);
    if(strncmp(source, "/ext/apps/", 10) && strncmp(source, "/int/apps/", 10)) {
        return WeavePackageResultFailed;
    }
    const char* filename = strrchr(source, '/');
    if(!filename || !filename[1]) return WeavePackageResultFailed;
    filename++;
    FuriString* directory = furi_string_alloc_printf("%s/apps/%s", volume,
        weave_fap_category_names[app->destination_category]);
    FuriString* dest = furi_string_alloc_printf("%s/%s",
        furi_string_get_cstr(directory), filename);
    WeavePackageResult result = WeavePackageResultFailed;
    const char* target = furi_string_get_cstr(dest);
    if(strcmp(target, source) == 0) {
        result = WeavePackageResultOk;
    } else if(storage_common_exists(app->storage, target)) {
        result = WeavePackageResultExists;
    } else {
        FileInfo info;
        uint64_t total = 0;
        uint64_t free = 0;
        bool same_volume = strncmp(source, volume, 4) == 0;
        if(storage_common_stat(app->storage, source, &info) != FSE_OK ||
           file_info_is_dir(&info)) {
            result = WeavePackageResultFailed;
        } else if(!same_volume &&
           storage_common_fs_info(app->storage, volume, &total, &free) != FSE_OK) {
            result = WeavePackageResultUnavailable;
        } else if(!same_volume && info.size > free) {
            result = WeavePackageResultNoSpace;
        } else if(!storage_simply_mkdir(app->storage,
               app->destination_storage == 0 ? INT_PATH("apps") : EXT_PATH("apps")) ||
                  !storage_simply_mkdir(app->storage, furi_string_get_cstr(directory))) {
            result = WeavePackageResultFailed;
        } else if(same_volume) {
            result = storage_common_rename(app->storage, source, target) == FSE_OK ?
                WeavePackageResultOk : WeavePackageResultFailed;
        } else {
            FS_Error copy_status = storage_common_copy(app->storage, source, target);
            if(copy_status == FSE_OK) {
                result = storage_common_remove(app->storage, source) == FSE_OK ?
                    WeavePackageResultOk : WeavePackageResultCopiedOnly;
            } else {
                /* Never leave a truncated destination after a failed copy. */
                storage_common_remove(app->storage, target);
            }
        }
    }
    if(result == WeavePackageResultOk && strcmp(source, target))
        weave_packages_shop_moved_path(app, source, target);
    furi_string_free(dest);
    furi_string_free(directory);
    return result;
}

static bool weave_packages_valid_id(const char* name) {
    if(!name || !*name) return false;
    while(*name) {
        if(!((*name >= 'a' && *name <= 'z') ||
             (*name >= '0' && *name <= '9') || *name == '_' || *name == '-')) return false;
        name++;
    }
    return true;
}

bool weave_packages_remove(WeavePackageApp* app, bool remove_data) {
    if(!app || !app->path || !app->storage) return false;
    const char* selected = furi_string_get_cstr(app->path);
    if(!selected) return false;
    const size_t path_len = strlen(selected);
    if(path_len < 14 || path_len >= 192 ||
       (strncmp(selected, "/ext/apps/", 10) &&
        strncmp(selected, "/int/apps/", 10)) ||
       strcmp(selected + path_len - 4, ".fap") ||
       strstr(selected, "/../") || strstr(selected, "/./") ||
       strchr(selected, '\\')) return false;

    // Snapshot du path avant suppression et refresh de l'UI.
    char source[192];
    memcpy(source, selected, path_len + 1);
    FileInfo info;
    if(storage_common_stat(app->storage, source, &info) != FSE_OK ||
       file_info_is_dir(&info)) return false;

    FuriString* appid = furi_string_alloc();
    path_extract_filename(app->path, appid, true);
    const char* id = furi_string_get_cstr(appid);
    bool valid = id && *id && (!remove_data || weave_packages_valid_id(id));
    if(valid) valid = storage_common_remove(app->storage, source) == FSE_OK;
    if(valid) weave_packages_shop_forget_path(app, source);
    if(valid && remove_data) {
        // Limiter l'effacement aux données de cet app ID.
        const char* volume = strncmp(source, "/int", 4) == 0 ? "/int" : "/ext";
        FuriString* data = furi_string_alloc_printf("%s/apps_data/%s", volume, id);
        valid = !storage_common_exists(app->storage, furi_string_get_cstr(data)) ||
            storage_simply_remove_recursive(app->storage, furi_string_get_cstr(data));
        furi_string_free(data);
    }
    furi_string_free(appid);
    return valid;
}
