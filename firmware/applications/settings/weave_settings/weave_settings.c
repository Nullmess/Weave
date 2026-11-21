#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view_dispatcher.h>
#include <gui/view.h>
#include <gui/canvas.h>
#include <gui/weave_ui.h>
#include <input/input.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/popup.h>
#include <gui/modules/text_input.h>
#include <gui/modules/text_box.h>
/* Reuse the native Clock & Alarm screen directly in Weave Settings. */
#include "../clock_settings/views/clock_settings_module.h"
#include <storage/storage.h>
#include "../../services/region/weave_radio_options.h"
#include <power/power_service/power.h>
#include <notification/notification_app.h>
#include <notification/notification_messages.h>
#include "../../services/weave/weave_ringtone.h"
#include <stdio.h>
#include <string.h>

/* API interne au firmware, non exposée aux FAP. */
extern void furi_hal_version_set_custom_name(const char* name);

#include <weave/weave_gateway_notice.h>
#include <weave/weave_factory_defaults.h>

#define WEAVE_NAME_FILE EXT_PATH(".weave/name")
#define WEAVE_NAME_DIR EXT_PATH(".weave")
#define WEAVE_RELEASE "1.0.1"

typedef enum {
    WeaveSettingsViewMenu,
    WeaveSettingsViewInput,
    WeaveSettingsViewAbout,
    WeaveSettingsViewDialog,
    WeaveSettingsViewStorage,
    WeaveSettingsViewBrightness,
    WeaveSettingsViewClock,
    WeaveSettingsViewAlerts,
    WeaveSettingsViewFirmwareUpdate,
    WeaveSettingsViewHostnameSaved,
} WeaveSettingsView;

typedef enum {
    WeaveSettingsRoot,
    WeaveSettingsStorage,
    WeaveSettingsSd,
    WeaveSettingsSubGhz,
} WeaveSettingsPage;

typedef enum {
    WeaveSettingsDialogInfo,
    WeaveSettingsDialogSdFormat,
    WeaveSettingsDialogResetFirst,
    WeaveSettingsDialogResetLast,
    WeaveSettingsDialogFindDevice,
    WeaveSettingsDialogSubGhzBypass,
    WeaveSettingsDialogSubGhzExtend,
} WeaveSettingsDialog;

typedef enum {
    WeaveFirmwareStatusChecking,
    WeaveFirmwareStatusAvailable,
    WeaveFirmwareStatusCurrent,
    WeaveFirmwareStatusUnavailable,
} WeaveFirmwareStatus;

typedef struct {
    Gui* gui;
    Storage* storage;
    NotificationApp* notification;
    ViewDispatcher* dispatcher;
    Submenu* menu;
    DialogEx* dialog;
    Popup* hostname_saved;
    TextInput* input;
    TextBox* about;
    View* storage_view;
    View* brightness_view;
    View* alerts_view;
    View* firmware_update_view;
    WeaveFirmwareStatus firmware_status;
    uint16_t firmware_elapsed_ticks;
    uint8_t firmware_spinner;
    char firmware_check_token[48];
    char firmware_latest[20];
    uint8_t sound_selected;
    uint8_t alerts_selected;
    bool alerts_editing;
    uint8_t profile_type;
    WeaveAlertSettings profile;
    WeaveAlertSettings alert_profiles[4];
    uint8_t ring_count;
    char ring_names[96][40];
    ClockSettingsModule* clock_view;
    uint8_t brightness[2];
    uint8_t brightness_selected;
    bool brightness_dirty;
    WeaveSettingsPage page;
    WeaveSettingsView active_view;
    WeaveSettingsDialog pending;
    char hostname[9];
    char about_text[400];
    char message[80];
    uint64_t sd_total;
    uint64_t sd_free;
    bool sd_available;
} WeaveSettings;

static void weave_settings_show_menu(WeaveSettings* app, WeaveSettingsPage page);
static void weave_settings_dialog_result(DialogExResult result, void* context);
static void weave_settings_show_info(WeaveSettings* app, const char* title, const char* message);
static void weave_settings_show_alerts(WeaveSettings* app);
static void weave_settings_check_firmware(WeaveSettings* app);
static bool weave_settings_find_device_request(WeaveSettings* app);

static void weave_settings_capacity(char* result, size_t len, uint64_t bytes) {
    if(bytes >= 1024ULL * 1024 * 1024) {
        snprintf(result, len, "%lu GiB", (unsigned long)(bytes / (1024ULL * 1024 * 1024)));
    } else if(bytes >= 1024ULL * 1024) {
        snprintf(result, len, "%lu MiB", (unsigned long)(bytes / (1024ULL * 1024)));
    } else {
        snprintf(result, len, "%lu KiB", (unsigned long)(bytes / 1024));
    }
}

static void weave_settings_draw_usage_meter(
    Canvas* canvas, uint8_t x, uint8_t y, uint64_t used, uint64_t total) {
    const uint8_t inner_width = 22;
    canvas_draw_frame(canvas, x, y, inner_width + 2, 6);
    if(!total || !used) return;
    /* Volume sizes fit well within a 64-bit integer; divide before scaling
     * to avoid overflow and retain resolution on almost empty volumes. */
    uint32_t pixels = (uint32_t)((used / total) * inner_width);
    pixels += (uint32_t)(((used % total) * inner_width) / total);
    if(!pixels) pixels = 1; /* Show very small nonzero use. */
    if(pixels > inner_width) pixels = inner_width;
    canvas_draw_box(canvas, x + 1, y + 1, pixels, 4);
}

/* Storage is a single SD volume. The /int alias is intentionally not a UI choice. */
static void weave_settings_draw_storage(Canvas* canvas, void* context) {
    WeaveSettings* app = *(WeaveSettings**)context;
    canvas_clear(canvas);
    weave_ui_header(canvas, "Storage");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_rframe(canvas, 1, 19, 126, 30, 2);
    canvas_draw_str(canvas, 6, 30, "SD Card");
    if(!app->sd_available) {
        canvas_draw_str(canvas, 6, 43, "SD Card unavailable");
        return;
    }
    const uint64_t used = app->sd_total >= app->sd_free ?
        app->sd_total - app->sd_free : 0;
    char used_str[16];
    char total_str[16];
    char line[48];
    weave_settings_capacity(used_str, sizeof(used_str), used);
    weave_settings_capacity(total_str, sizeof(total_str), app->sd_total);
    snprintf(line, sizeof(line), "%s / %s", used_str, total_str);
    canvas_draw_str(canvas, 6, 43, line);
    weave_settings_draw_usage_meter(canvas, 100, 25, used, app->sd_total);
    canvas_draw_str_aligned(canvas, 64, 61, AlignCenter, AlignBottom, "OK: SD Card options");
}

static void weave_settings_sd_categories(WeaveSettings* app) {
    static const char* const folders[] = {
        "USB", "GPIO", "Tools", "Bluetooth", "NFC", "RFID",
        "Sub-GHz", "Infrared", "iButton", "Games", "Other",
    };
    if(storage_sd_status(app->storage) != FSE_OK) return;
    storage_simply_mkdir(app->storage, EXT_PATH("apps"));
    for(size_t i = 0; i < COUNT_OF(folders); i++) {
        FuriString* dir = furi_string_alloc_printf(EXT_PATH("apps/%s"), folders[i]);
        storage_simply_mkdir(app->storage, furi_string_get_cstr(dir));
        furi_string_free(dir);
    }
}

