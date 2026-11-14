#include "weave_ringtone.h"
#include <furi_hal.h>
#include <storage/storage.h>
#include <notification/notification_app.h>
#include <notification/notification_messages.h>
#include <gui/gui.h>
#include <gui/view_port.h>
#include <input/input.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RING_DIR EXT_PATH(".weave/ringtones")
#define RING_MAX_NOTES 512U
#define RING_MAX_TEXT 16384U

typedef struct {uint16_t frequency; uint16_t duration;} WeaveTone;
typedef struct {
    bool stop;
    bool repeat;
    bool vibro;
    bool vibro_once;
    uint8_t volume;
    uint32_t completion_id;
    char filename[40];
} WeaveRingCommand;
static FuriMessageQueue* ring_queue;
static FuriMessageQueue* alert_requests;
static FuriMessageQueue* call_stop_requests;
static FuriMessageQueue* alert_audio_completed;
/* NFC and other large FAPs must not pay for the audio/overlay threads at boot.
 * Allocate the queues at startup but start these relatively large stacks only
 * when an audible notification or a Find-my-Flipper event is requested. */
static FuriMutex* weave_audio_start_mutex;
static bool weave_audio_worker_running;
static bool weave_alert_worker_running;
static int32_t weave_ringtone_worker(void* context);
static int32_t weave_alert_worker(void* context);

static bool weave_ringtone_ensure_audio_worker(void) {
    if(!ring_queue || !weave_audio_start_mutex) return false;
    furi_check(furi_mutex_acquire(weave_audio_start_mutex, FuriWaitForever) == FuriStatusOk);
    if(!weave_audio_worker_running) {
        FuriThread* thread = furi_thread_alloc_ex("WeaveRingtone", 4096, weave_ringtone_worker, NULL);
        if(thread) {
            weave_audio_worker_running = true;
            furi_thread_start(thread);
        }
    }
    bool ready = weave_audio_worker_running;
    furi_check(furi_mutex_release(weave_audio_start_mutex) == FuriStatusOk);
    return ready;
}

static bool weave_ringtone_ensure_alert_worker(void) {
    if(!alert_requests || !weave_audio_start_mutex) return false;
    furi_check(furi_mutex_acquire(weave_audio_start_mutex, FuriWaitForever) == FuriStatusOk);
    if(!weave_alert_worker_running) {
        FuriThread* thread = furi_thread_alloc_ex("WeaveAlerts", 2048, weave_alert_worker, NULL);
        if(thread) {
            weave_alert_worker_running = true;
            furi_thread_start(thread);
        }
    }
    bool ready = weave_alert_worker_running;
    furi_check(furi_mutex_release(weave_audio_start_mutex) == FuriStatusOk);
    return ready;
}

static const char* weave_alert_file(WeaveAlertType type) {
    switch(type) {
    case WeaveAlertCall: return EXT_PATH(".weave/alert-call.cfg");
    case WeaveAlertSms: return EXT_PATH(".weave/alert-sms.cfg");
    case WeaveAlertFinder: return EXT_PATH(".weave/alert-finder.cfg");
    default: return EXT_PATH(".weave/alert-alarm.cfg");
    }
}

bool weave_ringtone_valid_filename(const char* name) {
    if(!name) return false;
    size_t n = strlen(name);
    if(n < 6 || n >= 40 || name[0] == '.') return false;
    for(size_t i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)name[i];
        if(!(isalnum(c) || c == '-' || c == '_' || c == '.')) return false;
    }
    const char* ext = strrchr(name, '.');
    return ext && (!strcmp(ext, ".tone") || !strcmp(ext, ".fmf") ||
                   !strcmp(ext, ".txt") || !strcmp(ext, ".rtttl"));
}

