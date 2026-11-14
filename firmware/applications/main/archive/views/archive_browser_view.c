#include "assets_icons.h"
#include <gui/weave_ui.h>
#include "toolbox/path.h"
#include <furi.h>
#include "archive_browser_view.h"
#include "../helpers/archive_browser.h"

#define SCROLL_INTERVAL (333)
#define SCROLL_DELAY    (2)

static const Icon* ArchiveItemIcons[] = {
    [ArchiveFileTypeIButton] = &I_ibutt_10px,
    [ArchiveFileTypeNFC] = &I_Nfc_10px,
    [ArchiveFileTypeSubGhz] = &I_sub1_10px,
    [ArchiveFileTypeLFRFID] = &I_125_10px,
    [ArchiveFileTypeInfrared] = &I_ir_10px,
    [ArchiveFileTypeBadUsb] = &I_badusb_10px,
    [ArchiveFileTypeU2f] = &I_u2f_10px,
    [ArchiveFileTypeSetting] = &I_settings_10px,
    [ArchiveFileTypeUpdateManifest] = &I_update_10px,
    [ArchiveFileTypeFolder] = &I_dir_10px,
    [ArchiveFileTypeUnknown] = &I_file_10px,
    [ArchiveFileTypeLoading] = &I_loading_10px,
    [ArchiveFileTypeApplication] = &I_file_10px,
    [ArchiveFileTypeJS] = &I_js_script_10px,
    [ArchiveFileTypeAppOrJs] = &I_file_10px,
};

void archive_browser_set_callback(
    ArchiveBrowserView* browser,
    ArchiveBrowserViewCallback callback,
    void* context) {
    furi_assert(browser);
    furi_assert(callback);
    browser->callback = callback;
    browser->context = context;
}

static void render_item_menu(Canvas* canvas, ArchiveBrowserViewModel* model) {
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_box(canvas, 71, 17, 57, 46);
    canvas_set_color(canvas, ColorBlack);
    elements_slightly_rounded_frame(canvas, 70, 16, 58, 48);

    int32_t file_index = model->item_idx - model->array_offset;
    if(file_index < 0 || (size_t)file_index >= files_array_size(model->files))
        return;
    ArchiveFile_t* selected = files_array_get(model->files, file_index);
    bool executable = archive_file_can_execute(selected);
    const char* basic[] = {"Rename", "Move", "Delete"};
    const char* extended[] = {"Execute", "Rename", "Move", "Delete"};
    size_t count = executable ? 4 : 3;
    /* Context actions only: no file sizes or placeholder dates. */
    canvas_set_font(canvas, FontSecondary);
    for(size_t i = 0; i < count; i++) {
        canvas_draw_str(canvas, 82, 27 + i * 11,
            executable ? extended[i] : basic[i]);
    }
    canvas_draw_icon(canvas, 74, 20 + model->menu_idx * 11, &I_ButtonRight_4x7);
}

static void archive_draw_loading(Canvas* canvas, ArchiveBrowserViewModel* model) {
    furi_assert(model);

    uint8_t x = 128 / 2 - 24 / 2;
    uint8_t y = 64 / 2 - 24 / 2;

    canvas_draw_icon(canvas, x, y, &A_Loading_24);
}

static void draw_list(Canvas* canvas, ArchiveBrowserViewModel* model) {
    furi_assert(model);
    const uint8_t start_y = model->show_parent ? 28 : 16;
    const uint8_t visible = model->show_parent ? 3 : 4;
    const bool scrollbar = model->item_cnt > visible;
    canvas_set_font(canvas, FontSecondary);

    for(uint32_t i = 0; i < MIN(model->item_cnt, visible); ++i) {
        FuriString* name = furi_string_alloc();
        const int32_t idx = (int32_t)i + model->list_offset;
        uint8_t x_offset = (model->move_fav && model->item_idx == idx) ? MOVE_OFFSET : 0;
        ArchiveFileTypeEnum type = ArchiveFileTypeLoading;
        uint8_t* custom_icon = NULL;
        if(archive_is_item_in_array(model, idx)) {
            ArchiveFile_t* file = files_array_get(model->files, idx - model->array_offset);
            type = file->type;
            if(type == ArchiveFileTypeApplication && file->custom_icon_data) {
                custom_icon = file->custom_icon_data;
                furi_string_set(name, file->custom_name);
            } else {
                if(type == ArchiveFileTypeApplication) type = ArchiveFileTypeUnknown;
                path_extract_filename(file->path, name, archive_file_can_execute(file));
            }
        } else {
            furi_string_set_str(name, "---");
        }

        size_t scroll = model->item_idx == idx && model->scroll_counter >= SCROLL_DELAY ?
            model->scroll_counter - SCROLL_DELAY : 0;
        const uint8_t y = start_y + i * FRAME_HEIGHT;
        if(model->item_idx == idx && !model->filter_focus &&
           !model->search_focus && !model->parent_focus) {
            canvas_set_color(canvas, ColorBlack);
            canvas_draw_rbox(canvas, 1 + x_offset, y, (scrollbar ? 122 : 125) - x_offset,
                FRAME_HEIGHT, 2);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_set_color(canvas, ColorBlack);
            scroll = 0;
        }
        if(custom_icon) {
            canvas_draw_bitmap(canvas, 2 + x_offset, y + 1, 11, 10, custom_icon);
        } else {
            canvas_draw_icon(canvas, 2 + x_offset, y + 1, ArchiveItemIcons[type]);
        }
        elements_scrollable_text_line(canvas, 15 + x_offset, y + 9,
            (scrollbar ? MAX_LEN_PX - 6 : MAX_LEN_PX) - x_offset,
            name, scroll, model->item_idx != idx || model->filter_focus ||
            model->search_focus || model->parent_focus);
        furi_string_free(name);
    }
    if(scrollbar) {
        canvas_set_color(canvas, ColorBlack);
        elements_scrollbar_pos(canvas, 126, start_y, visible * FRAME_HEIGHT,
            model->item_idx, model->item_cnt);
    }
}