static void weave_settings_show_storage(WeaveSettings* app) {
    uint64_t total = 0;
    uint64_t free_bytes = 0;
    app->sd_available = storage_sd_status(app->storage) == FSE_OK &&
        storage_common_fs_info(app->storage, STORAGE_EXT_PATH_PREFIX,
            &total, &free_bytes) == FSE_OK;
    app->sd_total = total;
    app->sd_free = free_bytes;
    app->page = WeaveSettingsStorage;
    app->active_view = WeaveSettingsViewStorage;
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewStorage);
}

static bool weave_settings_storage_input(InputEvent* event, void* context) {
    WeaveSettings* app = context;
    if(event->type != InputTypeShort) return true;
    if(event->key == InputKeyOk) {
        weave_settings_show_menu(app, WeaveSettingsSd);
    } else if(event->key == InputKeyBack) {
        weave_settings_show_menu(app, WeaveSettingsRoot);
    }
    return true;
}

/* The official notification service owns LED brightness and the backlight.
 * Do not write directly to the light PWM: the service would overwrite it. */
static const NotificationMessage weave_led_brightness_apply = {
    .type = NotificationMessageTypeLedBrightnessSettingApply,
};
static const NotificationSequence weave_led_brightness_sequence = {
    &weave_led_brightness_apply,
    NULL,
};

static uint8_t weave_settings_brightness_to_percent(float value) {
    if(value <= 0.0f) return 0;
    if(value >= 1.0f) return 100;
    return (uint8_t)(value * 100.0f + 0.5f);
}

static void weave_settings_brightness_save(WeaveSettings* app) {
    if(app->brightness_dirty) {
        notification_message_save_settings(app->notification);
        app->brightness_dirty = false;
    }
}

static void weave_settings_brightness_apply(WeaveSettings* app, uint8_t selected) {
    float level = (float)app->brightness[selected] / 100.0f;
    if(selected == 0) {
        app->notification->settings.display_brightness = level;
        notification_message(app->notification, &sequence_display_backlight_on);
    } else {
        app->notification->settings.led_brightness = level;
        notification_message(app->notification, &weave_led_brightness_sequence);
        notification_internal_message(app->notification, &weave_led_brightness_sequence);
        notification_message(app->notification, &sequence_blink_white_100);
    }
    app->brightness_dirty = true;
}

/* Minimal dual-slider layout on the 128x64 LCD. No large selection box:
 * a slim marker and an inverted percentage identify the active control. */
static void weave_settings_draw_brightness(Canvas* canvas, void* context) {
    WeaveSettings* app = *(WeaveSettings**)context;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 11, AlignCenter, AlignBottom, "Brightness");
    canvas_draw_line(canvas, 8, 15, 119, 15);

    for(uint8_t i = 0; i < 2; i++) {
        const int32_t top = 19 + (int32_t)i * 21;
        const bool selected = app->brightness_selected == i;

        if(selected) {
            /* Compact navigation marker, without boxing in the whole row. */
            canvas_draw_line(canvas, 3, top + 2, 6, top + 5);
            canvas_draw_line(canvas, 6, top + 5, 3, top + 8);
        }

        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 14, top + 7, i == 0 ? "Screen" : "LED");
        char percent[5];
        snprintf(percent, sizeof(percent), "%u%%", app->brightness[i]);
        if(selected) {
            canvas_draw_rbox(canvas, 88, top - 1, 33, 11, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        canvas_draw_str_aligned(canvas, 117, top + 7, AlignRight, AlignBottom, percent);
        canvas_set_color(canvas, ColorBlack);

        /* Five-pixel track, with a restrained three-pixel progress fill. */
        canvas_draw_rframe(canvas, 14, top + 11, 107, 5, 2);
        const uint8_t filled = (uint8_t)(((uint16_t)app->brightness[i] * 105U) / 100U);
        if(filled) canvas_draw_box(canvas, 15, top + 12, filled, 3);
    }
}

static void weave_settings_show_brightness(WeaveSettings* app) {
    app->brightness[0] = weave_settings_brightness_to_percent(
        app->notification->settings.display_brightness);
    app->brightness[1] = weave_settings_brightness_to_percent(
        app->notification->settings.led_brightness);
    app->brightness_selected = 0;
    app->brightness_dirty = false;
    app->active_view = WeaveSettingsViewBrightness;
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewBrightness);
}

static bool weave_settings_brightness_input(InputEvent* event, void* context) {
    WeaveSettings* app = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return true;
    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        if(event->type == InputTypeShort) {
            app->brightness_selected ^= 1U;
            view_commit_model(app->brightness_view, true);
        }
    } else if(event->key == InputKeyLeft || event->key == InputKeyRight) {
        uint8_t i = app->brightness_selected;
        uint8_t old = app->brightness[i];
        /* Snap to 5% for predictable volume-slider behavior. */
        uint8_t adjusted = event->key == InputKeyRight ?
            (uint8_t)(((old / 5U) + 1U) * 5U) :
            (uint8_t)(old ? ((old - 1U) / 5U) * 5U : 0U);
        if(adjusted > 100U) adjusted = 100U;
        if(adjusted != old) {
            app->brightness[i] = adjusted;
            weave_settings_brightness_apply(app, i);
            view_commit_model(app->brightness_view, true);
        }
    } else if(event->key == InputKeyBack && event->type == InputTypeShort) {
        weave_settings_brightness_save(app);
        weave_settings_show_menu(app, WeaveSettingsRoot);
    }
    return true;
}

/* The four quick tabs share one compact overview. Each profile is stored
 * independently; changing the Flipper's own alert does not alter phone audio. */
#define WEAVE_RINGTONES_DIR EXT_PATH(".weave/ringtones")
#define WEAVE_RINGTONES_STOCK EXT_PATH(".weave/stock-ringtones")
#define WEAVE_RINGTONES_SEEDED EXT_PATH(".weave/ringtones/.weave-seeded-v1")

/* Four 18px monochrome symbols, sized for the real 128x64 Flipper LCD. */
static void weave_settings_draw_profile_icon(Canvas* canvas, uint8_t type, int32_t x, int32_t y) {
    if(type == WeaveAlertFinder) {
        canvas_draw_box(canvas,x+1,y+6,4,7);
        canvas_draw_line(canvas,x+5,y+6,x+10,y+2);
        canvas_draw_line(canvas,x+5,y+12,x+10,y+16);
        canvas_draw_line(canvas,x+10,y+2,x+10,y+16);
        canvas_draw_line(canvas,x+12,y+6,x+14,y+8);
        canvas_draw_line(canvas,x+14,y+8,x+12,y+11);
        canvas_draw_line(canvas,x+15,y+4,x+18,y+8);
        canvas_draw_line(canvas,x+18,y+8,x+15,y+13);
    } else if(type == WeaveAlertAlarm) {
        canvas_draw_circle(canvas,x+9,y+10,7);
        canvas_draw_line(canvas,x+9,y+10,x+9,y+5);
        canvas_draw_line(canvas,x+9,y+10,x+13,y+12);
        canvas_draw_line(canvas,x+4,y+2,x+7,y+1);
        canvas_draw_line(canvas,x+12,y+1,x+15,y+2);
        canvas_draw_line(canvas,x+7,y+18,x+5,y+19);
        canvas_draw_line(canvas,x+12,y+18,x+14,y+19);
    } else if(type == WeaveAlertSms) {
        canvas_draw_rframe(canvas,x+1,y+3,17,13,3);
        canvas_draw_line(canvas,x+4,y+8,x+15,y+8);
        canvas_draw_line(canvas,x+4,y+11,x+12,y+11);
        canvas_draw_line(canvas,x+11,y+16,x+16,y+19);
    } else {
        canvas_draw_rframe(canvas,x+5,y+2,8,15,3);
        canvas_draw_line(canvas,x+3,y+5,x+5,y+7);
        canvas_draw_line(canvas,x+3,y+13,x+5,y+15);
        canvas_draw_line(canvas,x+13,y+7,x+15,y+5);
        canvas_draw_line(canvas,x+13,y+15,x+15,y+13);
        canvas_draw_line(canvas,x+17,y+6,x+19,y+9);
        canvas_draw_line(canvas,x+19,y+9,x+17,y+12);
    }
}

