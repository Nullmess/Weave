#pragma once
#include <furi.h>

/* All alert melodies are stored under /ext/.weave/ringtones/. Supported
 * files: simple Hz + milliseconds .tone, Flipper Music .fmf, RTTTL .txt/.rtttl. */
typedef enum {
    WeaveAlertCall = 0,
    WeaveAlertSms = 1,
    WeaveAlertAlarm = 2,
    WeaveAlertFinder = 3,
} WeaveAlertType;

typedef struct {
    bool enabled;
    uint8_t volume; /* 0..100 */
    bool vibro;
    char ringtone[40]; /* empty = built-in */
} WeaveAlertSettings;

bool weave_alert_load(WeaveAlertType type, WeaveAlertSettings* settings);
bool weave_alert_save(WeaveAlertType type, const WeaveAlertSettings* settings);
bool weave_ringtone_valid_filename(const char* name);
void weave_ringtone_init(void);
void weave_ringtone_reap_idle(void);
bool weave_ringtone_has_selection(void);
void weave_ringtone_start(bool loud, uint32_t duration_ms);
void weave_ringtone_start_profile(WeaveAlertType type);
void weave_ringtone_preview(const char* filename);
void weave_ringtone_preview_default(void);
void weave_ringtone_stop(void);
void weave_ringtone_find_flipper(void);
void weave_ringtone_notify(WeaveAlertType type);
void weave_ringtone_end_call(void);
void weave_ringtone_prepare_call(void);