static void archive_view_render(Canvas* canvas, void* mdl) {
    ArchiveBrowserViewModel* model = mdl;
    canvas_clear(canvas);
    /* Search is global across the SD card, not limited to the open directory. */
    if(model->search_focus) {
        canvas_draw_rbox(canvas, 0, 0, 128, 14, 2);
        canvas_set_color(canvas, ColorWhite);
    } else {
        canvas_set_color(canvas, ColorBlack);
        canvas_draw_rframe(canvas, 0, 0, 128, 14, 2);
    }
    canvas_draw_circle(canvas, 9, 6, 3);
    canvas_draw_line(canvas, 11, 8, 14, 11);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 21, 10, "...");
    canvas_set_color(canvas, ColorBlack);

    if(model->show_parent) {
        if(model->parent_focus) {
            canvas_draw_rbox(canvas, 1, 16, 125, 12, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 16, 25, "..");
        canvas_set_color(canvas, ColorBlack);
    }
    if(model->folder_loading) {
        /* Keep the global search visible during directory loading. */
        archive_draw_loading(canvas, model);
    } else if(model->item_cnt > 0) {
        draw_list(canvas, model);
    } else if(!model->show_parent) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 35, AlignCenter, AlignCenter, "Empty SD Card");
    }

    /* The full display is reserved for the fixed search bar and file rows. */
    if(model->menu) render_item_menu(canvas, model);
}

View* archive_browser_get_view(ArchiveBrowserView* browser) {
    furi_assert(browser);
    return browser->view;
}