static void weave_settings_write_tone(Storage* storage,const char* name,const char* notes) {
    FuriString* path=furi_string_alloc_printf(WEAVE_RINGTONES_DIR "/%s",name);
    if(!storage_common_exists(storage,furi_string_get_cstr(path))) {
        File* file=storage_file_alloc(storage);
        if(storage_file_open(file,furi_string_get_cstr(path),FSAM_WRITE,FSOM_CREATE_NEW)) {
            storage_file_write(file,notes,strlen(notes));
            storage_file_close(file);
        }
        storage_file_free(file);
    }
    furi_string_free(path);
}
static void weave_settings_prepare_ringtones(WeaveSettings* app) {
    if(storage_sd_status(app->storage)!=FSE_OK)return;
    storage_simply_mkdir(app->storage,EXT_PATH(".weave"));
    storage_simply_mkdir(app->storage,WEAVE_RINGTONES_DIR);
    /* The firmware update package supplies stock songs under stock-ringtones.
     * Copy once into the user's library; updates cannot erase personal songs. */
    if(!storage_common_exists(app->storage,WEAVE_RINGTONES_SEEDED)) {
        File* dir=storage_file_alloc(app->storage);
        if(storage_dir_open(dir,WEAVE_RINGTONES_STOCK)) {
            FileInfo info;
            char name[40];
            while(storage_dir_read(dir,&info,name,sizeof(name))) {
                if(file_info_is_dir(&info)||!weave_ringtone_valid_filename(name))continue;
                FuriString* src=furi_string_alloc_printf(WEAVE_RINGTONES_STOCK "/%s",name);
                FuriString* dst=furi_string_alloc_printf(WEAVE_RINGTONES_DIR "/%s",name);
                if(!storage_common_exists(app->storage,furi_string_get_cstr(dst)))
                    storage_common_copy(app->storage,furi_string_get_cstr(src),furi_string_get_cstr(dst));
                furi_string_free(src);
                furi_string_free(dst);
            }
            storage_dir_close(dir);
            File* mark=storage_file_alloc(app->storage);
            if(storage_file_open(mark,WEAVE_RINGTONES_SEEDED,FSAM_WRITE,FSOM_CREATE_ALWAYS)) {
                storage_file_write(mark,"1",1);
                storage_file_close(mark);
            }
            storage_file_free(mark);
        }
        storage_file_free(dir);
    }
    /* Standalone firmware / SD without resources: still provide three tones. */
    weave_settings_write_tone(app->storage,"Classic.tone",
        "880 220\n659 170\n1046 220\n659 170\n880 250\n");
    weave_settings_write_tone(app->storage,"Pulse.tone",
        "988 150\n784 150\n988 150\n784 150\n988 180\n");
    weave_settings_write_tone(app->storage,"Chime.tone",
        "523 230\n659 230\n784 300\n1046 360\n");
}

/* One 128x64 dashboard, matching the four-column reference. All controls
 * remain on this View: never navigate to secondary Sound or Ringtones pages.
 * A ringtone preview is strictly local to the Flipper (no companion RPC). */
static const uint8_t weave_settings_tabs[4] = {
    WeaveAlertFinder, WeaveAlertAlarm, WeaveAlertSms, WeaveAlertCall,
};

static void weave_settings_refresh_library(WeaveSettings* app) {
    app->ring_count = 0;
    if(storage_sd_status(app->storage) != FSE_OK) return;
    weave_settings_prepare_ringtones(app);
    File* dir = storage_file_alloc(app->storage);
    if(storage_dir_open(dir, WEAVE_RINGTONES_DIR)) {
        FileInfo info;
        char name[40];
        while(app->ring_count < COUNT_OF(app->ring_names) &&
              storage_dir_read(dir, &info, name, sizeof(name))) {
            if(file_info_is_dir(&info) || !weave_ringtone_valid_filename(name) ||
               strncmp(name, "Preview_remote.", 15) == 0) continue;
            strlcpy(app->ring_names[app->ring_count++], name, sizeof(app->ring_names[0]));
        }
        storage_dir_close(dir);
    }
    storage_file_free(dir);
    /* Stable alphabetical order, independently of FAT directory traversal. */
    for(uint8_t i = 1; i < app->ring_count; ++i) {
        char item[40];
        strlcpy(item, app->ring_names[i], sizeof(item));
        uint8_t j = i;
        while(j > 0 && strcmp(app->ring_names[j - 1], item) > 0) {
            strlcpy(app->ring_names[j], app->ring_names[j - 1], sizeof(app->ring_names[0]));
            --j;
        }
        strlcpy(app->ring_names[j], item, sizeof(app->ring_names[0]));
    }
}

/* 0 = None; 1 = built-in default; 2.. = user/stock SD ringtones. */
static uint8_t weave_settings_ring_index(const WeaveSettings* app) {
    if(strcmp(app->profile.ringtone, "None") == 0) return 0;
    if(!app->profile.ringtone[0]) return 1;
    for(uint8_t i = 0; i < app->ring_count; ++i) {
        if(!strcmp(app->profile.ringtone, app->ring_names[i])) return (uint8_t)(i + 2);
    }
    return 1;
}

static void weave_settings_draw_calls_icon(Canvas* canvas, int32_t x, int32_t y) {
    /* Angled, recognisable handset silhouette, rather than a smartphone icon. */
    canvas_draw_line(canvas,x+3,y+3,x+5,y+1);
    canvas_draw_line(canvas,x+5,y+1,x+8,y+1);
    canvas_draw_line(canvas,x+8,y+1,x+10,y+5);
    canvas_draw_line(canvas,x+10,y+5,x+7,y+7);
    canvas_draw_line(canvas,x+7,y+7,x+10,y+12);
    canvas_draw_line(canvas,x+10,y+12,x+12,y+10);
    canvas_draw_line(canvas,x+12,y+10,x+16,y+12);
    canvas_draw_line(canvas,x+16,y+12,x+16,y+15);
    canvas_draw_line(canvas,x+16,y+15,x+14,y+17);
    canvas_draw_line(canvas,x+14,y+17,x+10,y+16);
    canvas_draw_line(canvas,x+10,y+16,x+5,y+12);
    canvas_draw_line(canvas,x+5,y+12,x+2,y+7);
    canvas_draw_line(canvas,x+2,y+7,x+3,y+3);
    canvas_draw_line(canvas,x+17,y+4,x+19,y+7);
    canvas_draw_line(canvas,x+19,y+7,x+17,y+10);
}

/* Two-level, hardware-only UI: four icons, then an ordinary settings page.
 * General has no selectable ringtone; Alarm/SMS/Calls each use local files. */
