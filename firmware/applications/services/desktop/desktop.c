#include "desktop_i.h"

#include <cli/cli_vcp.h>

#include <gui/gui_i.h>

#include <locale/locale.h>
#include <storage/storage.h>
#include <power/power_service/power.h>

#include <assets_icons.h>

#include "scenes/desktop_scene.h"
#include "scenes/desktop_scene_locked.h"

#define TAG "Desktop"

static void desktop_auto_lock_arm(Desktop*);
static void desktop_auto_lock_inhibit(Desktop*);
static void desktop_start_auto_lock_timer(Desktop*);
static void desktop_apply_settings(Desktop*);

static void desktop_loader_callback(const void* message, void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    const LoaderEvent* event = message;

    if(event->type == LoaderEventTypeApplicationBeforeLoad) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalBeforeAppStarted);
        furi_check(furi_semaphore_acquire(desktop->animation_semaphore, 3000) == FuriStatusOk);
    } else if(event->type == LoaderEventTypeNoMoreAppsInQueue) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalAfterAppFinished);
    }
}

static void desktop_storage_callback(const void* message, void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    const StorageEvent* event = message;

    if(event->type == StorageEventTypeCardMount) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalReloadSettings);
    }
}

static void desktop_lock_icon_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    furi_assert(canvas);
    canvas_draw_icon(canvas, 0, 0, &I_Lock_7x8);
}

static void desktop_dummy_mode_icon_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    furi_assert(canvas);
    canvas_draw_icon(canvas, 0, 0, &I_GameMode_11x8);
}

/* Global, nonblocking long-Back shortcut. PubSub events can originate from
 * the input timer thread, so we only queue an event here. GUI changes and
 * power operations happen on their respective service threads. */
static void desktop_power_menu_input_subscriber(const void* message, void* context) {
    Desktop* desktop = context;
    const InputEvent* event = message;
    if(event->key == InputKeyBack && event->type == InputTypeLong && !desktop->locked) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalPowerMenuOpen);
    }
}

/* Small 4x6 glyphs keep every power label inside a 42px tile.
 * Avoid U8G2 proportional fonts here: Poweroff was wider than its tile. */
static void desktop_power_label(Canvas* canvas, int32_t center, int32_t top, const char* label) {
    static const struct {
        char letter;
        uint8_t rows[6];
    } glyphs[] = {
        {'S', {0x7, 0x8, 0x6, 0x1, 0x1, 0xE}},
        {'l', {0x4, 0x4, 0x4, 0x4, 0x4, 0x2}},
        {'e', {0x0, 0x6, 0x9, 0xF, 0x8, 0x7}},
        {'p', {0x0, 0xE, 0x9, 0xE, 0x8, 0x8}},
        {'R', {0xE, 0x9, 0xE, 0xA, 0x9, 0x9}},
        {'b', {0x8, 0x8, 0xE, 0x9, 0x9, 0xE}},
        {'o', {0x0, 0x6, 0x9, 0x9, 0x9, 0x6}},
        {'t', {0x4, 0xE, 0x4, 0x4, 0x4, 0x3}},
        {'P', {0xE, 0x9, 0xE, 0x8, 0x8, 0x8}},
        {'w', {0x0, 0x9, 0x9, 0xB, 0xF, 0x9}},
        {'r', {0x0, 0xA, 0xD, 0x8, 0x8, 0x8}},
        {'f', {0x3, 0x4, 0xE, 0x4, 0x4, 0x4}},
    };
    size_t count = 0;
    while(label[count]) count++;
    /* Each glyph is 4px wide with 1px letter spacing. */
    const int32_t left = center - (int32_t)(count * 5U - 1U) / 2;
    for(size_t i = 0; i < count; i++) {
        for(size_t j = 0; j < COUNT_OF(glyphs); j++) {
            if(glyphs[j].letter != label[i]) continue;
            for(uint8_t row = 0; row < 6; row++) {
                for(uint8_t col = 0; col < 4; col++) {
                    if(glyphs[j].rows[row] & (1U << (3U - col))) {
                        canvas_draw_dot(canvas, left + (int32_t)i * 5 + col, top + row);
                    }
                }
            }
            break;
        }
    }
}

