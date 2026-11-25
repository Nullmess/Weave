/* Weave BLE CLI: the existing Flipper CLI shell on a second pipe.
 * Commands and output are carried through the already authenticated
 * protobuf Storage RPC channel. No extra BLE service or pairing mode.
 * Only enabled on demand (the Android terminal writes the inbox file).
 */
#include <furi.h>
#include <furi/core/memmgr.h>
#include <storage/storage.h>
#include <toolbox/pipe.h>
#include <toolbox/cli/shell/cli_shell.h>
#include <cli/cli.h>
#include <cli/cli_main_shell.h>
#include <stdio.h>
#include "weave_ringtone.h"
#include <notification/notification_app.h>
#include <notification/notification_messages.h>
#include <string.h>

#define WEAVE_FIND_FLIPPER EXT_PATH(".weave/find-flipper.req")
#define WEAVE_ALERT_REQUEST EXT_PATH(".weave/alert.req")
#define WEAVE_RING_PREVIEW EXT_PATH(".weave/ringtone-preview.req")
#define WEAVE_CLI_IN  EXT_PATH(".weave/cli.in")
#define WEAVE_CLI_OUT EXT_PATH(".weave/cli.out")
#define WEAVE_CLI_IDLE_TICKS (furi_ms_to_ticks(120000U))
#define WEAVE_CLI_MAX_OUT 8192U

typedef struct {
    PipeSide* remote;
    PipeSide* shell_pipe;
    CliShell* shell;
    uint32_t last_activity;
} WeaveBleCli;

static void weave_ble_cli_stop(WeaveBleCli* state) {
    if(!state->shell) return;
    // Closing the remote end makes the shell's blocking read terminate.
    pipe_free(state->remote);
    state->remote = NULL;
    cli_shell_join(state->shell);
    cli_shell_free(state->shell);
    pipe_free(state->shell_pipe);
    state->shell = NULL;
    state->shell_pipe = NULL;
}

static bool weave_ble_cli_start(WeaveBleCli* state, Storage* storage) {
    if(state->shell) return true;
    // Do not spawn a shell if the SD card is missing or memory is constrained.
    if(storage_sd_status(storage) != FSE_OK || memmgr_get_free_heap() < 18000) return false;
    PipeSideBundle pair = pipe_alloc(2048, 1);
    if(!pair.alices_side || !pair.bobs_side) return false;
    state->remote = pair.alices_side;
    state->shell_pipe = pair.bobs_side;
    CliRegistry* registry = furi_record_open(RECORD_CLI);
    state->shell = cli_shell_alloc(cli_main_motd, NULL, state->shell_pipe, registry, NULL);
    furi_record_close(RECORD_CLI);
    if(!state->shell) {
        pipe_free(state->remote);
        pipe_free(state->shell_pipe);
        state->remote = state->shell_pipe = NULL;
        return false;
    }
    File* output = storage_file_alloc(storage);
    if(storage_file_open(output, WEAVE_CLI_OUT, FSAM_WRITE, FSOM_CREATE_ALWAYS))
        storage_file_close(output);
    storage_file_free(output);
    cli_shell_start(state->shell);
    state->last_activity = furi_get_tick();
    return true;
}