static const char* weave_settings_profile_title(uint8_t index) {
    static const char* const titles[4] = {"General", "Alarm", "SMS", "Calls"};
    return titles[index < COUNT_OF(titles) ? index : 0];
}

static uint8_t weave_settings_alert_row_count(const WeaveSettings* app) {
    return app->profile_type == WeaveAlertFinder ? 2U : 3U;
}

static void weave_settings_alerts_overview(Canvas* canvas, const WeaveSettings* app) {
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 11, AlignCenter, AlignBottom, "Ringtones & Alerts");
    canvas_draw_line(canvas, 6, 15, 121, 15);
    for(uint8_t i = 0; i < 4; ++i) {
        const int32_t x = (int32_t)i * 32;
        if(app->alerts_selected == i) canvas_draw_rframe(canvas, x + 2, 20, 28, 28, 4);
        if(weave_settings_tabs[i] == WeaveAlertCall) {
            weave_settings_draw_calls_icon(canvas, x + 6, 23);
        } else {
            weave_settings_draw_profile_icon(canvas, weave_settings_tabs[i], x + 6, 23);
        }
    }
    /* Only the icons are selectable. The caption identifies the active icon. */
    canvas_draw_str(canvas, 4, 59, "<");
    canvas_draw_str_aligned(
        canvas, 64, 59, AlignCenter, AlignBottom,
        weave_settings_profile_title(app->alerts_selected));
    canvas_draw_str(canvas, 120, 59, ">");
}

static void weave_settings_alerts_profile(Canvas* canvas, const WeaveSettings* app) {
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(
        canvas, 64, 12, AlignCenter, AlignBottom,
        weave_settings_profile_title(app->alerts_selected));
    canvas_draw_line(canvas, 8, 15, 119, 15);
    canvas_set_font(canvas, FontSecondary);

    const uint8_t rows = weave_settings_alert_row_count(app);
    for(uint8_t i = 0; i < rows; ++i) {
        const int32_t baseline = rows == 2 ? 31 + (int32_t)i * 21 :
                                             27 + (int32_t)i * 15;
        if(app->sound_selected == i)
            canvas_draw_rframe(canvas, 2, baseline - 10, 124, 14, 2);

        const char* label = i == 0 ? "Volume" : i == 1 ? "Vibration" : "Ringtone";
        canvas_draw_str(canvas, 13, baseline, label);
        char value[40] = {0};
        if(i == 0) {
            snprintf(value, sizeof(value), "%u%%", (unsigned)app->profile.volume);
        } else if(i == 1) {
            strlcpy(value, app->profile.vibro ? "ON" : "OFF", sizeof(value));
        } else {
            const char* ringtone = app->profile.ringtone;
            if(!ringtone[0]) ringtone = "Default";
            strlcpy(value, ringtone, sizeof(value));
            char* extension = strrchr(value, '.');
            if(extension && strcmp(value, "None") != 0) *extension = '\0';
            while(value[0] && canvas_string_width(canvas, value) > 45U)
                value[strlen(value) - 1U] = '\0';
        }
        canvas_draw_str_aligned(canvas, 116, baseline, AlignRight, AlignBottom, value);
        if(app->sound_selected == i) {
            canvas_draw_str(canvas, i == 2 ? 63 : 83, baseline, "<");
            canvas_draw_str(canvas, 120, baseline, ">");
        }
    }
}

static void weave_settings_alerts_draw(Canvas* canvas, void* context) {
    WeaveSettings* app = *(WeaveSettings**)context;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    if(app->alerts_editing) weave_settings_alerts_profile(canvas, app);
    else weave_settings_alerts_overview(canvas, app);
}

static void weave_settings_show_alerts(WeaveSettings* app) {
    weave_settings_refresh_library(app);
    app->alerts_editing = false;
    app->alerts_selected = 0;
    app->sound_selected = 0;
    for(uint8_t i = 0; i < 4; ++i)
        weave_alert_load((WeaveAlertType)weave_settings_tabs[i], &app->alert_profiles[i]);
    /* General controls only the local sound level and vibration. The finder
     * continues to use its built-in sound, never an unrelated call/SMS tone. */
    if(app->alert_profiles[0].ringtone[0]) {
        app->alert_profiles[0].ringtone[0] = '\0';
        weave_alert_save(WeaveAlertFinder, &app->alert_profiles[0]);
    }
    app->profile_type = weave_settings_tabs[0];
    app->profile = app->alert_profiles[0];
    app->active_view = WeaveSettingsViewAlerts;
    view_commit_model(app->alerts_view, true);
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewAlerts);
}

static bool weave_settings_alerts_input(InputEvent* event, void* context) {
    WeaveSettings* app = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return true;
    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        if(app->alerts_editing) {
            weave_ringtone_stop();
            app->alerts_editing = false;
            view_commit_model(app->alerts_view, true);
        } else {
            weave_settings_show_menu(app, WeaveSettingsRoot);
        }
        return true;
    }
    if(!app->alerts_editing) {
        if((event->key == InputKeyLeft || event->key == InputKeyRight) &&
           event->type == InputTypeShort) {
            app->alerts_selected = (uint8_t)((app->alerts_selected +
                (event->key == InputKeyRight ? 1U : 3U)) % 4U);
            app->profile_type = weave_settings_tabs[app->alerts_selected];
            app->profile = app->alert_profiles[app->alerts_selected];
            view_commit_model(app->alerts_view, true);
        } else if(event->key == InputKeyOk && event->type == InputTypeShort) {
            app->alerts_editing = true;
            app->sound_selected = 0; /* Open every category on Volume. */
            view_commit_model(app->alerts_view, true);
        }
        return true;
    }
    const uint8_t row_count = weave_settings_alert_row_count(app);
    if(event->key == InputKeyUp || event->key == InputKeyDown) {
        if(event->type == InputTypeShort) {
            app->sound_selected = (uint8_t)((app->sound_selected +
                (event->key == InputKeyDown ? 1U : row_count - 1U)) % row_count);
            view_commit_model(app->alerts_view, true);
        }
        return true;
    }
    if(app->sound_selected == 2U && event->key == InputKeyOk &&
       event->type == InputTypeShort) {
        const uint8_t index = weave_settings_ring_index(app);
        /* A ringtone preview plays on the Flipper only; no phone RPC. */
        weave_ringtone_stop();
        if(index == 1U) weave_ringtone_preview_default();
        else if(index >= 2U) weave_ringtone_preview(app->ring_names[index - 2U]);
        return true;
    }
    if(event->key != InputKeyLeft && event->key != InputKeyRight &&
       event->key != InputKeyOk) return true;
    if(app->sound_selected == 0U) {
        if(event->key == InputKeyOk) return true;
        int next = (int)app->profile.volume + (event->key == InputKeyRight ? 5 : -5);
        if(next < 0) next = 0;
        if(next > 100) next = 100;
        app->profile.volume = (uint8_t)next;
    } else if(app->sound_selected == 1U) {
        if(event->key == InputKeyOk) app->profile.vibro = !app->profile.vibro;
        else app->profile.vibro = event->key == InputKeyRight;
    } else {
        const uint16_t count = (uint16_t)app->ring_count + 2U;
        uint16_t index = weave_settings_ring_index(app);
        index = (uint16_t)((index +
            (event->key == InputKeyRight ? 1U : count - 1U)) % count);
        const char* selected = index == 0U ? "None" :
            index == 1U ? "" : app->ring_names[index - 2U];
        strlcpy(app->profile.ringtone, selected, sizeof(app->profile.ringtone));
        weave_ringtone_stop();
    }
    /* A previous disabled profile becomes active when the user edits it. */
    app->profile.enabled = true;
    app->alert_profiles[app->alerts_selected] = app->profile;
    weave_alert_save((WeaveAlertType)app->profile_type, &app->profile);
    view_commit_model(app->alerts_view, true);
    return true;
}