static void desktop_power_menu_draw(Canvas* canvas, void* context) {
    Desktop* desktop = context;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignBottom,
        desktop->power_menu_exit_error ? "Close app first" : "Power");

    static const char* const labels[3] = {"Sleep", "Reboot", "Poweroff"};
    for(uint8_t i = 0; i < 3; i++) {
        const int32_t x = (int32_t)i * 43;
        const int32_t cx = x + 21;
        const bool selected = i == desktop->power_menu_selected;
        canvas_set_color(canvas, ColorBlack);
        if(selected) {
            canvas_draw_rbox(canvas, x, 19, 42, 37, 3);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_draw_rframe(canvas, x, 19, 42, 37, 3);
        }
        if(i == 0) {
            /* Crescent moon. */
            canvas_draw_disc(canvas, cx, 31, 5);
            canvas_set_color(canvas, selected ? ColorBlack : ColorWhite);
            canvas_draw_disc(canvas, cx + 3, 29, 4);
            canvas_set_color(canvas, selected ? ColorWhite : ColorBlack);
        } else if(i == 1) {
            /* Built-in 18px asset rather than a fragile set of canvas lines. */
            canvas_draw_icon(canvas, cx - 9, 22, &I_WeaveReboot_18x18);
        } else {
            /* Poweroff ring with a gap for the vertical stem. */
            canvas_draw_circle(canvas, cx, 31, 5);
            canvas_set_color(canvas, selected ? ColorBlack : ColorWhite);
            canvas_draw_box(canvas, cx - 2, 26, 5, 3);
            canvas_set_color(canvas, selected ? ColorWhite : ColorBlack);
            canvas_draw_line(canvas, cx, 23, cx, 31);
        }
        desktop_power_label(canvas, cx, 45, labels[i]);
    }
    canvas_set_color(canvas, ColorBlack);
}

static void desktop_power_menu_input(InputEvent* event, void* context) {
    Desktop* desktop = context;
    if(event->type != InputTypeShort) return;
    if(event->key == InputKeyLeft) {
        desktop->power_menu_selected = (desktop->power_menu_selected + 2) % 3;
        view_port_update(desktop->power_menu_viewport);
    } else if(event->key == InputKeyRight) {
        desktop->power_menu_selected = (desktop->power_menu_selected + 1) % 3;
        view_port_update(desktop->power_menu_viewport);
    } else if(event->key == InputKeyUp) {
        desktop->power_menu_selected = 0;
        view_port_update(desktop->power_menu_viewport);
    } else if(event->key == InputKeyDown) {
        desktop->power_menu_selected = 2;
        view_port_update(desktop->power_menu_viewport);
    } else if(event->key == InputKeyBack) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalPowerMenuClose);
    } else if(event->key == InputKeyOk) {
        view_dispatcher_send_custom_event(desktop->view_dispatcher,
            desktop->power_menu_selected == 0 ? DesktopGlobalPowerMenuSleep :
            desktop->power_menu_selected == 1 ? DesktopGlobalPowerMenuReboot :
                                                DesktopGlobalPowerMenuOff);
    }
}

static void desktop_power_menu_close(Desktop* desktop) {
    desktop->power_menu_open = false;
    view_port_enabled_set(desktop->power_menu_viewport, false);
}

static void desktop_clock_update(Desktop* desktop) {
    furi_assert(desktop);

    DateTime curr_dt;
    furi_hal_rtc_get_datetime(&curr_dt);
    bool time_format_12 = locale_get_time_format() == LocaleTimeFormat12h;

    if(desktop->clock.hour != curr_dt.hour || desktop->clock.minute != curr_dt.minute ||
       desktop->clock.format_12 != time_format_12) {
        desktop->clock.format_12 = time_format_12;
        desktop->clock.hour = curr_dt.hour;
        desktop->clock.minute = curr_dt.minute;
        view_port_update(desktop->clock_viewport);
    }
}