static void weave_ble_cli_input(WeaveBleCli* state, Storage* storage) {
    File* f = storage_file_alloc(storage);
    if(!storage_file_open(f, WEAVE_CLI_IN, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(f);
        return;
    }
    char line[192];
    uint16_t count = storage_file_read(f, line, sizeof(line)-2);
    storage_file_close(f);
    storage_file_free(f);
    // Remove before executing: a restart never executes the command twice.
    storage_common_remove(storage, WEAVE_CLI_IN);
    if(!count || count > sizeof(line)-2) return;
    if(!weave_ble_cli_start(state, storage)) {
        File* error_file = storage_file_alloc(storage);
        if(storage_file_open(error_file, WEAVE_CLI_OUT, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
            const char* message = "BLE CLI unavailable (SD card or free memory).\r\n";
            storage_file_write(error_file, message, strlen(message));
            storage_file_close(error_file);
        }
        storage_file_free(error_file);
        return;
    }
    // One complete line per RPC request; no arbitrary binary/control input.
    size_t n = 0;
    for(uint16_t i = 0; i < count; i++) {
        unsigned char c = (unsigned char)line[i];
        if(c == '\n' || c == '\r') break;
        if(c < 32 || c > 126) return;
        line[n++] = c;
    }
    if(!n) return;
    line[n++] = '\r';
    line[n++] = '\n';
    // Pipe has enough space for bounded input; never panic on a full pipe.
    size_t written = pipe_send(state->remote, (const uint8_t*)line, n);
    if(written == n) state->last_activity = furi_get_tick();
}

static void weave_ble_cli_output(WeaveBleCli* state, Storage* storage) {
    if(!state->shell) return;
    size_t available = pipe_bytes_available(state->remote);
    if(!available) return;
    File* f = storage_file_alloc(storage);
    FileInfo info;
    bool reset = storage_common_stat(storage, WEAVE_CLI_OUT, &info) == FSE_OK &&
                 info.size >= WEAVE_CLI_MAX_OUT;
    if(!storage_file_open(f, WEAVE_CLI_OUT, FSAM_WRITE,
                          reset ? FSOM_CREATE_ALWAYS : FSOM_OPEN_APPEND)) {
        storage_file_free(f);
        return;
    }
    uint8_t data[256];
    while(available) {
        size_t size = pipe_receive(state->remote, data, MIN(sizeof(data), available));
        if(!size) break;
        if(storage_file_write(f, data, size) != size) break;
        available = pipe_bytes_available(state->remote);
        state->last_activity = furi_get_tick();
    }
    storage_file_close(f);
    storage_file_free(f);
}

/* The companion creates this command through authenticated BLE storage RPC.
 * Consume the one-shot marker before ringing so restarts cannot replay it.
 * The notification service mixes speaker/vibration without changing settings. */
static bool weave_find_flipper_check(Storage* storage) {
    File* file = storage_file_alloc(storage);
    bool found = storage_file_open(file, WEAVE_FIND_FLIPPER, FSAM_READ, FSOM_OPEN_EXISTING);
    char marker[48];
    uint16_t length = found ? storage_file_read(file, marker, sizeof(marker)) : 0;
    if(found) storage_file_close(file);
    storage_file_free(file);
    if(!found) return false;
    storage_common_remove(storage, WEAVE_FIND_FLIPPER);
    if(length < 4 || length >= sizeof(marker)) return false;
    for(uint16_t i = 0; i < length; ++i)
        if(marker[i] < '0' || marker[i] > '9') return false;
    return true;
}

/* One-shot, authenticated RPC-storage event from the Android notification
 * listener. Nothing about the caller or SMS content is transferred. */
static char weave_active_call_token[32];
static void weave_alert_check(Storage* storage) {
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, WEAVE_ALERT_REQUEST, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char text[48] = {0};
        size_t n = storage_file_read(file, text, sizeof(text) - 1);
        text[n] = '\0';
        storage_file_close(file);
        storage_common_remove(storage, WEAVE_ALERT_REQUEST);
        if(n > 3 && text[1] == '|' && strspn(text + 2, "0123456789") == n - 2) {
            if(text[0] == 'C' && strlen(text + 2) < sizeof(weave_active_call_token)) {
                /* Only the matching call's E| token can silence this call.
                 * Clear stale stop commands before a new incoming call. */
                weave_ringtone_prepare_call();
                strlcpy(weave_active_call_token,text + 2,sizeof(weave_active_call_token));
                weave_ringtone_notify(WeaveAlertCall);
            } else if(text[0] == 'E') {
                if(weave_active_call_token[0] &&
                   !strcmp(weave_active_call_token,text + 2)) {
                    weave_active_call_token[0] = '\0';
                    weave_ringtone_end_call();
                }
            } else if(text[0] == 'S') weave_ringtone_notify(WeaveAlertSms);
        }
    }
    storage_file_free(file);
}
static void weave_preview_check(Storage* storage) {
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, WEAVE_RING_PREVIEW, FSAM_READ, FSOM_OPEN_EXISTING)) {
        char name[40] = {0};
        size_t n = storage_file_read(file,name,sizeof(name)-1);
        storage_file_close(file);
        storage_common_remove(storage, WEAVE_RING_PREVIEW);
        if(n && n < sizeof(name)) {
            name[n] = '\0';
            if(weave_ringtone_valid_filename(name)) weave_ringtone_preview(name);
        }
    }
    storage_file_free(file);
}

/* Called from the existing WeaveNetwork worker. There is no need to reserve
 * another 2 KiB RTOS thread stack merely to poll the same Storage RPC files.
 * The interactive CLI shell itself is still created only on demand. */
void weave_ble_cli_poll(Storage* storage) {
    static WeaveBleCli state;
    static bool directory_ready;
    if(storage_sd_status(storage) == FSE_OK) {
        if(!directory_ready)
            directory_ready = storage_simply_mkdir(storage, EXT_PATH(".weave"));
        if(directory_ready) {
            if(weave_find_flipper_check(storage))
                weave_ringtone_find_flipper();
            weave_alert_check(storage);
            weave_preview_check(storage);
            weave_ble_cli_input(&state, storage);
            weave_ble_cli_output(&state, storage);
        }
    } else directory_ready = false;
    if(state.shell &&
       (uint32_t)(furi_get_tick() - state.last_activity) > WEAVE_CLI_IDLE_TICKS)
        weave_ble_cli_stop(&state);
}