static void weave_settings_show_dialog(
    WeaveSettings* app, const char* title, const char* message,
    WeaveSettingsDialog pending, bool danger) {
    app->pending = pending;
    dialog_ex_reset(app->dialog);
    dialog_ex_set_context(app->dialog, app);
    dialog_ex_set_result_callback(app->dialog, weave_settings_dialog_result);
    dialog_ex_set_header(app->dialog, title, 64, 5, AlignCenter, AlignTop);
    dialog_ex_set_text(app->dialog, message, 64, 29, AlignCenter, AlignCenter);
    dialog_ex_set_left_button_text(app->dialog,
        pending == WeaveSettingsDialogFindDevice ? "No" : "Back");
    if(pending == WeaveSettingsDialogFindDevice)
        dialog_ex_set_right_button_text(app->dialog, "Yes");
    else if(danger) dialog_ex_set_right_button_text(app->dialog, "Confirm");
    app->active_view = WeaveSettingsViewDialog;
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewDialog);
}

static void weave_settings_show_info(WeaveSettings* app, const char* title, const char* message) {
    weave_settings_show_dialog(app, title, message, WeaveSettingsDialogInfo, false);
}

static void weave_settings_load_name(WeaveSettings* app) {
    const char* current = furi_hal_version_get_name_ptr();
    strlcpy(app->hostname, current ? current : "FZ", sizeof(app->hostname));
    File* file = storage_file_alloc(app->storage);
    if(storage_file_open(file, WEAVE_NAME_FILE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char saved[9] = {0};
        uint16_t size = storage_file_read(file, saved, sizeof(saved));
        bool valid = size > 0 && size <= 8;
        for(uint16_t i = 0; valid && i < size; i++) {
            if((uint8_t)saved[i] < 0x20 || (uint8_t)saved[i] > 0x7E)
                valid = false;
        }
        if(valid) {
            saved[size] = '\0';
            strlcpy(app->hostname, saved, sizeof(app->hostname));
        }
    }
    storage_file_close(file);
    storage_file_free(file);
}

/* Popup timeout runs in the timer context; queue the reboot on our
 * dispatcher instead of restarting the device from a GUI/timer callback. */
#define WEAVE_SETTINGS_EVENT_REBOOT 1U

static void weave_settings_hostname_popup_done(void* context) {
    WeaveSettings* app = context;
    view_dispatcher_send_custom_event(app->dispatcher, WEAVE_SETTINGS_EVENT_REBOOT);
}

static bool weave_settings_custom_event(void* context, uint32_t event) {
    WeaveSettings* app = context;
    if(event != WEAVE_SETTINGS_EVENT_REBOOT ||
       app->active_view != WeaveSettingsViewHostnameSaved) return false;
    Power* power = furi_record_open(RECORD_POWER);
    power_reboot(power, PowerBootModeNormal);
    furi_record_close(RECORD_POWER);
    return true;
}

static void weave_settings_save_name(WeaveSettings* app) {
    storage_common_mkdir(app->storage, WEAVE_NAME_DIR);
    File* file = storage_file_alloc(app->storage);
    bool ok = storage_file_open(file, WEAVE_NAME_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        size_t length = strlen(app->hostname);
        ok = storage_file_write(file, app->hostname, length) == length;
    }
    storage_file_close(file);
    storage_file_free(file);
    if(ok) {
        furi_hal_version_set_custom_name(app->hostname);
        popup_reset(app->hostname_saved);
        /* Reset clears the callback and context too. Reinstall them or the
         * timeout would never trigger the automatic reboot. */
        popup_set_context(app->hostname_saved, app);
        popup_set_callback(app->hostname_saved, weave_settings_hostname_popup_done);
        popup_set_header(app->hostname_saved, "Hostname", 64, 18, AlignCenter, AlignCenter);
        popup_set_text(app->hostname_saved, "Updated", 64, 38, AlignCenter, AlignCenter);
        popup_set_timeout(app->hostname_saved, 1200);
        popup_enable_timeout(app->hostname_saved);
        app->active_view = WeaveSettingsViewHostnameSaved;
        view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewHostnameSaved);
    } else {
        weave_settings_show_info(app, "Hostname", "Cannot save name.");
    }
}

static void weave_settings_input_done(void* context) {
    WeaveSettings* app = context;
    if(app->hostname[0]) weave_settings_save_name(app);
    else weave_settings_show_menu(app, WeaveSettingsRoot);
}

static void weave_settings_show_about(WeaveSettings* app) {
    const Version* version = furi_hal_version_get_firmware_version();
    const char* fw = version ? version_get_version(version) : "Unknown";
    const uint8_t* mac = furi_hal_version_get_ble_mac();
    snprintf(
        app->about_text, sizeof(app->about_text),
        "Weave %s\nFirmware: %s\n"
        "Flipper reference: 1.5.1\n"
        "Upstream fixes: selected\n"
        "Model: %s (%s)\n"
        "Hostname: %s\nHW revision: %u\nRegion: %s\n"
        "BLE MAC: %02X:%02X:%02X:%02X:%02X:%02X\n"
        "Target: F7 / STM32WB55\n"
        "Apps: .fap (microSD)\n"
        "SD Card: %s\n",
        WEAVE_RELEASE, fw, furi_hal_version_get_model_name(),
        furi_hal_version_get_model_code(), app->hostname,
        furi_hal_version_get_hw_version(),
        furi_hal_version_get_hw_region_name(),
        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
        storage_sd_status(app->storage) == FSE_OK ? "Mounted" : "Unavailable");
    text_box_set_text(app->about, app->about_text);
    text_box_set_focus(app->about, TextBoxFocusStart);
    app->active_view = WeaveSettingsViewAbout;
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewAbout);
}

static void weave_settings_reboot_reset(WeaveSettings* app) {
    /* SD conservée ; les FAP natives sont restaurées si besoin au boot. */
    weave_factory_mark_restore(app->storage);
    furi_hal_rtc_reset_registers();
    furi_hal_rtc_set_flag(FuriHalRtcFlagStorageFormatInternal);
    Power* power = furi_record_open(RECORD_POWER);
    power_reboot(power, PowerBootModeNormal);
}