bool weave_alert_load(WeaveAlertType type, WeaveAlertSettings* settings) {
    if(!settings) return false;
    *settings = (WeaveAlertSettings){.enabled = true,
        .volume = type == WeaveAlertFinder ? 100 : 85, .vibro = true};
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    bool loaded = false;
    if(storage_file_open(file, weave_alert_file(type), FSAM_READ, FSOM_OPEN_EXISTING)) {
        char data[90] = {0};
        size_t n = storage_file_read(file, data, sizeof(data) - 1);
        data[n] = '\0';
        unsigned on = 1, volume = 85, vibro = 1;
        char name[40] = {0};
        if(sscanf(data, "%u %u %u %39s", &on, &volume, &vibro, name) == 4 &&
           on <= 1 && volume <= 100 && vibro <= 1 &&
           (!strcmp(name, "-") || !strcmp(name, "None") ||
            weave_ringtone_valid_filename(name))) {
            settings->enabled = on == 1;
            settings->volume = (uint8_t)volume;
            settings->vibro = vibro == 1;
            if(strcmp(name, "-")) strlcpy(settings->ringtone, name, sizeof(settings->ringtone));
            loaded = true;
        }
        storage_file_close(file);
    }
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return loaded;
}

bool weave_alert_save(WeaveAlertType type, const WeaveAlertSettings* settings) {
    if(!settings || type > WeaveAlertFinder || settings->volume > 100 ||
       (settings->ringtone[0] && strcmp(settings->ringtone,"None") &&
        !weave_ringtone_valid_filename(settings->ringtone))) return false;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    storage_simply_mkdir(storage, EXT_PATH(".weave"));
    char data[100];
    snprintf(data, sizeof(data), "%u %u %u %s\n", settings->enabled ? 1U : 0U,
        (unsigned)settings->volume, settings->vibro ? 1U : 0U,
        settings->ringtone[0] ? settings->ringtone : "-");
    File* file = storage_file_alloc(storage);
    bool ok = storage_file_open(file, weave_alert_file(type), FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) ok = storage_file_write(file, data, strlen(data)) == strlen(data);
    if(storage_file_is_open(file)) storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
    return ok;
}

bool weave_ringtone_has_selection(void) {
    WeaveAlertSettings settings;
    weave_alert_load(WeaveAlertAlarm, &settings);
    return settings.ringtone[0] != '\0' && strcmp(settings.ringtone,"None") != 0;
}

/* The buzzer is monophonic. Frequencies use a fixed semitone table instead
 * of floating-point pow(), keeping the parser small on STM32WB. */
static uint16_t weave_note_frequency(char pitch, bool sharp, int octave) {
    static const uint16_t base[] = {523, 554, 587, 622, 659, 698, 740, 784,
                                    831, 880, 932, 988};
    int index;
    switch((char)toupper((unsigned char)pitch)) {
    case 'C': index = 0; break;
    case 'D': index = 2; break;
    case 'E': index = 4; break;
    case 'F': index = 5; break;
    case 'G': index = 7; break;
    case 'A': index = 9; break;
    case 'B': case 'H': index = 11; break;
    default: return 0;
    }
    if(sharp) index = (index + 1) % 12;
    uint32_t value = base[index];
    if(octave < 1 || octave > 9) return 0;
    for(int i = 5; i < octave; ++i) value *= 2;
    for(int i = octave; i < 5; ++i) value = (value + 1) / 2;
    return value >= 160 && value <= 3500 ? (uint16_t)value : 0;
}

/* Parses the common note-token syntax of both RTTTL and FMF. */
static bool weave_parse_note(const char* token, unsigned duration, unsigned octave,
                             unsigned bpm, WeaveTone* out) {
    while(*token == ' ' || *token == '\t') ++token;
    char* end;
    unsigned number = (unsigned)strtoul(token, &end, 10);
    if(end != token) {duration = number; token = end;}
    if(!duration || duration > 128 || bpm < 20 || bpm > 900) return false;
    char pitch = *token++;
    if(!pitch || !(strchr("ABCDEFGHabcdefghPp", pitch))) return false;
    bool sharp = *token == '#';
    if(sharp) ++token;
    number = (unsigned)strtoul(token, &end, 10);
    if(end != token) {octave = number; token = end;}
    /* FMF also uses dots before the octave in some community files. */
    uint32_t milliseconds = (uint32_t)(240000UL / bpm) / duration;
    if(!milliseconds) milliseconds = 1;
    unsigned dots = 0;
    while(*token == '.' && dots++ < 4) {
        milliseconds += milliseconds / 2;
        ++token;
    }
    while(*token == ' ' || *token == '\r' || *token == '\t') ++token;
    if(*token && *token != '\n') return false;
    if(milliseconds > 5000) milliseconds = 5000;
    out->duration = (uint16_t)milliseconds;
    out->frequency = toupper((unsigned char)pitch) == 'P' ? 0 :
        weave_note_frequency(pitch, sharp, (int)octave);
    return out->frequency != 0 || toupper((unsigned char)pitch) == 'P';
}

