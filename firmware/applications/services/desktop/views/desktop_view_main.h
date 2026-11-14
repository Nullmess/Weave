#pragma once

#include <gui/view.h>
#include "desktop_events.h"

typedef struct DesktopMainView DesktopMainView;

typedef void (*DesktopMainViewCallback)(DesktopEvent event, void* context);

void desktop_main_set_callback(
    DesktopMainView* main_view,
    DesktopMainViewCallback callback,
    void* context);

View* desktop_main_get_view(DesktopMainView* main_view);
void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode);
uint32_t desktop_main_get_selection(DesktopMainView* main_view);
void desktop_main_reset(DesktopMainView* main_view);
void desktop_main_refresh(DesktopMainView* main_view);
DesktopMainView* desktop_main_alloc(void);
void desktop_main_free(DesktopMainView* main_view);