static void desktop_clock_draw_callback(Canvas* canvas, void* context) {
    furi_assert(context);
    furi_assert(canvas);

    Desktop* desktop = context;

    canvas_set_font(canvas, FontPrimary);

    uint8_t hour = desktop->clock.hour;
    if(desktop->clock.format_12) {
        if(hour > 12) {
            hour -= 12;
        }
        if(hour == 0) {
            hour = 12;
        }
    }

    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02u:%02u", hour, desktop->clock.minute);

    view_port_set_width(
        desktop->clock_viewport,
        canvas_string_width(canvas, buffer) - 1 + (desktop->clock.minute % 10 == 1));

    canvas_draw_str_aligned(canvas, 0, 8, AlignLeft, AlignBottom, buffer);
}

static void desktop_stealth_mode_icon_draw_callback(Canvas* canvas, void* context) {
    UNUSED(context);
    furi_assert(canvas);
    canvas_draw_icon(canvas, 0, 0, &I_Muted_8x8);
}

static bool desktop_custom_event_callback(void* context, uint32_t event) {
    furi_assert(context);
    Desktop* desktop = (Desktop*)context;

    if(event == DesktopGlobalPowerMenuOpen) {
        if(!desktop->power_menu_open && !desktop->locked) {
            desktop->power_menu_selected = 0;
            desktop->power_menu_exit_error = false;
            desktop->power_menu_open = true;
            gui_view_port_send_to_front(desktop->gui, desktop->power_menu_viewport);
            view_port_enabled_set(desktop->power_menu_viewport, true);
        }
    } else if(event == DesktopGlobalPowerMenuClose) {
        desktop_power_menu_close(desktop);
    } else if(event == DesktopGlobalPowerMenuSleep) {
        /* Ask active FAPs and native WEAVE screens to close gracefully.
         * Apps that do not handle FuriSignalExit cannot be killed safely. */
        bool accepted = true;
        if(loader_is_locked(desktop->loader)) {
            accepted = loader_signal(desktop->loader, FuriSignalExit, NULL);
        }
        if(furi_thread_get_state(desktop->scene_thread) != FuriThreadStateStopped) {
            accepted = furi_thread_signal(desktop->scene_thread, FuriSignalExit, NULL) && accepted;
        }
        if(accepted) {
            desktop_main_reset(desktop->main_view);
            desktop_power_menu_close(desktop);
        } else {
            FURI_LOG_W(TAG, "Active app does not support clean exit");
            desktop->power_menu_exit_error = true;
            view_port_update(desktop->power_menu_viewport);
            /* Keep the menu visible; don't claim that an app was closed. */
        }
    } else if(event == DesktopGlobalPowerMenuReboot) {
        desktop_power_menu_close(desktop);
        Power* power = furi_record_open(RECORD_POWER);
        power_reboot(power, PowerBootModeNormal);
        furi_record_close(RECORD_POWER);
    } else if(event == DesktopGlobalPowerMenuOff) {
        desktop_power_menu_close(desktop);
        Power* power = furi_record_open(RECORD_POWER);
        power_off(power);
        furi_record_close(RECORD_POWER);
    } else if(event == DesktopGlobalBeforeAppStarted) {
        if(animation_manager_is_animation_loaded(desktop->animation_manager)) {
            animation_manager_unload_and_stall_animation(desktop->animation_manager);
        }

        desktop_auto_lock_inhibit(desktop);
        desktop->app_running = true;

        furi_semaphore_release(desktop->animation_semaphore);

    } else if(event == DesktopGlobalAfterAppFinished) {
        animation_manager_load_and_continue_animation(desktop->animation_manager);
        desktop_auto_lock_arm(desktop);
        desktop->app_running = false;
        desktop_main_reset(desktop->main_view);

    } else if(event == DesktopGlobalAutoLock) {
        if(!desktop->app_running && !desktop->locked) {
            desktop_lock(desktop);
        }

    } else if(event == DesktopGlobalSaveSettings) {
        desktop_settings_save(&desktop->settings);
        desktop_apply_settings(desktop);

    } else if(event == DesktopGlobalReloadSettings) {
        desktop_settings_load(&desktop->settings);
        desktop_apply_settings(desktop);

    } else {
        return scene_manager_handle_custom_event(desktop->scene_manager, event);
    }

    return true;
}