static bool archive_view_input(InputEvent* event, void* context) {
    furi_assert(event);
    furi_assert(context);
    ArchiveBrowserView* browser = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat &&
       event->type != InputTypeLong) return true;
    const bool short_press = event->type == InputTypeShort;
    bool in_menu = false;
    with_view_model(browser->view, ArchiveBrowserViewModel * model,
        { in_menu = model->menu; }, false);

    if(in_menu) {
        if(!short_press) return true;
        if(event->key == InputKeyBack) {
            browser->callback(ArchiveBrowserEventFileMenuClose, browser->context);
        } else if(event->key == InputKeyUp || event->key == InputKeyDown) {
            with_view_model(browser->view, ArchiveBrowserViewModel * model, {
                ArchiveFile_t* file = archive_is_item_in_array(model, model->item_idx) ?
                    files_array_get(model->files, model->item_idx - model->array_offset) : NULL;
                const uint8_t count = archive_file_can_execute(file) ? 4 : 3;
                model->menu_idx = event->key == InputKeyUp ?
                    (model->menu_idx + count - 1) % count : (model->menu_idx + 1) % count;
            }, true);
        } else if(event->key == InputKeyOk) {
            uint8_t index = 0;
            with_view_model(browser->view, ArchiveBrowserViewModel * model,
                { index = model->menu_idx; }, false);
            ArchiveFile_t* file = archive_get_current_file(browser);
            if(file) browser->callback(archive_file_can_execute(file) ?
                file_menu_actions[index] : basic_menu_actions[index], browser->context);
        }
        return true;
    }

    if(event->key == InputKeyBack && short_press) {
        browser->callback(ArchiveBrowserEventExit, browser->context);
        return true;
    }

    bool search = false;
    bool parent = false;
    with_view_model(browser->view, ArchiveBrowserViewModel * model, {
        search = model->search_focus;
        parent = model->parent_focus;
    }, false);

    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        if(!short_press && event->type != InputTypeRepeat) return true;
        bool reload = false;
        with_view_model(browser->view, ArchiveBrowserViewModel * model, {
            if(event->key == InputKeyUp) {
                if(model->parent_focus) {
                    model->parent_focus = false;
                    model->search_focus = true;
                } else if(!model->search_focus) {
                    if(model->item_idx == 0) {
                        if(model->show_parent) model->parent_focus = true;
                        else model->search_focus = true;
                    } else {
                        model->item_idx--;
                    }
                }
            } else {
                if(model->search_focus) {
                    model->search_focus = false;
                    if(model->show_parent) model->parent_focus = true;
                    else if(model->item_cnt) model->item_idx = 0;
                } else if(model->parent_focus) {
                    model->parent_focus = false;
                    if(model->item_cnt) model->item_idx = 0;
                } else if(model->item_cnt && model->item_idx + 1 < (int32_t)model->item_cnt) {
                    model->item_idx++;
                }
            }
            if(!model->filter_focus && !model->parent_focus && !model->search_focus &&
               archive_is_file_list_load_required(model)) {
                model->list_loading = true;
                reload = true;
            }
            model->scroll_counter = 0;
        }, true);
        archive_update_offset(browser);
        if(reload) browser->callback(event->key == InputKeyUp ?
            ArchiveBrowserEventLoadPrevItems : ArchiveBrowserEventLoadNextItems,
            browser->context);
        return true;
    }

    if((event->key == InputKeyLeft || event->key == InputKeyRight) && short_press) {
        /* Left focuses global search; right returns to the file list.
         * No hidden/undiscoverable type-filter control. */
        with_view_model(browser->view, ArchiveBrowserViewModel * model, {
            model->search_focus = event->key == InputKeyLeft;
            model->filter_focus = false;
            model->parent_focus = false;
        }, true);
        return true;
    }
    if(event->key == InputKeyOk) {
        if(search) {
            if(short_press)
                browser->callback(ArchiveBrowserEventSearch, browser->context);
            else if(event->type == InputTypeLong)
                browser->callback(ArchiveBrowserEventCreateDir, browser->context);
        } else if(parent) {
            if(short_press) browser->callback(ArchiveBrowserEventExit, browser->context);
            else if(event->type == InputTypeLong)
                browser->callback(ArchiveBrowserEventCreateDir, browser->context);
        } else if(!search && !parent) {
            ArchiveFile_t* selected = archive_get_current_file(browser);
            if(!selected) return true;
            if(short_press) {
                if(selected->type == ArchiveFileTypeFolder)
                    browser->callback(ArchiveBrowserEventEnterDir, browser->context);
                else browser->callback(ArchiveBrowserEventFileMenuOpen, browser->context);
            } else if(event->type == InputTypeLong) {
                browser->callback(ArchiveBrowserEventFileMenuOpen, browser->context);
            }
        }
    }
    return true;
}

static void browser_scroll_timer(void* context) {
    furi_assert(context);
    ArchiveBrowserView* browser = context;
    with_view_model(
        browser->view, ArchiveBrowserViewModel * model, { model->scroll_counter++; }, true);
}

static void browser_view_enter(void* context) {
    furi_assert(context);
    ArchiveBrowserView* browser = context;
    with_view_model(
        browser->view, ArchiveBrowserViewModel * model, { model->scroll_counter = 0; }, true);
    furi_timer_start(browser->scroll_timer, SCROLL_INTERVAL);
}

static void browser_view_exit(void* context) {
    furi_assert(context);
    ArchiveBrowserView* browser = context;
    furi_timer_stop(browser->scroll_timer);
}

ArchiveBrowserView* browser_alloc(void) {
    ArchiveBrowserView* browser = malloc(sizeof(ArchiveBrowserView));
    browser->worker = NULL;
    browser->worker_running = false;
    browser->callback = NULL;
    browser->context = NULL;
    browser->is_root = true;
    browser->reverse_navigation = false;
    browser->filter_idx = 0;
    browser->filter_refresh = false;
    browser->view = view_alloc();
    view_allocate_model(browser->view, ViewModelTypeLocking, sizeof(ArchiveBrowserViewModel));
    view_set_context(browser->view, browser);
    view_set_draw_callback(browser->view, archive_view_render);
    view_set_input_callback(browser->view, archive_view_input);
    view_set_enter_callback(browser->view, browser_view_enter);
    view_set_exit_callback(browser->view, browser_view_exit);

    browser->scroll_timer = furi_timer_alloc(browser_scroll_timer, FuriTimerTypePeriodic, browser);

    browser->path = furi_string_alloc_set(archive_get_default_path(TAB_DEFAULT));

    with_view_model(
        browser->view,
        ArchiveBrowserViewModel * model,
        {
            files_array_init(model->files);
            model->tab_idx = TAB_DEFAULT;
            model->parent_focus = false;
            model->show_parent = false;
            model->search_focus = false;
            model->filter_focus = false;
            model->filter_idx = 0;
        },
        true);

    return browser;
}

void browser_free(ArchiveBrowserView* browser) {
    furi_assert(browser);

    furi_timer_free(browser->scroll_timer);

    if(browser->worker_running) {
        file_browser_worker_free(browser->worker);
    }

    with_view_model(
        browser->view,
        ArchiveBrowserViewModel * model,
        { files_array_clear(model->files); },
        false);

    furi_string_free(browser->path);

    view_free(browser->view);
    free(browser);
}