static void weave_settings_dialog_result(DialogExResult result, void* context) {
    WeaveSettings* app = context;
    if(result == DialogExResultRight) {
        if(app->pending == WeaveSettingsDialogSubGhzBypass ||
           app->pending == WeaveSettingsDialogSubGhzExtend) {
            const bool bypass = true;
            const bool extend = app->pending == WeaveSettingsDialogSubGhzExtend ? true :
                furi_hal_subghz_get_extended_range();
            if(!weave_radio_options_save(app->storage, bypass, extend))
                weave_settings_show_info(app, "Sub-GHz", "Could not save settings.");
            else weave_settings_show_menu(app, WeaveSettingsSubGhz);
            return;
        }
        if(app->pending == WeaveSettingsDialogFindDevice) {
            /* One new request only after explicit YES. No intermediate info page. */
            if(!weave_settings_find_device_request(app)) {
                weave_settings_show_info(app, "Find my Device", "SD unavailable.");
                return;
            }
            weave_settings_show_menu(app, WeaveSettingsRoot);
            return;
        }
        if(app->pending == WeaveSettingsDialogSdFormat) {
            FS_Error error = storage_sd_format(app->storage);
            if(error == FSE_OK) weave_settings_sd_categories(app);
            weave_settings_show_info(
                app, "SD Card", error == FSE_OK ? "Formatted." : "Format failed.");
            return;
        }
        if(app->pending == WeaveSettingsDialogResetFirst) {
            weave_settings_show_dialog(
                app, "Last warning", "Erase internal data?\nSD apps are kept.",
                WeaveSettingsDialogResetLast, true);
            return;
        }
        if(app->pending == WeaveSettingsDialogResetLast) {
            weave_settings_reboot_reset(app);
            return;
        }
    }
    weave_settings_show_menu(app, app->page);
}


#define WEAVE_FW_CHECK_REQ EXT_PATH(".weave/fw-check.req")
#define WEAVE_FW_CHECK_RESULT EXT_PATH(".weave/fw-check.result")
/* Both Android and desktop companions reply with TOKEN|ok|X.Y.Z or
 * TOKEN|error|... . The Flipper does not download or flash firmware itself. */
#define WEAVE_FW_CHECK_TICK_MS 200U
#define WEAVE_FW_CHECK_TIMEOUT_TICKS 300U /* 60 seconds including gateway/network delays */

static bool weave_settings_parse_version(
    const char* value, unsigned* major, unsigned* minor, unsigned* patch) {
    if(!value[0] || strlen(value) >= 20 ||
       strspn(value, "0123456789.") != strlen(value)) return false;
    char trailing = 0;
    return sscanf(value, "%u.%u.%u%c", major, minor, patch, &trailing) == 3;
}

static void weave_settings_firmware_draw(Canvas* canvas, void* context) {
    WeaveSettings* app = *(WeaveSettings**)context;
    canvas_clear(canvas);
    weave_ui_header(canvas, "Firmware Update");
    canvas_set_font(canvas, FontSecondary);
    if(app->firmware_status == WeaveFirmwareStatusChecking) {
        /* Animated 8-point spinner, drawn in the Flipper's monochrome canvas. */
        static const int8_t positions[8][2] = {
            {0,-10},{7,-7},{10,0},{7,7},{0,10},{-7,7},{-10,0},{-7,-7}
        };
        for(uint8_t i = 0; i < 8; ++i) {
            uint8_t step = (uint8_t)((i + app->firmware_spinner) % 8);
            if(step == 0 || step == 1 || step == 2)
                canvas_draw_disc(canvas, 64 + positions[i][0], 33 + positions[i][1],
                    step == 0 ? 2 : 1);
            else
                canvas_draw_dot(canvas, 64 + positions[i][0], 33 + positions[i][1]);
        }
        canvas_draw_str_aligned(canvas, 64, 57, AlignCenter, AlignBottom, "Checking latest...");
    } else if(app->firmware_status == WeaveFirmwareStatusAvailable) {
        char version_line[44];
        snprintf(version_line, sizeof(version_line), "v%s -> v%s", WEAVE_RELEASE, app->firmware_latest);
        canvas_draw_str_aligned(canvas, 64, 27, AlignCenter, AlignBottom, "Update available");
        if(canvas_string_width(canvas, version_line) < 126) {
            canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignBottom, version_line);
        } else {
            snprintf(version_line, sizeof(version_line), "Latest: v%.15s", app->firmware_latest);
            canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignBottom, version_line);
        }
        canvas_draw_str_aligned(canvas, 64, 57, AlignCenter, AlignBottom, "Install via companion");
    } else if(app->firmware_status == WeaveFirmwareStatusCurrent) {
        char version_line[28];
        snprintf(version_line, sizeof(version_line), "Weave v%s", WEAVE_RELEASE);
        canvas_draw_str_aligned(canvas, 64, 33, AlignCenter, AlignBottom, "Already up to date");
        canvas_draw_str_aligned(canvas, 64, 49, AlignCenter, AlignBottom, version_line);
    } else {
        /* Same result for no gateway, no Internet, no release, or SD errors. */
        canvas_draw_str_aligned(canvas, 64, 26, AlignCenter, AlignBottom, "Check unavailable");
        canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignBottom, "Gateway / Internet /");
        canvas_draw_str_aligned(canvas, 64, 56, AlignCenter, AlignBottom, "Release unavailable");
    }
}

/* A small, nonblocking file poll: we stay on this screen while the companion
 * checks GitHub, and reject results belonging to a previous check token. */
static void weave_settings_firmware_poll(WeaveSettings* app) {
    char result[96] = {0};
    File* file = storage_file_alloc(app->storage);
    if(storage_file_open(file, WEAVE_FW_CHECK_RESULT, FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t size = storage_file_read(file, result, sizeof(result) - 1);
        result[size] = '\0';
        storage_file_close(file);
    }
    storage_file_free(file);
    const size_t token_length = strlen(app->firmware_check_token);
    if(token_length == 0 || strncmp(result, app->firmware_check_token, token_length) != 0 ||
       result[token_length] != '|') return;
    const char* payload = result + token_length + 1;
    if(strncmp(payload, "ok|", 3) != 0) {
        app->firmware_status = WeaveFirmwareStatusUnavailable;
        return;
    }
    const char* version = payload + 3;
    unsigned a, b, c, x, y, z;
    if(!weave_settings_parse_version(version, &a, &b, &c) ||
       !weave_settings_parse_version(WEAVE_RELEASE, &x, &y, &z)) {
        app->firmware_status = WeaveFirmwareStatusUnavailable;
        return;
    }
    const bool newer = a > x || (a == x && (b > y || (b == y && c > z)));
    strlcpy(app->firmware_latest, version, sizeof(app->firmware_latest));
    app->firmware_status = newer ? WeaveFirmwareStatusAvailable : WeaveFirmwareStatusCurrent;
}

static void weave_settings_firmware_tick(void* context) {
    WeaveSettings* app = context;
    if(app->active_view != WeaveSettingsViewFirmwareUpdate ||
       app->firmware_status != WeaveFirmwareStatusChecking) return;
    app->firmware_spinner = (uint8_t)((app->firmware_spinner + 1) % 8);
    app->firmware_elapsed_ticks++;
    if(app->firmware_elapsed_ticks >= WEAVE_FW_CHECK_TIMEOUT_TICKS) {
        app->firmware_status = WeaveFirmwareStatusUnavailable;
    } else if(app->firmware_elapsed_ticks % 3 == 0) {
        weave_settings_firmware_poll(app);
    }
    view_commit_model(app->firmware_update_view, true);
}

static bool weave_settings_firmware_input(InputEvent* event, void* context) {
    WeaveSettings* app = context;
    if(event->type == InputTypeShort && event->key == InputKeyOk &&
       app->firmware_status != WeaveFirmwareStatusChecking) {
        /* OK on a result retries the check; BACK returns to Settings. */
        weave_settings_check_firmware(app);
        return true;
    }
    return false;
}

/* Every visit issues a fresh token. A previous GitHub response is ignored, so
 * "up to date" can never be shown from a cached, unrelated request. */
static void weave_settings_check_firmware(WeaveSettings* app) {
    app->firmware_status = WeaveFirmwareStatusChecking;
    app->firmware_elapsed_ticks = 0;
    app->firmware_spinner = 0;
    app->firmware_latest[0] = '\0';
    app->firmware_check_token[0] = '\0';
    app->active_view = WeaveSettingsViewFirmwareUpdate;
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewFirmwareUpdate);

    if(storage_sd_status(app->storage) != FSE_OK) {
        app->firmware_status = WeaveFirmwareStatusUnavailable;
    } else {
        storage_simply_mkdir(app->storage, EXT_PATH(".weave"));
        snprintf(app->firmware_check_token, sizeof(app->firmware_check_token), "%lu-%lu",
            (unsigned long)furi_hal_rtc_get_timestamp(), (unsigned long)furi_get_tick());
        File* file = storage_file_alloc(app->storage);
        bool written = storage_file_open(file, WEAVE_FW_CHECK_REQ, FSAM_WRITE, FSOM_CREATE_ALWAYS);
        if(written) {
            const size_t length = strlen(app->firmware_check_token);
            written = storage_file_write(file, app->firmware_check_token, length) == length;
        }
        if(storage_file_is_open(file)) storage_file_close(file);
        storage_file_free(file);
        if(!written) app->firmware_status = WeaveFirmwareStatusUnavailable;
    }
    view_commit_model(app->firmware_update_view, true);
}