static bool desktop_back_event_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = (Desktop*)context;
    return scene_manager_handle_back_event(desktop->scene_manager);
}

static void desktop_tick_event_callback(void* context) {
    furi_assert(context);
    Desktop* app = context;
    scene_manager_handle_tick_event(app->scene_manager);
    desktop_main_refresh(app->main_view);
}

static void desktop_input_event_callback(const void* value, void* context) {
    furi_assert(value);
    furi_assert(context);
    const InputEvent* event = value;
    Desktop* desktop = context;
    if(event->type == InputTypePress) {
        desktop_start_auto_lock_timer(desktop);
    }
}

static void desktop_auto_lock_timer_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;
    view_dispatcher_send_custom_event(desktop->view_dispatcher, DesktopGlobalAutoLock);
}

static void desktop_start_auto_lock_timer(Desktop* desktop) {
    furi_timer_start(
        desktop->auto_lock_timer, furi_ms_to_ticks(desktop->settings.auto_lock_delay_ms));
}

static void desktop_stop_auto_lock_timer(Desktop* desktop) {
    furi_timer_stop(desktop->auto_lock_timer);
}

static void desktop_auto_lock_arm(Desktop* desktop) {
    if(desktop->settings.auto_lock_delay_ms) {
        desktop->input_events_subscription = furi_pubsub_subscribe(
            desktop->input_events_pubsub, desktop_input_event_callback, desktop);
        desktop_start_auto_lock_timer(desktop);
    }
}

static void desktop_auto_lock_inhibit(Desktop* desktop) {
    desktop_stop_auto_lock_timer(desktop);
    if(desktop->input_events_subscription) {
        furi_pubsub_unsubscribe(desktop->input_events_pubsub, desktop->input_events_subscription);
        desktop->input_events_subscription = NULL;
    }
}

static void desktop_clock_timer_callback(void* context) {
    furi_assert(context);
    Desktop* desktop = context;

    const bool clock_enabled = gui_active_view_port_count(desktop->gui, GuiLayerStatusBarLeft) < 6;

    if(clock_enabled) {
        desktop_clock_update(desktop);
    }

    view_port_enabled_set(desktop->clock_viewport, clock_enabled);
}

static void desktop_apply_settings(Desktop* desktop) {
    desktop->in_transition = true;

    view_port_enabled_set(desktop->clock_viewport, false);
    furi_timer_stop(desktop->update_clock_timer);

    view_port_enabled_set(desktop->dummy_mode_icon_viewport, desktop->settings.dummy_mode);
    desktop_main_set_dummy_mode_state(desktop->main_view, desktop->settings.dummy_mode);
    animation_manager_set_dummy_mode_state(
        desktop->animation_manager, desktop->settings.dummy_mode);

    /* Home WEAVE : aucune animation ni verrouillage automatique. */

    desktop->in_transition = false;
}

static void desktop_init_settings(Desktop* desktop) {
    furi_pubsub_subscribe(storage_get_pubsub(desktop->storage), desktop_storage_callback, desktop);

    if(storage_sd_status(desktop->storage) != FSE_OK) {
        FURI_LOG_D(TAG, "SD Card not ready, skipping settings");
        return;
    }

    desktop_settings_load(&desktop->settings);
    desktop_apply_settings(desktop);
}

