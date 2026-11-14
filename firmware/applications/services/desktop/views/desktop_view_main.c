#include <furi.h>
#include <furi_hal.h>
#include <applications.h>
#include <assets_icons.h>
#include <gui/canvas.h>
#include <gui/weave_ui.h>
#include <gui/view.h>
#include <input/input.h>
#include <stdio.h>

#include "desktop_view_main.h"

typedef enum {
    WeavePageClock,
    WeavePageMenu,
} WeavePage;

typedef struct {
    WeavePage page;
    uint32_t selected;
    uint32_t animation_tick;
} WeaveMainModel;

struct DesktopMainView {
    View* view;
    DesktopMainViewCallback callback;
    void* context;
};

/* A horizontal three-tile launcher fits the 128x64 display. */
#define WEAVE_LAUNCHER_COUNT 3U

static const char* const weave_launcher_labels[WEAVE_LAUNCHER_COUNT] = {
    "Apps", "Files", "Settings",
};

/* Mini 3x5 digits inside the battery frame; no fonts outside the icon. */
static const uint8_t weave_battery_digits[11][5] = {
    {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7},
    {7, 1, 7, 1, 7}, {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7},
    {7, 4, 7, 5, 7}, {7, 1, 2, 2, 2}, {7, 5, 7, 5, 7},
    {7, 5, 7, 1, 7}, {5, 1, 2, 4, 5}, /* % */
};

static void weave_battery_draw(Canvas* canvas, uint32_t battery) {
    char value[5];
    if(battery > 100) battery = 100;
    snprintf(value, sizeof(value), "%lu%%", (unsigned long)battery);
    /* Slim 18x8 battery; keep clear of the sky and clock baseline. */
    canvas_draw_frame(canvas, 107, 1, 18, 8);
    canvas_draw_box(canvas, 125, 3, 2, 4);
    uint8_t count = 0;
    while(value[count]) count++;
    int32_t x = 108 + (15 - (count * 4 - 1)) / 2;
    for(uint8_t i = 0; i < count; i++) {
        uint8_t glyph = value[i] == '%' ? 10 : (uint8_t)(value[i] - '0');
        for(uint8_t row = 0; row < 5; row++) {
            for(uint8_t col = 0; col < 3; col++) {
                if(weave_battery_digits[glyph][row] & (1 << (2 - col))) {
                    canvas_draw_dot(canvas, x + i * 4 + col, 2 + row);
                }
            }
        }
    }
}

/* Animated 1-bit sea. 24 frames per dive: the dolphin enters the water,
 * disappears below the surface, resurfaces and then leaps to the right. The
 * water mask is applied after the sprite so immersion is pixel-accurate. */
static void weave_ocean_draw(Canvas* canvas, uint32_t tick) {
    static const uint8_t waterline[16] = {58, 58, 57, 57, 58, 59, 59, 58,
                                          58, 58, 57, 57, 58, 59, 59, 58};
    /* Top-left sprite Y, not its centre. Values 65+ hide it under water. */
    static const int8_t jump_y[24] = {
        65, 65, 65, 64, 61, 58, 54, 50, 47, 45, 44, 44,
        44, 45, 47, 50, 54, 58, 61, 64, 65, 65, 65, 65,
    };
    uint32_t phase = tick % COUNT_OF(jump_y);
    int32_t x = (int32_t)((tick * 3U) % 163U) - 27;
    const Icon* dolphin = phase < 9 ? &I_weave_dolphin_rise_22x12 :
        phase < 15 ? &I_weave_dolphin_glide_22x12 :
                     &I_weave_dolphin_dive_22x12;
    if(jump_y[phase] < 64) {
        canvas_draw_icon(canvas, x, jump_y[phase], dolphin);
    }
    /* The sea occludes the sprite, frame by frame, rather than allowing it
     * to float on top of the surface even when it should be submerged. */
    canvas_set_color(canvas, ColorWhite);
    uint32_t offset = (tick * 2U) % COUNT_OF(waterline);
    for(int32_t px = 0; px < 128; px++) {
        uint32_t idx = (px + COUNT_OF(waterline) - offset) % COUNT_OF(waterline);
        uint8_t y = waterline[idx];
        canvas_draw_line(canvas, px, y, px, 63);
    }
    canvas_set_color(canvas, ColorBlack);
    for(int32_t px = 0; px < 128; px++) {
        uint32_t idx = (px + COUNT_OF(waterline) - offset) % COUNT_OF(waterline);
        canvas_draw_dot(canvas, px, waterline[idx]);
    }
    /* Short splash only at the moment the head leaves/re-enters the water. */
    if(phase == 5 || phase == 6 || phase == 17 || phase == 18) {
        int32_t splash_x = x + (phase < 12 ? 2 : 19);
        if(splash_x >= 0 && splash_x < 128) {
            canvas_draw_dot(canvas, splash_x, 53);
            if(splash_x + 3 < 128) canvas_draw_dot(canvas, splash_x + 3, 55);
        }
    }
}