#define WEAVE_FIND_DEVICE_FILE EXT_PATH(".weave/find-device.req")
static bool weave_settings_find_device_request(WeaveSettings* app) {
    /* The companion polls this file over an authenticated Flipper RPC link.
     * No direct network connection or unauthenticated BLE advertisement. */
    if(storage_sd_status(app->storage) != FSE_OK) return false;
    storage_simply_mkdir(app->storage, EXT_PATH(".weave"));
    char token[48];
    snprintf(token, sizeof(token), "%lu-%lu", (unsigned long)furi_hal_rtc_get_timestamp(),
             (unsigned long)furi_get_tick());
    File* file = storage_file_alloc(app->storage);
    bool ok = storage_file_open(file, WEAVE_FIND_DEVICE_FILE, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) ok = storage_file_write(file, token, strlen(token)) == strlen(token);
    if(storage_file_is_open(file)) storage_file_close(file);
    storage_file_free(file);
    return ok;
}
static void weave_settings_select(void* context, uint32_t index) {
    WeaveSettings* app = context;
    if(app->page == WeaveSettingsRoot) {
        switch(index) {
        case 0:
            text_input_reset(app->input);
            weave_settings_load_name(app);
            text_input_set_header_text(app->input, "Hostname (max 8)");
            text_input_set_result_callback(app->input, weave_settings_input_done,
                app, app->hostname, sizeof(app->hostname), false);
            text_input_set_minimum_length(app->input, 1);
            app->active_view = WeaveSettingsViewInput;
            view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewInput);
            break;
        case 1:
            weave_settings_show_about(app);
            break;
        case 2:
            weave_settings_show_brightness(app);
            break;
        case 3:
            weave_settings_show_storage(app);
            break;
        case 4:
            weave_settings_check_firmware(app);
            break;
        case 5:
            weave_settings_show_dialog(app, "Factory reset",
                "Erase settings/data?\nSD apps are kept.",
                WeaveSettingsDialogResetFirst, true);
            break;
        case 8:
            weave_settings_show_alerts(app);
            break;
        case 7:
            weave_settings_show_dialog(app, "Find my Device",
                "Ring the connected\nphone or computer?",
                WeaveSettingsDialogFindDevice, false);
            break;
        case 9:
            weave_settings_show_menu(app, WeaveSettingsSubGhz);
            break;
        case 6:
            /* No nested loader launch: the current Settings app already owns
             * the GUI, so display the upstream module in this dispatcher. */
            app->active_view = WeaveSettingsViewClock;
            view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewClock);
            break;
        }
    } else if(app->page == WeaveSettingsSubGhz) {
        if(index == 0) {
            if(furi_hal_region_get_tx_bypass()) {
                if(!weave_radio_options_save(app->storage, false, false))
                    weave_settings_show_info(app, "Sub-GHz", "Could not save settings.");
                else weave_settings_show_menu(app, WeaveSettingsSubGhz);
            } else {
                weave_settings_show_dialog(app, "Bypass Region Lock",
                    "TX restrictions off.\nKnow local radio laws?",
                    WeaveSettingsDialogSubGhzBypass, true);
            }
        } else if(index == 1) {
            if(furi_hal_subghz_get_extended_range()) {
                if(!weave_radio_options_save(app->storage, true, false))
                    weave_settings_show_info(app, "Sub-GHz", "Could not save settings.");
                else weave_settings_show_menu(app, WeaveSettingsSubGhz);
            } else if(!furi_hal_region_get_tx_bypass()) {
                weave_settings_show_info(app, "Sub-GHz", "Enable bypass first.");
            } else {
                weave_settings_show_dialog(app, "Extend Freq Bands",
                    "Out-of-spec tuning.\nMay damage hardware!",
                    WeaveSettingsDialogSubGhzExtend, true);
            }
        }
    } else if(app->page == WeaveSettingsSd) {
        FS_Error error;
        switch(index) {
        case 0:
            error = storage_sd_mount(app->storage);
            if(error == FSE_OK) weave_settings_sd_categories(app);
            weave_settings_show_info(app, "SD Card",
                error == FSE_OK ? "Mounted." : "Mount failed.");
            break;
        case 1:
            error = storage_sd_unmount(app->storage);
            weave_settings_show_info(app, "SD Card",
                error == FSE_OK ? "Unmounted." : "Unmount failed.");
            break;
        case 2:
            weave_settings_show_dialog(app, "Format SD Card",
                "Erase all SD data?", WeaveSettingsDialogSdFormat, true);
            break;
        }
    }
}

static void weave_settings_show_menu(WeaveSettings* app, WeaveSettingsPage page) {
    app->page = page;
    submenu_reset(app->menu);
    if(page == WeaveSettingsRoot) {
        submenu_set_header(app->menu, "Settings");
        submenu_add_item(app->menu, "Hostname", 0, weave_settings_select, app);
        submenu_add_item(app->menu, "Clock & Alarm", 6, weave_settings_select, app);
        submenu_add_item(app->menu, "Brightness", 2, weave_settings_select, app);
        submenu_add_item(app->menu, "Storage", 3, weave_settings_select, app);
        submenu_add_item(app->menu, "Sub-GHz", 9, weave_settings_select, app);
        submenu_add_item(app->menu, "Ringtones & Alerts", 8, weave_settings_select, app);
        submenu_add_item(app->menu, "Find my Device", 7, weave_settings_select, app);
        submenu_add_item(app->menu, "Firmware Update", 4, weave_settings_select, app);
        submenu_add_item(app->menu, "Factory Reset", 5, weave_settings_select, app);
        submenu_add_item(app->menu, "About", 1, weave_settings_select, app);
    } else if(page == WeaveSettingsSubGhz) {
        submenu_set_header(app->menu, "Sub-GHz");
        submenu_add_item(app->menu,
            furi_hal_region_get_tx_bypass() ? "Region Bypass: ON" : "Region Bypass: OFF",
            0, weave_settings_select, app);
        submenu_add_item(app->menu,
            furi_hal_subghz_get_extended_range() ? "Extend Bands: ON" : "Extend Bands: OFF",
            1, weave_settings_select, app);
    } else if(page == WeaveSettingsSd) {
        submenu_set_header(app->menu, "SD Card");
        submenu_add_item(app->menu, "Mount SD", 0, weave_settings_select, app);
        submenu_add_item(app->menu, "Unmount SD", 1, weave_settings_select, app);
        submenu_add_item(app->menu, "Format SD", 2, weave_settings_select, app);
    }
    app->active_view = WeaveSettingsViewMenu;
    view_dispatcher_switch_to_view(app->dispatcher, WeaveSettingsViewMenu);
}