static Desktop* desktop_alloc(void) {
    Desktop* desktop = calloc(1, sizeof(Desktop));

    desktop->animation_semaphore = furi_semaphore_alloc(1, 0);
    desktop->animation_manager = animation_manager_alloc();
    desktop->gui = furi_record_open(RECORD_GUI);
    desktop->scene_thread = furi_thread_alloc();
    desktop->view_dispatcher = view_dispatcher_alloc();
    desktop->scene_manager = scene_manager_alloc(&desktop_scene_handlers, desktop);

    view_dispatcher_attach_to_gui(
        desktop->view_dispatcher, desktop->gui, ViewDispatcherTypeDesktop);
    view_dispatcher_set_tick_event_callback(
        desktop->view_dispatcher, desktop_tick_event_callback, 200);

    view_dispatcher_set_event_callback_context(desktop->view_dispatcher, desktop);
    view_dispatcher_set_custom_event_callback(
        desktop->view_dispatcher, desktop_custom_event_callback);
    view_dispatcher_set_navigation_event_callback(
        desktop->view_dispatcher, desktop_back_event_callback);

    desktop->lock_menu = desktop_lock_menu_alloc();
    desktop->debug_view = desktop_debug_alloc();
    desktop->popup = popup_alloc();
    desktop->locked_view = desktop_view_locked_alloc();
    desktop->pin_input_view = desktop_view_pin_input_alloc();
    desktop->pin_timeout_view = desktop_view_pin_timeout_alloc();
    desktop->slideshow_view = desktop_view_slideshow_alloc();

    desktop->main_view_stack = view_stack_alloc();
    desktop->main_view = desktop_main_alloc();
    View* dolphin_view = animation_manager_get_animation_view(desktop->animation_manager);
    view_stack_add_view(desktop->main_view_stack, desktop_main_get_view(desktop->main_view));

    /* Vue verrouillée partagée ; affiche Unlocked sur principale */

    desktop->locked_view_stack = view_stack_alloc();
    view_stack_add_view(desktop->locked_view_stack, dolphin_view);
    view_stack_add_view(
        desktop->locked_view_stack, desktop_view_locked_get_view(desktop->locked_view));

    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdMain,
        view_stack_get_view(desktop->main_view_stack));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdLocked,
        view_stack_get_view(desktop->locked_view_stack));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdLockMenu,
        desktop_lock_menu_get_view(desktop->lock_menu));
    view_dispatcher_add_view(
        desktop->view_dispatcher, DesktopViewIdDebug, desktop_debug_get_view(desktop->debug_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher, DesktopViewIdPopup, popup_get_view(desktop->popup));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdPinTimeout,
        desktop_view_pin_timeout_get_view(desktop->pin_timeout_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdPinInput,
        desktop_view_pin_input_get_view(desktop->pin_input_view));
    view_dispatcher_add_view(
        desktop->view_dispatcher,
        DesktopViewIdSlideshow,
        desktop_view_slideshow_get_view(desktop->slideshow_view));

    // Lock icon
    desktop->lock_icon_viewport = view_port_alloc();
    view_port_set_width(desktop->lock_icon_viewport, icon_get_width(&I_Lock_7x8));
    view_port_draw_callback_set(
        desktop->lock_icon_viewport, desktop_lock_icon_draw_callback, desktop);
    view_port_enabled_set(desktop->lock_icon_viewport, false);
    gui_add_view_port(desktop->gui, desktop->lock_icon_viewport, GuiLayerStatusBarLeft);

    // Dummy mode icon
    desktop->dummy_mode_icon_viewport = view_port_alloc();
    view_port_set_width(desktop->dummy_mode_icon_viewport, icon_get_width(&I_GameMode_11x8));
    view_port_draw_callback_set(
        desktop->dummy_mode_icon_viewport, desktop_dummy_mode_icon_draw_callback, desktop);
    view_port_enabled_set(desktop->dummy_mode_icon_viewport, false);
    gui_add_view_port(desktop->gui, desktop->dummy_mode_icon_viewport, GuiLayerStatusBarLeft);

    // Clock
    desktop->clock_viewport = view_port_alloc();
    view_port_set_width(desktop->clock_viewport, 25);
    view_port_draw_callback_set(desktop->clock_viewport, desktop_clock_draw_callback, desktop);
    view_port_enabled_set(desktop->clock_viewport, false);
    gui_add_view_port(desktop->gui, desktop->clock_viewport, GuiLayerStatusBarRight);

    // Stealth mode icon
    desktop->stealth_mode_icon_viewport = view_port_alloc();
    view_port_set_width(desktop->stealth_mode_icon_viewport, icon_get_width(&I_Muted_8x8));
    view_port_draw_callback_set(
        desktop->stealth_mode_icon_viewport, desktop_stealth_mode_icon_draw_callback, desktop);
    if(furi_hal_rtc_is_flag_set(FuriHalRtcFlagStealthMode)) {
        view_port_enabled_set(desktop->stealth_mode_icon_viewport, true);
    } else {
        view_port_enabled_set(desktop->stealth_mode_icon_viewport, false);
    }
    gui_add_view_port(desktop->gui, desktop->stealth_mode_icon_viewport, GuiLayerStatusBarLeft);

    /* Décharger animations avant démarrage application */
    desktop->loader = furi_record_open(RECORD_LOADER);
    furi_pubsub_subscribe(loader_get_pubsub(desktop->loader), desktop_loader_callback, desktop);

    desktop->storage = furi_record_open(RECORD_STORAGE);
    desktop->notification = furi_record_open(RECORD_NOTIFICATION);
    desktop->input_events_pubsub = furi_record_open(RECORD_INPUT_EVENTS);
    desktop->power_menu_viewport = view_port_alloc();
    view_port_draw_callback_set(
        desktop->power_menu_viewport, desktop_power_menu_draw, desktop);
    view_port_input_callback_set(
        desktop->power_menu_viewport, desktop_power_menu_input, desktop);
    view_port_enabled_set(desktop->power_menu_viewport, false);
    gui_add_view_port(desktop->gui, desktop->power_menu_viewport, GuiLayerFullscreen);
    desktop->power_menu_subscription = furi_pubsub_subscribe(
        desktop->input_events_pubsub, desktop_power_menu_input_subscriber, desktop);

    desktop->auto_lock_timer =
        furi_timer_alloc(desktop_auto_lock_timer_callback, FuriTimerTypeOnce, desktop);

    desktop->status_pubsub = furi_pubsub_alloc();

    desktop->update_clock_timer =
        furi_timer_alloc(desktop_clock_timer_callback, FuriTimerTypePeriodic, desktop);

    desktop->app_running = loader_is_locked(desktop->loader);

    furi_record_create(RECORD_DESKTOP, desktop);

    return desktop;
}