static void weave_sun_draw(Canvas* canvas, int32_t x, int32_t y, uint32_t tick) {
    canvas_draw_disc(canvas, x, y, 3);
    /* Rays alternate a single pixel, a subtle sparkle rather than blinking. */
    uint8_t radius = tick % 4 < 2 ? 6 : 5;
    canvas_draw_dot(canvas, x - radius, y);
    canvas_draw_dot(canvas, x + radius, y);
    canvas_draw_dot(canvas, x, y - radius);
    canvas_draw_dot(canvas, x, y + radius);
    if(tick % 3) {
        canvas_draw_dot(canvas, x - 4, y - 4);
        canvas_draw_dot(canvas, x + 4, y + 4);
    } else {
        canvas_draw_dot(canvas, x + 4, y - 4);
        canvas_draw_dot(canvas, x - 4, y + 4);
    }
}

static void weave_moon_draw(Canvas* canvas, int32_t x, int32_t y) {
    canvas_draw_disc(canvas, x, y, 4);
    canvas_set_color(canvas, ColorWhite);
    canvas_draw_disc(canvas, x + 2, y - 2, 3);
    canvas_set_color(canvas, ColorBlack);
}

static void weave_cloud_draw(Canvas* canvas, int32_t x, int32_t y) {
    /* Hollow 17x6 cloud avoids large black patches on a 128x64 LCD. */
    canvas_draw_line(canvas, x + 1, y + 5, x + 16, y + 5);
    canvas_draw_line(canvas, x, y + 3, x, y + 4);
    canvas_draw_line(canvas, x + 1, y + 2, x + 4, y + 2);
    canvas_draw_line(canvas, x + 5, y, x + 9, y);
    canvas_draw_line(canvas, x + 4, y + 1, x + 5, y + 1);
    canvas_draw_line(canvas, x + 10, y + 1, x + 11, y + 1);
    canvas_draw_line(canvas, x + 12, y + 2, x + 15, y + 2);
    canvas_draw_line(canvas, x + 16, y + 3, x + 16, y + 4);
}

/* DateTime comes from the Flipper's RTC, i.e. its configured local time.
 * No network/timezone service or floating-point maths are needed. Sunrise
 * and sunset are illustrative fixed times (07:00 and 19:00 local), not GPS
 * or astronomical calculations. Twilight gradually exchanges clouds/stars. */