static bool weave_settings_back(void* context) {
    WeaveSettings* app = context;
    if(app->active_view == WeaveSettingsViewHostnameSaved) return true;
    if(app->active_view == WeaveSettingsViewFirmwareUpdate) {
        weave_settings_show_menu(app, WeaveSettingsRoot);
        return true;
    }
    if(app->active_view == WeaveSettingsViewAlerts) {
        if(app->alerts_editing) {
            weave_ringtone_stop();
            app->alerts_editing = false;
            view_commit_model(app->alerts_view, true);
        } else weave_settings_show_menu(app, WeaveSettingsRoot);
        return true;
    }
    if(app->active_view == WeaveSettingsViewStorage) {
        weave_settings_show_menu(app, WeaveSettingsRoot);
    } else if(app->active_view == WeaveSettingsViewBrightness) {
        weave_settings_brightness_save(app);
        weave_settings_show_menu(app, WeaveSettingsRoot);
    } else if(app->active_view == WeaveSettingsViewMenu) {
        if(app->page == WeaveSettingsRoot) view_dispatcher_stop(app->dispatcher);
        else if(app->page == WeaveSettingsSd)
            weave_settings_show_storage(app);
        else weave_settings_show_menu(app, WeaveSettingsRoot);
    } else {
        weave_settings_show_menu(app, app->page);
    }
    return true;
}

int32_t weave_settings_app(void* context) {
    UNUSED(context);
    WeaveSettings* app = malloc(sizeof(WeaveSettings));
    memset(app, 0, sizeof(*app));
    app->gui = furi_record_open(RECORD_GUI);
    app->storage = furi_record_open(RECORD_STORAGE);
    app->notification = furi_record_open(RECORD_NOTIFICATION);
    app->dispatcher = view_dispatcher_alloc();
    app->menu = submenu_alloc();
    app->input = text_input_alloc();
    app->about = text_box_alloc();
    app->dialog = dialog_ex_alloc();
    app->hostname_saved = popup_alloc();
    popup_set_context(app->hostname_saved, app);
    popup_set_callback(app->hostname_saved, weave_settings_hostname_popup_done);
    app->storage_view = view_alloc();
    view_allocate_model(app->storage_view, ViewModelTypeLockFree, sizeof(WeaveSettings*));
    WeaveSettings** storage_model = view_get_model(app->storage_view);
    *storage_model = app;
    view_commit_model(app->storage_view, false);
    view_set_context(app->storage_view, app);
    view_set_draw_callback(app->storage_view, weave_settings_draw_storage);
    view_set_input_callback(app->storage_view, weave_settings_storage_input);
    /* Clock module uses this dispatcher's event loop for its live RTC clock. */
    app->clock_view = clock_settings_module_alloc(view_dispatcher_get_event_loop(app->dispatcher));
    app->firmware_update_view = view_alloc();
    view_allocate_model(app->firmware_update_view, ViewModelTypeLockFree, sizeof(WeaveSettings*));
    *(WeaveSettings**)view_get_model(app->firmware_update_view) = app;
    view_commit_model(app->firmware_update_view, false);
    view_set_context(app->firmware_update_view, app);
    view_set_draw_callback(app->firmware_update_view, weave_settings_firmware_draw);
    view_set_input_callback(app->firmware_update_view, weave_settings_firmware_input);
    app->alerts_view=view_alloc();
    view_allocate_model(app->alerts_view,ViewModelTypeLockFree,sizeof(WeaveSettings*));
    *(WeaveSettings**)view_get_model(app->alerts_view)=app;
    view_commit_model(app->alerts_view,false);
    view_set_context(app->alerts_view,app);
    view_set_draw_callback(app->alerts_view,weave_settings_alerts_draw);
    view_set_input_callback(app->alerts_view,weave_settings_alerts_input);
    app->brightness_view = view_alloc();
    view_allocate_model(app->brightness_view, ViewModelTypeLockFree, sizeof(WeaveSettings*));
    WeaveSettings** brightness_model = view_get_model(app->brightness_view);
    *brightness_model = app;
    view_commit_model(app->brightness_view, false);
    view_set_context(app->brightness_view, app);
    view_set_draw_callback(app->brightness_view, weave_settings_draw_brightness);
    view_set_input_callback(app->brightness_view, weave_settings_brightness_input);
    view_dispatcher_set_event_callback_context(app->dispatcher, app);
    view_dispatcher_set_custom_event_callback(app->dispatcher, weave_settings_custom_event);
    view_dispatcher_set_navigation_event_callback(app->dispatcher, weave_settings_back);
    view_dispatcher_set_tick_event_callback(app->dispatcher, weave_settings_firmware_tick,
        WEAVE_FW_CHECK_TICK_MS);
    view_dispatcher_attach_to_gui(app->dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewMenu, submenu_get_view(app->menu));
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewInput, text_input_get_view(app->input));
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewAbout, text_box_get_view(app->about));
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewDialog, dialog_ex_get_view(app->dialog));
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewHostnameSaved, popup_get_view(app->hostname_saved));
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewStorage, app->storage_view);
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewBrightness, app->brightness_view);
    view_dispatcher_add_view(app->dispatcher, WeaveSettingsViewFirmwareUpdate, app->firmware_update_view);
    view_dispatcher_add_view(app->dispatcher,WeaveSettingsViewAlerts,app->alerts_view);
    view_dispatcher_add_view(
        app->dispatcher, WeaveSettingsViewClock, clock_settings_module_get_view(app->clock_view));
    dialog_ex_set_context(app->dialog, app);
    dialog_ex_set_result_callback(app->dialog, weave_settings_dialog_result);
    weave_settings_load_name(app);
    weave_settings_show_menu(app, WeaveSettingsRoot);
    view_dispatcher_run(app->dispatcher);
    weave_settings_brightness_save(app);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewFirmwareUpdate);
    view_dispatcher_remove_view(app->dispatcher,WeaveSettingsViewAlerts);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewClock);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewBrightness);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewStorage);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewHostnameSaved);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewDialog);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewAbout);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewInput);
    view_dispatcher_remove_view(app->dispatcher, WeaveSettingsViewMenu);
    clock_settings_module_free(app->clock_view);
    view_free(app->firmware_update_view);
    view_free(app->alerts_view);
    view_free(app->brightness_view);
    view_free(app->storage_view);
    dialog_ex_free(app->dialog);
    popup_free(app->hostname_saved);
    text_box_free(app->about);
    text_input_free(app->input);
    submenu_free(app->menu);
    view_dispatcher_free(app->dispatcher);
    furi_record_close(RECORD_NOTIFICATION);
    furi_record_close(RECORD_STORAGE);
    furi_record_close(RECORD_GUI);
    free(app);
    return 0;
}
