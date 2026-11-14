/* Vue ajoutée au dispatcher interdite simultanément dans ViewStack */

#pragma once

#include "view.h"
#include "gui.h"
#include "scene_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ViewDispatcherTypeDesktop, /* Desktop interne : fullscreen avec status bar */
    ViewDispatcherTypeWindow, /* Window avec status bar */
    ViewDispatcherTypeFullscreen /* Fullscreen sans status bar */
} ViewDispatcherType;

typedef struct ViewDispatcher ViewDispatcher;

typedef bool (*ViewDispatcherCustomEventCallback)(void* context, uint32_t event);

typedef bool (*ViewDispatcherNavigationEventCallback)(void* context);

typedef void (*ViewDispatcherTickEventCallback)(void* context);

ViewDispatcher* view_dispatcher_alloc(void);

/* Event loop externe : libération à charge appelant */

ViewDispatcher* view_dispatcher_alloc_ex(FuriEventLoop* loop);

/* Retirer toutes les vues avant free */

void view_dispatcher_free(ViewDispatcher* view_dispatcher);

/* Obsolète : queue toujours active ; retirer les appels */

FURI_DEPRECATED void view_dispatcher_enable_queue(ViewDispatcher* view_dispatcher);

void view_dispatcher_send_custom_event(ViewDispatcher* view_dispatcher, uint32_t event);

/* Thread run ; callback si event custom non consommé par vue */

void view_dispatcher_set_custom_event_callback(
    ViewDispatcher* view_dispatcher,
    ViewDispatcherCustomEventCallback callback);

/* Thread run ; Short Back non consommé par vue */

void view_dispatcher_set_navigation_event_callback(
    ViewDispatcher* view_dispatcher,
    ViewDispatcherNavigationEventCallback callback);

/* Tick exige loop owned via alloc ; callback dans thread run */

void view_dispatcher_set_tick_event_callback(
    ViewDispatcher* view_dispatcher,
    ViewDispatcherTickEventCallback callback,
    uint32_t tick_period);

void view_dispatcher_set_event_callback_context(ViewDispatcher* view_dispatcher, void* context);

/* Ne pas appeler event_loop_run ; dispatcher_run le gère */

FuriEventLoop* view_dispatcher_get_event_loop(ViewDispatcher* view_dispatcher);

/* Bloque jusqu’à stop ou signal FuriSignalExit */

void view_dispatcher_run(ViewDispatcher* view_dispatcher);

void view_dispatcher_stop(ViewDispatcher* view_dispatcher);

bool view_dispatcher_check_id(ViewDispatcher* view_dispatcher, uint32_t view_id);

void view_dispatcher_add_view(ViewDispatcher* view_dispatcher, uint32_t view_id, View* view);

void view_dispatcher_remove_view(ViewDispatcher* view_dispatcher, uint32_t view_id);

/* Switch différable jusqu’à complémentarité input events */

void view_dispatcher_switch_to_view(ViewDispatcher* view_dispatcher, uint32_t view_id);

void view_dispatcher_send_to_front(ViewDispatcher* view_dispatcher);

void view_dispatcher_send_to_back(ViewDispatcher* view_dispatcher);

void view_dispatcher_attach_to_gui(
    ViewDispatcher* view_dispatcher,
    Gui* gui,
    ViewDispatcherType type);

#ifdef __cplusplus
}
#endif