static void weave_sky_draw(Canvas* canvas, const DateTime* dt, uint32_t tick) {
    uint32_t minutes = (uint32_t)dt->hour * 60U + dt->minute;
    const uint32_t sunrise = 7U * 60U;
    const uint32_t sunset = 19U * 60U;
    bool day = minutes >= sunrise && minutes < sunset;
    uint32_t elapsed = day ? (minutes - sunrise) :
        (minutes >= sunset ? minutes - sunset : minutes + 1440U - sunset);
    uint32_t duration = 720U;
    int32_t x = 13 + (int32_t)(elapsed * 88U / duration);
    /* Parabolic path from left horizon to zenith to right horizon. */
    uint32_t height = elapsed * (duration - elapsed) * 11U /
                      (360U * 360U);
    int32_t y = 17 - (int32_t)height;

    /* Approximate 45-minute dawn and dusk: progressively remove stars
     * and introduce clouds, rather than suddenly swapping the whole sky. */
    uint32_t daylight = day ? 4U : 0U;
    uint32_t from_sunrise = minutes > sunrise ? minutes - sunrise : sunrise - minutes;
    uint32_t from_sunset = minutes > sunset ? minutes - sunset : sunset - minutes;
    if(from_sunrise < 45U) {
        daylight = minutes >= sunrise ? 2U + 2U * from_sunrise / 45U :
                                          2U - 2U * from_sunrise / 45U;
    } else if(from_sunset < 45U) {
        daylight = minutes < sunset ? 2U + 2U * from_sunset / 45U :
                                        2U - 2U * from_sunset / 45U;
    }

    /* Day clouds travel at independent speeds; always stay above the date. */
    static const uint8_t cloud_start[3] = {7, 53, 96};
    for(uint32_t i = 0; i < 3; i++) {
        if(i >= daylight) continue;
        int32_t cx = ((int32_t)cloud_start[i] + (int32_t)(tick / (3 + i)) * (i + 1)) % 154 - 18;
        if(cx > 0 && cx < 110) weave_cloud_draw(canvas, cx, i % 2 ? 11 : 5);
    }

    /* Sparse, twinkling stars on the upper edge of the screen at night. */
    static const uint8_t star_x[6] = {7, 29, 43, 80, 95, 116};
    static const uint8_t star_y[6] = {6, 14, 5, 13, 3, 17};
    for(uint32_t i = 0; i < 6U - (daylight * 6U / 4U); i++) {
        if((tick + i * 7U) % 9U != 0U) {
            canvas_draw_dot(canvas, star_x[i], star_y[i]);
            if((tick + i) % 7U == 0) {
                canvas_draw_dot(canvas, star_x[i] + 1, star_y[i]);
                canvas_draw_dot(canvas, star_x[i], star_y[i] + 1);
            }
        }
    }

    if(day) weave_sun_draw(canvas, x, y, tick);
    else weave_moon_draw(canvas, x, y);
}

static void weave_clock_draw(Canvas* canvas, uint32_t tick) {
    DateTime date;
    char line[32];

    furi_hal_rtc_get_datetime(&date);
    weave_sky_draw(canvas, &date, tick);
    weave_battery_draw(canvas, furi_hal_power_get_pct());
    canvas_set_font(canvas, FontBigNumbers);
    snprintf(line, sizeof(line), "%02u:%02u", date.hour, date.minute);
    canvas_draw_str_aligned(canvas, 64, 30, AlignCenter, AlignBottom, line);
    canvas_set_font(canvas, FontSecondary);
    snprintf(line, sizeof(line), "%02u/%02u/%04u", date.day, date.month, date.year);
    canvas_draw_str_aligned(canvas, 64, 42, AlignCenter, AlignBottom, line);
    weave_ocean_draw(canvas, tick);
}

/* Compact launcher pictograms with 1px strokes. */
static void weave_launcher_icon(Canvas* canvas, uint32_t index, int32_t cx, int32_t cy) {
    if(index == 0) {
        canvas_draw_icon(canvas, cx - 7, cy - 7, &A_Plugins_14);
    } else if(index == 1) {
        canvas_draw_icon(canvas, cx - 7, cy - 7, &A_FileManager_14);
    } else {
        canvas_draw_icon(canvas, cx - 7, cy - 7, &A_Settings_14);
    }
}

static void weave_launcher_draw(Canvas* canvas, const WeaveMainModel* model) {
    canvas_set_color(canvas, ColorBlack);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str_aligned(canvas, 64, 10, AlignCenter, AlignBottom, "Menu");
    /* 3 x 40px tiles, with 2px spacing. The icons share one vertical axis. */
    for(uint32_t index = 0; index < WEAVE_LAUNCHER_COUNT; index++) {
        const int32_t x = 2 + (int32_t)index * 42;
        const int32_t cx = x + 20;
        const bool selected = model->selected == index;
        canvas_set_color(canvas, ColorBlack);
        if(selected) {
            canvas_draw_rbox(canvas, x, 19, 40, 37, 3);
            canvas_set_color(canvas, ColorWhite);
        } else {
            canvas_draw_rframe(canvas, x, 19, 40, 37, 3);
        }
        weave_launcher_icon(canvas, index, cx, 32);
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(
            canvas, cx, 51, AlignCenter, AlignBottom, weave_launcher_labels[index]);
    }
    canvas_set_color(canvas, ColorBlack);
}