static size_t weave_parse_sequence(char* notes_text, unsigned duration,
                                   unsigned octave, unsigned bpm, WeaveTone* out) {
    size_t count = 0;
    char* next = notes_text;
    while(next && *next && count < RING_MAX_NOTES) {
        char* token = next;
        next = strchr(token, ',');
        if(next) *next++ = '\0';
        if(weave_parse_note(token, duration, octave, bpm, &out[count])) ++count;
    }
    return count;
}

static size_t weave_parse_rtttl(char* text, WeaveTone* out) {
    char* colon = strchr(text, ':');
    if(!colon) return 0;
    *colon++ = '\0';
    char* notes = strchr(colon, ':');
    if(!notes) return 0;
    *notes++ = '\0';
    unsigned duration = 4, octave = 6, bpm = 63;
    char* item = colon;
    while(item && *item) {
        char* next = strchr(item, ',');
        if(next) *next++ = '\0';
        unsigned value;
        if(sscanf(item, " d = %u", &value) == 1) duration = value;
        else if(sscanf(item, " o = %u", &value) == 1) octave = value;
        else if(sscanf(item, " b = %u", &value) == 1) bpm = value;
        item = next;
    }
    return weave_parse_sequence(notes, duration, octave, bpm, out);
}

static size_t weave_parse_fmf(char* text, WeaveTone* out) {
    if(!strstr(text, "Flipper Music Format")) return 0;
    unsigned bpm = 120, duration = 4, octave = 5;
    char* notes = NULL;
    for(char* line = text; line && *line;) {
        char* next = strchr(line, '\n');
        if(next) *next++ = '\0';
        unsigned n;
        if(sscanf(line, "BPM: %u", &n) == 1) bpm = n;
        else if(sscanf(line, "Duration: %u", &n) == 1) duration = n;
        else if(sscanf(line, "Octave: %u", &n) == 1) octave = n;
        else if(strncmp(line, "Notes:", 6) == 0) {notes = line + 6; break;}
        line = next;
    }
    return notes ? weave_parse_sequence(notes, duration, octave, bpm, out) : 0;
}

