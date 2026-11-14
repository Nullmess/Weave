#include <furi.h>
#include <furi_hal.h>

#include <gui/gui.h>
#include <gui/view_port.h>

#include <notification/notification.h>
#include <notification/notification_messages.h>

#include <assets_icons.h>
#include "../../services/weave/weave_ringtone.h"

#define TAG "ClockSettingsAlarm"

typedef struct {
    DateTime now;
    IconAnimation* icon;
} ClockSettingsAlramModel;

const NotificationSequence sequence_alarm = {
    /* Respect Sound & Vibration settings for the daily alarm. */
    &message_force_display_brightness_setting_1f,
    &message_vibro_on,

    &message_display_backlight_on,
    &message_note_c7,
    &message_delay_250,

    &message_display_backlight_off,
    &message_note_c4,
    &message_delay_250,

    &message_display_backlight_on,
    &message_note_c7,
    &message_delay_250,

    &message_display_backlight_off,
    &message_note_c4,
    &message_delay_250,

    &message_sound_off,
    &message_vibro_off,
    NULL,
};

static void clock_settings_alarm_draw_callback(Canvas* canvas, void* ctx) {
    ClockSettingsAlramModel* model = ctx;
    char buffer[64] = {};

    canvas_draw_icon_animation(canvas, 5, 6, model->icon);

    canvas_set_font(canvas, FontBigNumbers);
    snprintf(buffer, sizeof(buffer), "%02u:%02u", model->now.hour, model->now.minute);
    canvas_draw_str(canvas, 58, 32, buffer);

    canvas_set_font(canvas, FontPrimary);
    snprintf(
        buffer,
        sizeof(buffer),
        "%02u.%02u.%04u",
        model->now.day,
        model->now.month,
        model->now.year);
    canvas_draw_str(canvas, 60, 44, buffer);

    // Preserve the original animated Flipper alarm, with explicit actions.
    canvas_set_color(canvas, ColorBlack);
    canvas_draw_rframe(canvas, 0, 51, 60, 13, 2);
    canvas_draw_rframe(canvas, 66, 51, 62, 13, 2);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 30, 57, AlignCenter, AlignCenter, "OK +2m");
    canvas_draw_str_aligned(canvas, 97, 57, AlignCenter, AlignCenter, "BACK Stop");
}

static void clock_settings_alarm_input_callback(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    FuriMessageQueue* event_queue = ctx;
    furi_message_queue_put(event_queue, input_event, 0);
}

static void clock_settings_alarm_start(void* context, uint32_t arg);
static FuriTimer* clock_settings_snooze_timer = NULL;

static void clock_settings_snooze_timer_callback(void* context) {
    UNUSED(context);
    clock_settings_alarm_start(NULL, 0);
}

void clock_settings_alarm_animation_callback(IconAnimation* instance, void* context) {
    UNUSED(instance);
    ViewPort* view_port = context;
    view_port_update(view_port);
}

int32_t clock_settings_alarm(void* p) {
    UNUSED(p);

    // View Model
    ClockSettingsAlramModel model;

    furi_hal_rtc_get_datetime(&model.now);
    model.icon = icon_animation_alloc(&A_Alarm_47x39);

    FuriMessageQueue* event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    ViewPort* view_port = view_port_alloc();
    view_port_draw_callback_set(view_port, clock_settings_alarm_draw_callback, &model);
    view_port_input_callback_set(view_port, clock_settings_alarm_input_callback, event_queue);

    Gui* gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(gui, view_port, GuiLayerFullscreen);

    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
    /* Alarm alert profile independently controls sound, volume and vibration.
     * The RTC schedule and snooze remain owned by Clock & Alarm. */
    weave_ringtone_start_profile(WeaveAlertAlarm);

    icon_animation_set_update_callback(
        model.icon, clock_settings_alarm_animation_callback, view_port);
    icon_animation_start(model.icon);

    // Process events
    InputEvent event;
    bool running = true;
    while(running) {
        if(furi_message_queue_get(event_queue, &event, 2000) == FuriStatusOk) {
            if(event.type == InputTypeShort) {
                if(event.key == InputKeyOk) {
                    // A software timer does not replace the daily hardware RTC alarm.
                    if(clock_settings_snooze_timer) {
                        furi_timer_start(clock_settings_snooze_timer, furi_ms_to_ticks(120000));
                    }
                    running = false;
                } else if(event.key == InputKeyBack) {
                    // The original RTC alarm remains enabled for tomorrow.
                    running = false;
                }
            }
        } else {
            /* The chiptune worker repeats until OK/BACK or snooze. */
            furi_hal_rtc_get_datetime(&model.now);
            view_port_update(view_port);
        }
    }

    weave_ringtone_stop();
    icon_animation_stop(model.icon);

    notification_message_block(notification, &sequence_empty);
    furi_record_close(RECORD_NOTIFICATION);

    view_port_enabled_set(view_port, false);
    gui_remove_view_port(gui, view_port);
    view_port_free(view_port);
    furi_message_queue_free(event_queue);
    furi_record_close(RECORD_GUI);

    icon_animation_free(model.icon);

    return 0;
}

FuriThread* clock_settings_alarm_thread = NULL;

static void clock_settings_alarm_thread_state_callback(
    FuriThread* thread,
    FuriThreadState state,
    void* context) {
    furi_assert(clock_settings_alarm_thread == thread);
    UNUSED(context);

    if(state == FuriThreadStateStopped) {
        furi_thread_free(thread);
        clock_settings_alarm_thread = NULL;
    }
}

static void clock_settings_alarm_start(void* context, uint32_t arg) {
    UNUSED(context);
    UNUSED(arg);

    FURI_LOG_I(TAG, "spawning alarm thread");

    if(clock_settings_alarm_thread) return;

    clock_settings_alarm_thread =
        furi_thread_alloc_ex("ClockAlarm", 2048, clock_settings_alarm, NULL);
    furi_thread_set_state_callback(
        clock_settings_alarm_thread, clock_settings_alarm_thread_state_callback);
    furi_thread_start(clock_settings_alarm_thread);
}

static void clock_settings_alarm_isr(void* context) {
    UNUSED(context);
    furi_timer_pending_callback(clock_settings_alarm_start, NULL, 0);
}

void clock_settings_start(void) {
#ifndef FURI_RAM_EXEC
    clock_settings_snooze_timer = furi_timer_alloc(
        clock_settings_snooze_timer_callback, FuriTimerTypeOnce, NULL);
    furi_hal_rtc_set_alarm_callback(clock_settings_alarm_isr, NULL);
#endif
}