static void weave_main_draw(Canvas* canvas, void* model) {
    const WeaveMainModel* state = model;
    canvas_clear(canvas);
    if(state->page == WeavePageClock) weave_clock_draw(canvas, state->animation_tick);
    else weave_launcher_draw(canvas, state);
}

static void weave_open_selected(DesktopMainView* main_view, WeaveMainModel* model) {
    if(model->page == WeavePageMenu) {
        switch(model->selected) {
        case 0:
            main_view->callback(DesktopMainEventOpenPackages, main_view->context);
            break;
        case 1:
            main_view->callback(DesktopMainEventOpenArchive, main_view->context);
            break;
        case 2:
            main_view->callback(DesktopMainEventOpenSettingsApp, main_view->context);
            break;
        }
    }
}

static bool weave_main_input(InputEvent* event, void* context) {
    DesktopMainView* main_view = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat)
        return true;
    with_view_model(main_view->view, WeaveMainModel * model, {
        if(model->page == WeavePageClock) {
            if(event->type == InputTypeShort) {
                model->page = WeavePageMenu;
                model->selected = 0;
            }
        } else if(event->key == InputKeyBack && event->type == InputTypeShort) {
            model->page = model->page == WeavePageMenu ? WeavePageClock : WeavePageMenu;
            model->selected = 0;
        } else if(event->key == InputKeyRight || event->key == InputKeyDown) {
            model->selected = (model->selected + 1U) % WEAVE_LAUNCHER_COUNT;
        } else if(event->key == InputKeyLeft || event->key == InputKeyUp) {
            model->selected = (model->selected + WEAVE_LAUNCHER_COUNT - 1U) %
                              WEAVE_LAUNCHER_COUNT;
        } else if(event->key == InputKeyOk && event->type == InputTypeShort) {
            weave_open_selected(main_view, model);
        }
    }, true);
    return true;
}

void desktop_main_set_callback(
    DesktopMainView* main_view, DesktopMainViewCallback callback, void* context) {
    furi_assert(main_view);
    main_view->callback = callback;
    main_view->context = context;
}

View* desktop_main_get_view(DesktopMainView* main_view) {
    return main_view->view;
}

uint32_t desktop_main_get_selection(DesktopMainView* main_view) {
    uint32_t selected = 0;
    with_view_model(main_view->view, WeaveMainModel * model,
        { selected = model->selected; }, false);
    return selected;
}

void desktop_main_reset(DesktopMainView* main_view) {
    with_view_model(main_view->view, WeaveMainModel * model, {
        model->page = WeavePageClock;
        model->selected = 0;
        model->animation_tick = 0;
    }, true);
}

void desktop_main_refresh(DesktopMainView* main_view) {
    with_view_model(main_view->view, WeaveMainModel * model,
        { if(model->page == WeavePageClock) model->animation_tick++; }, true);
}

void desktop_main_set_dummy_mode_state(DesktopMainView* main_view, bool dummy_mode) {
    UNUSED(main_view);
    UNUSED(dummy_mode);
}

DesktopMainView* desktop_main_alloc(void) {
    DesktopMainView* main_view = malloc(sizeof(DesktopMainView));
    main_view->view = view_alloc();
    view_allocate_model(main_view->view, ViewModelTypeLocking, sizeof(WeaveMainModel));
    view_set_context(main_view->view, main_view);
    view_set_draw_callback(main_view->view, weave_main_draw);
    view_set_input_callback(main_view->view, weave_main_input);
    desktop_main_reset(main_view);
    return main_view;
}

void desktop_main_free(DesktopMainView* main_view) {
    view_free(main_view->view);
    free(main_view);
}
