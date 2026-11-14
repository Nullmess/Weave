#pragma once

#include "../helpers/archive_files.h"
#include "../helpers/archive_favorites.h"

#include <gui/gui_i.h>
#include <gui/view.h>
#include <gui/canvas.h>
#include <gui/elements.h>
#include <gui/modules/file_browser_worker.h>
#include <storage/storage.h>
#include <furi.h>

#define MAX_LEN_PX   110
#define MAX_NAME_LEN 255
#define MAX_EXT_LEN  6
#define FRAME_HEIGHT 12
#define MENU_ITEMS   4u
#define ARCHIVE_FILTER_COUNT 8u
#define MOVE_OFFSET  5u

typedef enum {
    ArchiveTabFavorites,
    ArchiveTabSubGhz,
    ArchiveTabLFRFID,
    ArchiveTabNFC,
    ArchiveTabInfrared,
    ArchiveTabIButton,
    ArchiveTabBadUsb,
    ArchiveTabU2f,
    ArchiveTabApplications,
    ArchiveTabBrowser,
    ArchiveTabTotal,
} ArchiveTabEnum;

typedef enum {
    ArchiveBrowserEventFileMenuOpen,
    ArchiveBrowserEventFileMenuClose,
    ArchiveBrowserEventFileMenuRun,
    ArchiveBrowserEventFileMenuMove,
    ArchiveBrowserEventFileMenuRename,
    ArchiveBrowserEventFileMenuDelete,

    ArchiveBrowserEventEnterDir,
    ArchiveBrowserEventCreateDir,

    ArchiveBrowserEventFavMoveUp,
    ArchiveBrowserEventFavMoveDown,
    ArchiveBrowserEventEnterFavMove,
    ArchiveBrowserEventExitFavMove,
    ArchiveBrowserEventSaveFavMove,

    ArchiveBrowserEventLoadPrevItems,
    ArchiveBrowserEventLoadNextItems,

    ArchiveBrowserEventListRefresh,
    ArchiveBrowserEventSearch,
    ArchiveBrowserEventFilterNext,
    ArchiveBrowserEventFilterPrev,

    ArchiveBrowserEventExit,
} ArchiveBrowserEvent;

static const uint8_t file_menu_actions[MENU_ITEMS] = {
    [0] = ArchiveBrowserEventFileMenuRun,
    [1] = ArchiveBrowserEventFileMenuRename,
    [2] = ArchiveBrowserEventFileMenuMove,
    [3] = ArchiveBrowserEventFileMenuDelete,
};

static const uint8_t basic_menu_actions[3] = {
    [0] = ArchiveBrowserEventFileMenuRename,
    [1] = ArchiveBrowserEventFileMenuMove,
    [2] = ArchiveBrowserEventFileMenuDelete,
};

typedef struct ArchiveBrowserView ArchiveBrowserView;

typedef void (*ArchiveBrowserViewCallback)(ArchiveBrowserEvent event, void* context);

typedef enum {
    BrowserActionBrowse,
    BrowserActionItemMenu,
    BrowserActionTotal,
} BrowserActionEnum;

struct ArchiveBrowserView {
    View* view;
    BrowserWorker* worker;
    bool worker_running;
    ArchiveBrowserViewCallback callback;
    void* context;
    FuriString* path;
    InputKey last_tab_switch_dir;
    bool is_root;
    bool reverse_navigation;
    uint8_t filter_idx;
    bool filter_refresh;
    FuriTimer* scroll_timer;
};

typedef struct {
    ArchiveTabEnum tab_idx;
    files_array_t files;

    uint8_t menu_idx;
    bool menu;
    bool move_fav;
    bool list_loading;
    bool folder_loading;
    bool parent_focus;
    bool show_parent;
    bool search_focus;
    bool filter_focus;
    uint8_t filter_idx;

    uint32_t item_cnt;
    int32_t item_idx;
    int32_t array_offset;
    int32_t list_offset;
    size_t scroll_counter;

    uint32_t button_held_for_ticks;
} ArchiveBrowserViewModel;

void archive_browser_set_callback(
    ArchiveBrowserView* browser,
    ArchiveBrowserViewCallback callback,
    void* context);

View* archive_browser_get_view(ArchiveBrowserView* browser);

ArchiveBrowserView* browser_alloc(void);
void browser_free(ArchiveBrowserView* browser);
