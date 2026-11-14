#include "../archive_i.h"
#include "../helpers/archive_browser.h"
#include "../helpers/archive_search_match.h"
#include <toolbox/dir_walk.h>
#include <toolbox/path.h>
#include <storage/storage.h>

#define ARCHIVE_SEARCH_START 0U
#define ARCHIVE_SEARCH_DONE  1U
#define ARCHIVE_SEARCH_RETRY 2U

static void archive_search_clear(ArchiveApp* app) {
    for(uint8_t i = 0; i < app->search.count; ++i) {
        furi_string_free(app->search.matches[i]);
        app->search.matches[i] = NULL;
    }
    app->search.count = 0;
    app->search.scanned = 0;
    app->search.truncated = false;
    app->search.error = false;
    app->search.cancel = false;
}

static int32_t archive_search_worker(void* context) {
    ArchiveApp* app = context;
    ArchiveSearch* search = &app->search;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    DirWalk* walk = dir_walk_alloc(storage);
    FuriString* path = furi_string_alloc();
    FileInfo info;
    if(!dir_walk_open(walk, app->search_root)) {
        search->error = true;
    } else {
        DirWalkResult status = DirWalkOK;
        while(!search->cancel && (status = dir_walk_read(walk, path, &info)) == DirWalkOK) {
            search->scanned++;
            /* Reject very deep names rather than retaining unbounded paths in RAM. */
            if(furi_string_size(path) > 255) continue;
            const char* full = furi_string_get_cstr(path);
            const char* leaf = strrchr(full, '/');
            leaf = leaf ? leaf + 1 : full;
            if(archive_search_match(leaf, app->text_store)) {
                search->matches[search->count++] = furi_string_alloc_set(path);
                if(search->count == ARCHIVE_SEARCH_MAX_MATCHES) {
                    search->truncated = true;
                    break;
                }
            }
            if(search->scanned >= ARCHIVE_SEARCH_MAX_ENTRIES) {
                search->truncated = true;
                break;
            }
        }
        if(status == DirWalkError && !search->cancel) search->error = true;
        dir_walk_close(walk);
    }
    furi_string_free(path);
    dir_walk_free(walk);
    furi_record_close(RECORD_STORAGE);
    if(!search->cancel)
        view_dispatcher_send_custom_event(app->view_dispatcher, ARCHIVE_SEARCH_DONE);
    return 0;
}

static void archive_search_input_callback(void* context) {
    ArchiveApp* app = context;
    view_dispatcher_send_custom_event(app->view_dispatcher, ARCHIVE_SEARCH_START);
}

static void archive_search_selected(void* context, uint32_t index) {
    ArchiveApp* app = context;
    if(index == ARCHIVE_SEARCH_MAX_MATCHES) {
        view_dispatcher_send_custom_event(app->view_dispatcher, ARCHIVE_SEARCH_RETRY);
        return;
    }
    if(index >= app->search.count) return;
    FuriString* result = app->search.matches[index];
    FuriString* parent = furi_string_alloc();
    FileInfo info;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    bool exists = storage_common_stat(storage, furi_string_get_cstr(result), &info) == FSE_OK;
    bool is_folder = exists && file_info_is_dir(&info);
    furi_record_close(RECORD_STORAGE);
    if(!exists) {
        archive_notify(false);
        furi_string_free(parent);
        return;
    }
    if(is_folder) {
        furi_string_set(parent, result);
    } else {
        path_extract_dirname(furi_string_get_cstr(result), parent);
    }
    archive_open_storage(app->browser, furi_string_get_cstr(parent));
    furi_string_free(parent);
    scene_manager_next_scene(app->scene_manager, ArchiveAppSceneBrowser);
}

static void archive_search_show_results(ArchiveApp* app) {
    submenu_reset(app->roots);
    submenu_set_header(app->roots, app->search.error ? "Search interrupted" :
        app->search.truncated ? "Results (limited)" : "Search results");
    for(uint8_t i = 0; i < app->search.count; ++i) {
        const char* path = furi_string_get_cstr(app->search.matches[i]);
        submenu_add_item(app->roots, path, i, archive_search_selected, app);
    }
    if(app->search.count == 0)
        submenu_add_item(app->roots, "No matches", ARCHIVE_SEARCH_MAX_MATCHES + 1,
            archive_search_selected, app);
    submenu_add_item(app->roots, "Search again", ARCHIVE_SEARCH_MAX_MATCHES,
        archive_search_selected, app);
    view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewRoots);
}

static void archive_search_show_input(ArchiveApp* app) {
    app->text_store[0] = '\0';
    text_input_reset(app->text_input);
    text_input_set_header_text(app->text_input, "Search filename:");
    text_input_set_result_callback(app->text_input, archive_search_input_callback,
        app, app->text_store, 32, false);
    view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewTextInput);
}

void archive_scene_search_on_enter(void* context) {
    ArchiveApp* app = context;
    archive_search_clear(app);
    archive_search_show_input(app);
}

bool archive_scene_search_on_event(void* context, SceneManagerEvent event) {
    ArchiveApp* app = context;
    if(event.type != SceneManagerEventTypeCustom) return false;
    if(event.event == ARCHIVE_SEARCH_START) {
        if(app->search.thread || !app->text_store[0]) return true;
        widget_reset(app->widget);
        widget_add_string_element(app->widget, 64, 30, AlignCenter, AlignCenter,
            FontPrimary, "Searching...");
        view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewWidget);
        app->search.thread = furi_thread_alloc_ex("FileSearch", 2048, archive_search_worker, app);
        furi_thread_start(app->search.thread);
        return true;
    }
    if(event.event == ARCHIVE_SEARCH_DONE) {
        if(!app->search.thread) return true;
        furi_thread_join(app->search.thread);
        furi_thread_free(app->search.thread);
        app->search.thread = NULL;
        archive_search_show_results(app);
        return true;
    }
    if(event.event == ARCHIVE_SEARCH_RETRY) {
        archive_search_clear(app);
        archive_search_show_input(app);
        return true;
    }
    return false;
}

void archive_scene_search_on_exit(void* context) {
    ArchiveApp* app = context;
    if(app->search.thread) {
        app->search.cancel = true;
        furi_thread_join(app->search.thread);
        furi_thread_free(app->search.thread);
        app->search.thread = NULL;
    }
    submenu_reset(app->roots);
    widget_reset(app->widget);
    text_input_reset(app->text_input);
    archive_search_clear(app);
}
