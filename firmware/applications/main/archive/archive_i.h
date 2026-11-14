#pragma once

#include "archive.h"
#include <stdint.h>
#include <stdatomic.h>
#include <furi.h>
#include <gui/gui_i.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>
#include <gui/modules/text_input.h>
#include <gui/modules/widget.h>
#include <gui/modules/submenu.h>
#include <loader/loader.h>
#include <notification/notification_app.h>
#include <notification/notification_messages.h>

#include "views/archive_browser_view.h"
#include "scenes/archive_scene.h"

typedef enum {
    ArchiveViewBrowser,
    ArchiveViewTextInput,
    ArchiveViewWidget,
    ArchiveViewRoots,
    ArchiveViewTotal,
} ArchiveViewEnum;

#define ARCHIVE_SEARCH_MAX_MATCHES 24U
#define ARCHIVE_SEARCH_MAX_ENTRIES 4096U

typedef struct {
    FuriThread* thread;
    atomic_bool cancel;
    bool truncated;
    bool error;
    uint32_t scanned;
    uint8_t count;
    FuriString* matches[ARCHIVE_SEARCH_MAX_MATCHES];
} ArchiveSearch;

struct ArchiveApp {
    Gui* gui;
    Loader* loader;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;
    ArchiveBrowserView* browser;
    TextInput* text_input;
    Widget* widget;
    Submenu* roots;
    FuriPubSubSubscription* loader_stop_subscription;
    FuriString* fav_move_str;
    ArchiveSearch search;
    const char* search_root;
    char text_store[MAX_NAME_LEN];
    char file_extension[MAX_EXT_LEN + 1];
};

void archive_notify(bool success);