static size_t weave_read_notes(const char* filename, WeaveTone* notes) {
    if(!weave_ringtone_valid_filename(filename)) return 0;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* path = furi_string_alloc_printf(RING_DIR "/%s", filename);
    File* file = storage_file_alloc(storage);
    size_t count = 0;
    char* data = malloc(RING_MAX_TEXT + 1);
    if(data && storage_file_open(file, furi_string_get_cstr(path), FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t length = storage_file_read(file, data, RING_MAX_TEXT);
        data[length] = '\0';
        const char* ext = strrchr(filename, '.');
        if(!strcmp(ext, ".tone")) {
            for(char* line = data; line && *line && count < RING_MAX_NOTES;) {
                char* next = strchr(line, '\n');
                if(next) *next++ = '\0';
                unsigned hz, ms;
                if(sscanf(line, "%u %u", &hz, &ms) == 2 &&
                   hz <= 3500 && ms > 0 && ms <= 5000 && (hz == 0 || hz >= 160)) {
                    notes[count++] = (WeaveTone){(uint16_t)hz, (uint16_t)ms};
                }
                line = next;
            }
        } else if(!strcmp(ext, ".fmf")) count = weave_parse_fmf(data, notes);
        else count = weave_parse_rtttl(data, notes);
        storage_file_close(file);
    }
    if(data) free(data);
    storage_file_free(file);
    furi_string_free(path);
    furi_record_close(RECORD_STORAGE);
    return count;
}

static void weave_ringtone_send(const WeaveRingCommand* command) {
    if(!ring_queue) return;
    if(!weave_ringtone_ensure_audio_worker()) return;
    furi_message_queue_reset(ring_queue);
    furi_message_queue_put(ring_queue, command, 0);
}

/* Send completion for one-shot phone alerts, not for persistent find/alarm alerts.
 * The ID prevents an old completion from dismissing a newer notification. */
static void weave_ringtone_complete(uint32_t id) {
    if(id && alert_audio_completed) furi_message_queue_put(alert_audio_completed, &id, 0);
}
static void weave_ringtone_apply_update(WeaveRingCommand* current, const WeaveRingCommand* next) {
    if(current->completion_id && current->completion_id != next->completion_id)
        weave_ringtone_complete(current->completion_id);
    *current = *next;
}

static void weave_ringtone_delete_preview(const char* name) {
    if(strncmp(name, "Preview_remote.", 15) != 0) return;
    if(!weave_ringtone_valid_filename(name)) return;
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FuriString* path = furi_string_alloc_printf(RING_DIR "/%s", name);
    storage_common_remove(storage, furi_string_get_cstr(path));
    furi_string_free(path);
    furi_record_close(RECORD_STORAGE);
}
static int32_t weave_ringtone_worker(void* context) {
    UNUSED(context);
    static const WeaveTone fallback[] = {
        {880, 220}, {659, 170}, {1046, 220}, {0, 100}, {880, 250},
    };
    WeaveTone* notes = malloc(sizeof(WeaveTone) * RING_MAX_NOTES);
    if(!notes) return 0;
    WeaveRingCommand command;
    for(;;) {
        if(furi_message_queue_get(ring_queue, &command, FuriWaitForever) != FuriStatusOk) continue;
        if(command.stop) continue;
        bool playing = true;
        uint32_t last_vibration = 0;
        uint32_t started_tick = furi_get_tick();
        while(playing) {
            size_t count = command.filename[0] ? weave_read_notes(command.filename, notes) : 0;
            const WeaveTone* sequence = count ? notes : fallback;
            if(!count) count = COUNT_OF(fallback);
            if(!furi_hal_speaker_acquire(100)) {
                /* A busy speaker must not silently end a Find alert. Retry,
                 * keep vibration alive and honour STOP while waiting. */
                if(command.vibro && (!last_vibration ||
                   (!command.vibro_once &&
                    (uint32_t)(furi_get_tick() - last_vibration) > furi_ms_to_ticks(1500)))) {
                    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
                    notification_message(notification, &sequence_single_vibro);
                    furi_record_close(RECORD_NOTIFICATION);
                    last_vibration = furi_get_tick();
                }
                WeaveRingCommand update;
                if(furi_message_queue_get(ring_queue, &update, 150) == FuriStatusOk) {
                    if(update.stop) playing = false;
                    else weave_ringtone_apply_update(&command, &update);
                }
                /* Give a one-shot alert time to acquire the speaker, but do not
                 * leave its LED and overlay stuck if the speaker never frees. */
                if(!command.repeat &&
                   (uint32_t)(furi_get_tick() - started_tick) > furi_ms_to_ticks(4000))
                    playing = false;
                continue;
            }
            bool switched = false;
            for(size_t i = 0; i < count && playing; ++i) {
                WeaveRingCommand update;
                if(furi_message_queue_get(ring_queue, &update, 0) == FuriStatusOk) {
                    if(update.stop) playing = false;
                    else {weave_ringtone_apply_update(&command, &update); switched = true;}
                    break;
                }
                if(command.vibro && (!last_vibration ||
                   (!command.vibro_once &&
                    (i == 0 || (uint32_t)(furi_get_tick() - last_vibration) >
                     furi_ms_to_ticks(4500))))) {
                    NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
                    notification_message(notification, &sequence_single_vibro);
                    furi_record_close(RECORD_NOTIFICATION);
                    last_vibration = furi_get_tick();
                }
                if(sequence[i].frequency && command.volume)
                    furi_hal_speaker_start(sequence[i].frequency, command.volume / 100.0f);
                /* Short slices keep OK/BACK and STOP responsive during long notes. */
                uint32_t remaining = sequence[i].duration;
                while(remaining && playing && !switched) {
                    uint32_t slice = remaining > 45 ? 45 : remaining;
                    furi_delay_ms(slice);
                    remaining -= slice;
                    if(furi_message_queue_get(ring_queue, &update, 0) == FuriStatusOk) {
                        if(update.stop) playing = false;
                        else {weave_ringtone_apply_update(&command, &update); switched = true;}
                    }
                }
                furi_hal_speaker_stop();
                if(i + 1 == count && !command.repeat && !switched) playing = false;
                if(switched) break;
            }
            furi_hal_speaker_stop();
            furi_hal_speaker_release();
            if(!command.repeat && !switched)
                weave_ringtone_delete_preview(command.filename);
            if(playing && !switched) {
                WeaveRingCommand update;
                if(furi_message_queue_get(ring_queue, &update, 100) == FuriStatusOk) {
                    if(update.stop) playing = false;
                    else weave_ringtone_apply_update(&command, &update);
                }
            }
        }
        weave_ringtone_complete(command.completion_id);
    }
    return 0;
}

static void weave_ringtone_start_profile_internal(WeaveAlertType type, uint32_t completion_id) {
    WeaveAlertSettings settings;
    weave_alert_load(type, &settings);
    if(type != WeaveAlertFinder && !settings.enabled) return;
    /* Calls/SMS/Alarm volumes and vibration control ONLY the Flipper and are
     * fully independent. They never change the connected phone's settings. */
    /* Only the alarm and Find my Flipper should keep repeating. A call or SMS
     * plays exactly one ringtone, with a single vibration. */
    bool one_shot = type == WeaveAlertCall || type == WeaveAlertSms;
    WeaveRingCommand command = {.repeat = !one_shot, .volume = settings.volume,
        .vibro = settings.vibro, .vibro_once = one_shot,
        .completion_id = one_shot ? completion_id : 0};
    strlcpy(command.filename, settings.ringtone, sizeof(command.filename));
    if(type == WeaveAlertFinder) {
        /* A lost Flipper must always remain audible and vibrate until the
         * physical OK/BACK key is pressed, regardless of General's setting. */
        command.volume = settings.volume < 85 ? 85 : settings.volume;
        command.vibro = true;
        if(!strcmp(command.filename, "None")) command.filename[0] = '\0';
    } else if(!strcmp(settings.ringtone, "None")) {
        command.filename[0] = '\0';
        command.volume = 0; /* Silence the buzzer; vibro remains independent. */
    }
    weave_ringtone_send(&command);
}
void weave_ringtone_start_profile(WeaveAlertType type) {
    weave_ringtone_start_profile_internal(type, 0);
}
void weave_ringtone_start(bool loud, uint32_t duration_ms) {
    UNUSED(duration_ms);
    if(loud) weave_ringtone_start_profile(WeaveAlertFinder);
    else weave_ringtone_start_profile(WeaveAlertAlarm);
}
void weave_ringtone_preview_default(void) {
    WeaveRingCommand command = {.volume = 75, .repeat = false, .vibro = false};
    weave_ringtone_send(&command);
}
void weave_ringtone_preview(const char* filename) {
    if(!weave_ringtone_valid_filename(filename)) return;
    WeaveRingCommand command = {.volume = 75, .repeat = false, .vibro = false};
    strlcpy(command.filename, filename, sizeof(command.filename));
    weave_ringtone_send(&command);
}
void weave_ringtone_stop(void) {
    const WeaveRingCommand command = {.stop = true};
    weave_ringtone_send(&command);
}

static void weave_alert_draw(Canvas* canvas, void* context) {
    const uint8_t type = *(uint8_t*)context;
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    const char* title = type == WeaveAlertCall ? "Incoming call" :
        type == WeaveAlertSms ? "New message" : "Find my Flipper";
    canvas_draw_str_aligned(canvas, 64, 16, AlignCenter, AlignCenter, title);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str_aligned(canvas, 64, 35, AlignCenter, AlignCenter, "Ringing / vibrating");
    canvas_draw_rframe(canvas, 13, 46, 102, 17, 4);
    canvas_draw_str_aligned(canvas, 64, 54, AlignCenter, AlignCenter, "OK / BACK  Stop");
}
static void weave_alert_input(InputEvent* event, void* context) {
    FuriMessageQueue* events = context;
    if(event->key == InputKeyOk || event->key == InputKeyBack)
        furi_message_queue_put(events, event, 0);
}
static int32_t weave_alert_worker(void* context) {
    UNUSED(context);
    uint8_t type;
    for(;;) {
        if(furi_message_queue_get(alert_requests, &type, FuriWaitForever) != FuriStatusOk) continue;
        FuriMessageQueue* events = furi_message_queue_alloc(8, sizeof(InputEvent));
        ViewPort* port = view_port_alloc();
        view_port_draw_callback_set(port, weave_alert_draw, &type);
        view_port_input_callback_set(port, weave_alert_input, events);
        Gui* gui = furi_record_open(RECORD_GUI);
        gui_add_view_port(gui, port, GuiLayerFullscreen);
        /* Keep the LED signalling for exactly as long as the local alert
         * is sounding. The phone's LEDs and audio are never modified here. */
        NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
        if(type == WeaveAlertSms)
            notification_message(notification, &sequence_blink_start_blue);
        else if(type == WeaveAlertCall)
            notification_message(notification, &sequence_blink_start_green);
        furi_record_close(RECORD_NOTIFICATION);
        /* The audio worker reports when a call/SMS melody finishes. The alert
         * overlay and LED then close automatically, without needing OK/BACK. */
        static uint32_t next_completion_id = 0;
        uint32_t completion_id = 0;
        if((type == WeaveAlertCall || type == WeaveAlertSms) && alert_audio_completed) {
            furi_message_queue_reset(alert_audio_completed);
            if(++next_completion_id == 0) ++next_completion_id;
            completion_id = next_completion_id;
        }
        weave_ringtone_start_profile_internal((WeaveAlertType)type, completion_id);
        InputEvent event;
        uint8_t stop_signal;
        uint32_t completed_id;
        for(;;) {
            /* Poll so an answer/hangup on Android closes the Flipper overlay
             * without requiring an additional button press. */
            if(furi_message_queue_get(events, &event, 100) == FuriStatusOk &&
               (event.type == InputTypeShort || event.type == InputTypeLong ||
                event.type == InputTypePress)) break;
            if(type == WeaveAlertCall && call_stop_requests &&
               furi_message_queue_get(call_stop_requests, &stop_signal, 0) == FuriStatusOk) break;
            if(completion_id && alert_audio_completed &&
               furi_message_queue_get(alert_audio_completed, &completed_id, 0) == FuriStatusOk &&
               completed_id == completion_id) break;
        }
        weave_ringtone_stop();
        if(type == WeaveAlertSms || type == WeaveAlertCall) {
            NotificationApp* notification = furi_record_open(RECORD_NOTIFICATION);
            notification_message(notification, &sequence_blink_stop);
            notification_message(notification, &sequence_reset_rgb);
            furi_record_close(RECORD_NOTIFICATION);
        }
        gui_remove_view_port(gui, port);
        view_port_free(port);
        furi_record_close(RECORD_GUI);
        furi_message_queue_free(events);
        /* Discard call termination signals only after their associated alert
         * has been dismissed, not before the phone can answer it. */
        if(type == WeaveAlertCall && call_stop_requests)
            while(furi_message_queue_get(call_stop_requests, &stop_signal, 0) == FuriStatusOk) {}
        while(furi_message_queue_get(alert_requests, &type, 0) == FuriStatusOk) {}
    }
    return 0;
}
void weave_ringtone_notify(WeaveAlertType type) {
    if(!alert_requests || type > WeaveAlertFinder) return;
    if(!weave_ringtone_ensure_alert_worker()) return;
    WeaveAlertSettings settings;
    weave_alert_load(type, &settings);
    if(type != WeaveAlertFinder && !settings.enabled) return;
    uint8_t value = (uint8_t)type;
    furi_message_queue_put(alert_requests, &value, 0);
}
void weave_ringtone_prepare_call(void) {
    if(call_stop_requests) furi_message_queue_reset(call_stop_requests);
}
void weave_ringtone_end_call(void) {
    if(!call_stop_requests) return;
    const uint8_t stop = 1;
    furi_message_queue_put(call_stop_requests, &stop, 0);
}
void weave_ringtone_find_flipper(void) {weave_ringtone_notify(WeaveAlertFinder);}
void weave_ringtone_init(void) {
    if(ring_queue) return;
    weave_audio_start_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!weave_audio_start_mutex) return;
    ring_queue = furi_message_queue_alloc(4, sizeof(WeaveRingCommand));
    if(!ring_queue) return;
    alert_requests = furi_message_queue_alloc(3, sizeof(uint8_t));
    call_stop_requests = furi_message_queue_alloc(3, sizeof(uint8_t));
    alert_audio_completed = furi_message_queue_alloc(4, sizeof(uint32_t));
    /* No ringtone or alert worker until the first actual request. */
}