/*
 * Private API
 */

void desktop_lock(Desktop* desktop) {
    furi_assert(!desktop->locked);

    furi_hal_rtc_set_flag(FuriHalRtcFlagLock);

    if(desktop_pin_code_is_set()) {
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        cli_vcp_disable(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
    }

    desktop_auto_lock_inhibit(desktop);
    scene_manager_set_scene_state(
        desktop->scene_manager, DesktopSceneLocked, DesktopSceneLockedStateFirstEnter);
    scene_manager_next_scene(desktop->scene_manager, DesktopSceneLocked);

    DesktopStatus status = {.locked = true};
    furi_pubsub_publish(desktop->status_pubsub, &status);

    desktop->locked = true;
}

void desktop_unlock(Desktop* desktop) {
    furi_assert(desktop->locked);

    view_port_enabled_set(desktop->lock_icon_viewport, false);
    Gui* gui = furi_record_open(RECORD_GUI);
    gui_set_lockdown(gui, false);
    furi_record_close(RECORD_GUI);
    desktop_view_locked_unlock(desktop->locked_view);
    scene_manager_search_and_switch_to_previous_scene(desktop->scene_manager, DesktopSceneMain);
    desktop_auto_lock_arm(desktop);
    furi_hal_rtc_reset_flag(FuriHalRtcFlagLock);
    furi_hal_rtc_set_pin_fails(0);

    if(desktop_pin_code_is_set()) {
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        cli_vcp_enable(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
    }

    DesktopStatus status = {.locked = false};
    furi_pubsub_publish(desktop->status_pubsub, &status);

    desktop->locked = false;
}

void desktop_set_dummy_mode_state(Desktop* desktop, bool enabled) {
    desktop->in_transition = true;

    view_port_enabled_set(desktop->dummy_mode_icon_viewport, enabled);
    desktop_main_set_dummy_mode_state(desktop->main_view, enabled);
    animation_manager_set_dummy_mode_state(desktop->animation_manager, enabled);
    desktop->settings.dummy_mode = enabled;

    desktop->in_transition = false;

    desktop_settings_save(&desktop->settings);
}

void desktop_set_stealth_mode_state(Desktop* desktop, bool enabled) {
    desktop->in_transition = true;

    if(enabled) {
        furi_hal_rtc_set_flag(FuriHalRtcFlagStealthMode);
    } else {
        furi_hal_rtc_reset_flag(FuriHalRtcFlagStealthMode);
    }

    view_port_enabled_set(desktop->stealth_mode_icon_viewport, enabled);

    desktop->in_transition = false;
}

/*
 *  Public API
 */

bool desktop_api_is_locked(Desktop* instance) {
    furi_assert(instance);
    return furi_hal_rtc_is_flag_set(FuriHalRtcFlagLock);
}

void desktop_api_unlock(Desktop* instance) {
    furi_assert(instance);
    view_dispatcher_send_custom_event(instance->view_dispatcher, DesktopGlobalApiUnlock);
}

FuriPubSub* desktop_api_get_status_pubsub(Desktop* instance) {
    furi_assert(instance);
    return instance->status_pubsub;
}

void desktop_api_reload_settings(Desktop* instance) {
    furi_assert(instance);
    view_dispatcher_send_custom_event(instance->view_dispatcher, DesktopGlobalReloadSettings);
}

void desktop_api_get_settings(Desktop* instance, DesktopSettings* settings) {
    furi_assert(instance);
    furi_assert(settings);

    *settings = instance->settings;
}

void desktop_api_set_settings(Desktop* instance, const DesktopSettings* settings) {
    furi_assert(instance);
    furi_assert(settings);

    instance->settings = *settings;
    view_dispatcher_send_custom_event(instance->view_dispatcher, DesktopGlobalSaveSettings);
}

/*
 * Application thread
 */

int32_t desktop_srv(void* p) {
    UNUSED(p);

    if(furi_hal_rtc_get_boot_mode() != FuriHalRtcBootModeNormal) {
        FURI_LOG_W(TAG, "Skipping start in special boot mode");

        furi_thread_suspend(furi_thread_get_current_id());
        return 0;
    }

    Desktop* desktop = desktop_alloc();

    desktop_init_settings(desktop);

    scene_manager_next_scene(desktop->scene_manager, DesktopSceneMain);

    if(desktop_pin_code_is_set()) {
        desktop_lock(desktop);
    } else {
        CliVcp* cli_vcp = furi_record_open(RECORD_CLI_VCP);
        cli_vcp_enable(cli_vcp);
        furi_record_close(RECORD_CLI_VCP);
    }


    if(!furi_hal_version_do_i_belong_here()) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneHwMismatch);
    }

    if(furi_hal_rtc_get_fault_data()) {
        scene_manager_next_scene(desktop->scene_manager, DesktopSceneFault);
    }

    uint8_t keys_total, keys_valid;
    if(!furi_hal_crypto_enclave_verify(&keys_total, &keys_valid)) {
        FURI_LOG_E(
            TAG,
            "Secure Enclave verification failed: total %hhu, valid %hhu",
            keys_total,
            keys_valid);

        scene_manager_next_scene(desktop->scene_manager, DesktopSceneSecureEnclave);
    }

    /* Autostart déjà actif */
    if(desktop->app_running && animation_manager_is_animation_loaded(desktop->animation_manager)) {
        animation_manager_unload_and_stall_animation(desktop->animation_manager);
    }

    view_dispatcher_run(desktop->view_dispatcher);

    /* Thread service : retour interdit, crash automatique */
    return 0;
}
