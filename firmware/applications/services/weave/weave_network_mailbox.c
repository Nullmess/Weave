#include "weave_internal.h"
/* Weave Network WN2 over official USB/BLE Storage RPC.
 * The companion explicitly opts in and renews a leased heartbeat.
 * Requests and replies are complete binary files. The response.ready marker
 * is committed only after the entire reply upload finishes. Firmware streams
 * reply data from SD in 512-byte callbacks: no huge microcontroller buffers.
 * One outstanding operation per Flipper; sockets live in the companion.
 */
#include "weave_i.h"
#include "weave_ringtone.h"
#include <storage/storage.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <flipper_application/flipper_application.h>
#include <loader/firmware_api/firmware_api.h>

#define WN_LINK EXT_PATH(".weave/net.link")
#define WN_REQUEST EXT_PATH(".weave/net.request")
#define WN_RESPONSE EXT_PATH(".weave/net.response")
#define WN_READY EXT_PATH(".weave/net.response.ready")
#define WN_EVENT EXT_PATH(".weave/net.event")
#define WN_EVENT_READY EXT_PATH(".weave/net.event.ready")
#define WN_EVENT_MAX 4096U
#define WN_LINK_LIFETIME_MS 30000U
#define WN_REQUEST_TIMEOUT_MS 1800000U
#define WN_BUSY_LEASE_MS 90000U
#define WN_POLL_MS 200U
#define WN_FAP_INDEX_REQ EXT_PATH(".weave/fap-index.req")
#define WN_FAP_INDEX EXT_PATH(".weave/fap-index.txt")
#define WN_FAP_INDEX_TMP EXT_PATH(".weave/fap-index.tmp")

/* BLE CLI, notifications and network requests share the Storage RPC polling
 * thread to avoid permanently reserving a second idle thread stack. */
extern void weave_ble_cli_poll(Storage* storage);

typedef struct {
    Weave* weave;
    uint32_t last_heartbeat;
    uint32_t last_lease_poll;
    uint32_t last_fap_poll;
    uint32_t last_sequence;
    uint32_t last_event_sequence;
    uint32_t dispatched_at;
    uint32_t request_id;
    bool heartbeat_seen;
    bool sd_initialized;
    uint64_t response_size_seen;
} WeaveNetworkWorker;

static size_t wn_read(Storage* storage, const char* path, uint8_t* dst, size_t cap) {
    File* file = storage_file_alloc(storage);
    size_t n = 0;
    if(storage_file_open(file, path, FSAM_READ, FSOM_OPEN_EXISTING)) {
        FileInfo info;
        if(storage_common_stat(storage, path, &info) == FSE_OK && info.size <= cap)
            n = storage_file_read(file, dst, cap);
        storage_file_close(file);
    }
    storage_file_free(file);
    return n;
}
static void wn_heartbeat(WeaveNetworkWorker* worker, Storage* storage, uint32_t now) {
    char data[40] = {0}, type[4] = {0}, end = 0;
    unsigned long sequence = 0;
    size_t n = wn_read(storage, WN_LINK, (uint8_t*)data, sizeof(data)-1);
    FileInfo info;
    if(worker->request_id &&
       storage_common_stat(storage, WN_RESPONSE, &info) == FSE_OK &&
       info.size != worker->response_size_seen) {
        worker->response_size_seen = info.size;
        worker->last_heartbeat = now; /* Active RPC upload renews the lease. */
    }
    if(n && sscanf(data, "WN1|%3[^|]|%lu%c", type, &sequence, &end) == 3 &&
       end == '\n' && sequence <= UINT32_MAX &&
       (!strcmp(type,"USB") || !strcmp(type,"BLE"))) {
        if(!worker->heartbeat_seen || worker->last_sequence != (uint32_t)sequence) {
            worker->last_sequence = (uint32_t)sequence;
            worker->last_heartbeat = now;
            worker->heartbeat_seen = true;
        }
        const uint32_t lease = worker->request_id ? WN_BUSY_LEASE_MS : WN_LINK_LIFETIME_MS;
        if((uint32_t)(now - worker->last_heartbeat) < furi_ms_to_ticks(lease)) {
            weave_gateway_attach(worker->weave,
                !strcmp(type,"USB") ? WeaveTransportUsb : WeaveTransportBle);
            return;
        }
    }
    weave_gateway_attach(worker->weave, WeaveTransportNone);
}
static void wn_error(WeaveNetworkWorker* w, WeaveStatus status) {
    WeaveResponse reply = {.id = w->request_id, .status = status};
    if(w->request_id) weave_gateway_reply(w->weave, &reply);
    w->request_id = 0;
}
static void wn_reply(WeaveNetworkWorker* worker, Storage* storage) {
    char ready[32] = {0};
    size_t marker = wn_read(storage, WN_READY, (uint8_t*)ready, sizeof(ready)-1);
    if(!marker) return;
    uint32_t marked_id = 0;
    char trailing = 0;
    if(sscanf(ready,"WN2|%" SCNu32 "%c",&marked_id,&trailing) != 2 || trailing != '\n') {
        storage_common_remove(storage, WN_READY);
        return;
    }
    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file, WN_RESPONSE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        storage_common_remove(storage, WN_READY);
        return;
    }
    char header[100];
    size_t pos = 0;
    uint8_t ch = 0;
    while(pos < sizeof(header)-1 && storage_file_read(file, &ch, 1) == 1) {
        if(ch == '\n') break;
        header[pos++] = (char)ch;
    }
    bool has_newline = ch == '\n' && pos < sizeof(header)-1;
    header[pos] = 0;
    uint32_t id = 0, length = 0;
    unsigned status = 0, code = 0;
    char extra = 0;
    FileInfo info;
    bool valid = has_newline &&
        sscanf(header,"WN2|%" SCNu32 "|%u|%u|%" SCNu32 "%c",
            &id,&status,&code,&length,&extra) == 4 &&
        id == marked_id && id > 0 && status <= WeaveStatusDenied && code <= UINT16_MAX &&
        length <= WEAVE_STREAM_MAX &&
        storage_common_stat(storage, WN_RESPONSE, &info) == FSE_OK &&
        info.size == (uint64_t)(pos + 1U) + length;
    if(!valid) {
        if(worker->request_id && marked_id == worker->request_id)
            wn_error(worker, WeaveStatusRemoteError);
        storage_file_close(file);
        storage_file_free(file);
        storage_common_remove(storage, WN_RESPONSE);
        storage_common_remove(storage, WN_READY);
        if(!worker->request_id) storage_common_remove(storage,WN_REQUEST);
        return;
    }
    bool matched = id == worker->request_id;
    if(matched) {
        uint8_t buffer[WEAVE_RESPONSE_MAX];
        uint32_t offset = 0;
        bool success = true;
        do {
            size_t take = length-offset;
            if(take > sizeof(buffer)) take = sizeof(buffer);
            if(take && storage_file_read(file, buffer, take) != take) {
                success = false;
                break;
            }
            WeaveResponse response = {
                .id=id,.status=(WeaveStatus)status,.http_status=(uint16_t)code,
                .data=take ? buffer : NULL,.size=take,
                .offset=offset,.total=length,.more=(offset+take<length),
            };
            offset += take;
            if(!weave_gateway_reply(worker->weave,&response)) break;
        } while(offset < length);
        if(!success) wn_error(worker,WeaveStatusRemoteError);
        else worker->request_id = 0;
    }
    storage_file_close(file);
    storage_file_free(file);
    storage_common_remove(storage, WN_RESPONSE);
    storage_common_remove(storage, WN_READY);
    /* The request may contain user-supplied headers or other sensitive data.
     * Do not retain the completed WN2 request file on microSD. */
    if(matched) storage_common_remove(storage,WN_REQUEST);
}
/* A companion may commit one unsolicited socket event at a time.
 * Its ready marker is written last; both files are consumed together.
 * Event data is delivered to FAP listeners in bounded chunks. */
static void wn_socket_event(WeaveNetworkWorker* worker, Storage* storage) {
    char ready[32] = {0};
    const size_t count = wn_read(storage,WN_EVENT_READY,(uint8_t*)ready,sizeof(ready)-1);
    if(!count) return;
    uint32_t sequence = 0;
    char tail = 0;
    if(sscanf(ready,"WE3|%" SCNu32 "%c",&sequence,&tail) != 2 ||
       !sequence || tail != '\n' || sequence == worker->last_event_sequence) {
        storage_common_remove(storage,WN_EVENT);
        storage_common_remove(storage,WN_EVENT_READY);
        return;
    }
    File* file = storage_file_alloc(storage);
    if(!storage_file_open(file,WN_EVENT,FSAM_READ,FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        storage_common_remove(storage,WN_EVENT_READY);
        return;
    }
    char header[96] = {0};
    size_t pos = 0;
    uint8_t ch = 0;
    while(pos < sizeof(header)-1 && storage_file_read(file,&ch,1)==1) {
        if(ch == '\n') break;
        header[pos++] = (char)ch;
    }
    header[pos] = 0;
    uint32_t event_id = 0, length = 0;
    unsigned session = 0;
    char kind[4] = {0}, extra = 0;
    FileInfo info;
    bool valid = ch == '\n' &&
        sscanf(header,"WE3|%" SCNu32 "|%3[^|]|%u|%" SCNu32 "%c",
            &event_id,kind,&session,&length,&extra)==4 &&
        event_id == sequence && session > 0 && session <= UINT16_MAX &&
        length > 0 && length <= WN_EVENT_MAX &&
        storage_common_stat(storage,WN_EVENT,&info)==FSE_OK &&
        info.size == (uint64_t)(pos+1U)+length;
    WeaveSocketType type = WeaveSocketTcp;
    if(valid) {
        if(!strcmp(kind,"TCP")) type = WeaveSocketTcp;
        else if(!strcmp(kind,"UDP")) type = WeaveSocketUdp;
        else if(!strcmp(kind,"WS")) type = WeaveSocketWebSocket;
        else valid = false;
    }
    if(valid) {
        worker->last_event_sequence = sequence;
        uint8_t bytes[WEAVE_RESPONSE_MAX];
        uint32_t offset=0;
        while(offset < length) {
            size_t take = length-offset;
            if(take > sizeof(bytes)) take=sizeof(bytes);
            if(storage_file_read(file,bytes,take)!=take) break;
            const WeaveSocketEvent event = {
                .type=type,.session=(uint16_t)session,.data=bytes,.size=take,
                .offset=offset,.total=length,.more=(offset+take<length),
            };
            weave_gateway_socket_event(worker->weave,&event);
            offset += take;
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    storage_common_remove(storage,WN_EVENT);
    storage_common_remove(storage,WN_EVENT_READY);
}
/* Appends a request body from SD without mapping it into SRAM. */
static bool wn_send_file(Storage* storage, File* output, const char* path, uint32_t size) {
    File* input = storage_file_alloc(storage);
    bool ok = storage_file_open(input,path,FSAM_READ,FSOM_OPEN_EXISTING);
    uint8_t buffer[WEAVE_RESPONSE_MAX];
    uint32_t offset = 0;
    while(ok && offset < size) {
        size_t chunk = size-offset;
        if(chunk > sizeof(buffer)) chunk = sizeof(buffer);
        if(storage_file_read(input,buffer,chunk) != chunk ||
           storage_file_write(output,buffer,chunk) != chunk) ok = false;
        else offset += chunk;
    }
    if(storage_file_is_open(input)) storage_file_close(input);
    storage_file_free(input);
    return ok;
}
static bool wn_send_request(Storage* storage, const WeaveRequest* r) {
    FileInfo info;
    uint32_t body_length = r->body_size;
    if(r->file_path[0]) {
        if(storage_common_stat(storage,r->file_path,&info) != FSE_OK ||
           info.size > WEAVE_STREAM_MAX) return false;
        if(info.size > WEAVE_STREAM_MAX - body_length) return false;
        body_length += (uint32_t)info.size;
    }
    char header[WEAVE_URL_MAX + 72];
    int n = snprintf(header,sizeof(header),"WN2|%" PRIu32 "|%s|%s|%" PRIu32 "\n",
        r->id,r->method,r->target,body_length);
    if(n <= 0 || n >= (int)sizeof(header)) return false;
    File* output = storage_file_alloc(storage);
    bool ok = storage_file_open(output,WN_REQUEST,FSAM_WRITE,FSOM_CREATE_ALWAYS);
    if(ok) {
        ok = storage_file_write(output,header,(size_t)n) == (size_t)n;
        if(ok && r->body_size)
            ok = storage_file_write(output,r->body,r->body_size) == r->body_size;
        if(ok && r->file_path[0])
            ok = wn_send_file(storage,output,r->file_path,body_length - r->body_size);
        storage_file_close(output);
    }
    storage_file_free(output);
    if(!ok) storage_common_remove(storage,WN_REQUEST);
    return ok;
}
static void wn_send(WeaveNetworkWorker* worker, Storage* storage, uint32_t now) {
    WeaveRequest request;
    if(!weave_gateway_poll(worker->weave,&request)) return;
    worker->request_id = request.id;
    worker->response_size_seen = 0;
    worker->dispatched_at = now;
    if(!wn_send_request(storage,&request)) wn_error(worker,WeaveStatusRemoteError);
    else worker->last_heartbeat = furi_get_tick();
}
static void wn_expire(WeaveNetworkWorker* worker, Storage* storage, uint32_t now) {
    if(worker->request_id &&
        (uint32_t)(now-worker->dispatched_at) > furi_ms_to_ticks(WN_REQUEST_TIMEOUT_MS)) {
        wn_error(worker,WeaveStatusTimeout);
        storage_common_remove(storage,WN_REQUEST);
        storage_common_remove(storage,WN_RESPONSE);
        storage_common_remove(storage,WN_READY);
    }
}
/* L'index FAP est généré à la demande, jamais à chaque heartbeat. */
static bool wn_index_faps(Storage* storage, const char* token) {
    static const char* dirs[] = {
        "USB", "GPIO", "Tools", "Bluetooth", "NFC", "RFID", "Sub-GHz",
        "Infrared", "iButton", "Games", "Other", "Others", "Media",
    };
    File* output = storage_file_alloc(storage);
    if(!storage_file_open(output, WN_FAP_INDEX_TMP, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_free(output);
        return false;
    }
    bool ok = token && strlen(token) > 0 && strlen(token) <= 18;
    if(ok) {
        char header[32];
        int n = snprintf(header, sizeof(header), "#%s\n", token);
        ok = n > 0 && n < (int)sizeof(header) &&
            storage_file_write(output, header, n) == (size_t)n;
    }
    for(size_t i = 0; i < COUNT_OF(dirs) && ok; i++) {
        char folder[96];
        snprintf(folder, sizeof(folder), "/ext/apps/%s", dirs[i]);
        File* dir = storage_file_alloc(storage);
        if(storage_dir_open(dir, folder)) {
            FileInfo item;
            char filename[96];
            while(ok && storage_dir_read(dir, &item, filename, sizeof(filename))) {
                size_t length = strlen(filename);
                if(file_info_is_dir(&item) || length < 5 ||
                   strcmp(filename + length - 4, ".fap")) continue;
                char path[224];
                if(snprintf(path, sizeof(path), "%s/%s", folder, filename) >= (int)sizeof(path)) continue;
                FlipperApplication* fap = flipper_application_alloc(storage, firmware_api_interface);
                const FlipperApplicationManifest* manifest = NULL;
                if(fap && flipper_application_preload_manifest(fap, path) ==
                              FlipperApplicationPreloadStatusSuccess) {
                    manifest = flipper_application_get_manifest(fap);
                }
                char title[33] = {0};
                uint32_t version = 0;
                char icon[65] = "-";
                if(manifest) {
                    memcpy(title, manifest->name, 32);
                    title[32] = 0;
                    for(size_t j = 0; title[j]; j++)
                        if(title[j] == '|' || title[j] == '\n' || title[j] == '\r') title[j] = ' ';
                    version = manifest->app_version;
                    if(manifest->has_icon) {
                        for(size_t j = 0; j < 32; j++)
                            snprintf(icon + j * 2, 3, "%02x", (uint8_t)manifest->icon[j]);
                    }
                }
                if(!title[0]) {
                    size_t n = length - 4;
                    if(n > 32) n = 32;
                    memcpy(title, filename, n);
                    title[n] = 0;
                }
                char line[400];
                int n = snprintf(line, sizeof(line), "%s|%s|%" PRIu64 "|%" PRIu32 "|%s\n",
                    path, title, (uint64_t)item.size, version, icon);
                if(n <= 0 || n >= (int)sizeof(line) ||
                   storage_file_write(output, line, n) != (size_t)n) ok = false;
                if(fap) flipper_application_free(fap);
            }
            storage_dir_close(dir);
        }
        storage_file_free(dir);
    }
    storage_file_close(output);
    storage_file_free(output);
    if(ok) {
        storage_common_remove(storage, WN_FAP_INDEX);
        ok = storage_common_rename(storage, WN_FAP_INDEX_TMP, WN_FAP_INDEX) == FSE_OK;
    }
    if(!ok) storage_common_remove(storage, WN_FAP_INDEX_TMP);
    return ok;
}

static int32_t wn_thread(void* context) {
    WeaveNetworkWorker worker = {.weave=context};
    Storage* storage = furi_record_open(RECORD_STORAGE);
    while(true) {
        uint32_t now = furi_get_tick();
        if(storage_sd_status(storage) == FSE_OK) {
            if(!worker.sd_initialized) {
                storage_simply_mkdir(storage,EXT_PATH(".weave"));
                storage_common_remove(storage,WN_REQUEST);
                storage_common_remove(storage,WN_RESPONSE);
                storage_common_remove(storage,WN_READY);
                storage_common_remove(storage,WN_EVENT);
                storage_common_remove(storage,WN_EVENT_READY);
                storage_common_remove(storage,WN_LINK);
                worker.sd_initialized = true;
            }
            // Limite les I/O SD du lease hors trafic pour fluidifier GUI/RPC.
            if((uint32_t)(now-worker.last_lease_poll) >= furi_ms_to_ticks(800U)) {
                worker.last_lease_poll = now;
                wn_heartbeat(&worker,storage,now);
            }
            /* Index sollicité par Android ; throttle SD en dehors des requêtes réseau. */
            if((uint32_t)(now - worker.last_fap_poll) >= furi_ms_to_ticks(5000U)) {
                worker.last_fap_poll = now;
                FileInfo index_request;
                if(storage_common_stat(storage, WN_FAP_INDEX_REQ, &index_request) == FSE_OK) {
                    char index_token[24] = {0};
                    size_t n = wn_read(storage, WN_FAP_INDEX_REQ,
                        (uint8_t*)index_token, sizeof(index_token) - 1);
                    storage_common_remove(storage, WN_FAP_INDEX_REQ);
                    if(n > 1 && index_token[n - 1] == '\n') {
                        index_token[n - 1] = 0;
                        wn_index_faps(storage, index_token);
                    }
                }
            }
            /* A transport change aborts the operation. Do not keep the previous
             * mailbox lease alive and block the new FIFO head. */
            if(worker.request_id && !weave_gateway_active(worker.weave,worker.request_id)) {
                storage_common_remove(storage,WN_REQUEST);
                storage_common_remove(storage,WN_RESPONSE);
                storage_common_remove(storage,WN_READY);
                worker.request_id = 0;
            }
            if(weave_get_transport(worker.weave) != WeaveTransportNone)
                wn_socket_event(&worker,storage);
            else {
                storage_common_remove(storage,WN_EVENT);
                storage_common_remove(storage,WN_EVENT_READY);
            }
            if(worker.request_id) wn_reply(&worker,storage);
            wn_expire(&worker,storage,now);
            if(weave_get_transport(worker.weave) != WeaveTransportNone) {
                if(!worker.request_id) wn_send(&worker,storage,now);
            }
            else if(worker.request_id) {
                storage_common_remove(storage,WN_REQUEST);
                storage_common_remove(storage,WN_READY);
                worker.request_id = 0;
            }
        } else {
            worker.sd_initialized = false;
            worker.heartbeat_seen = false;
            worker.request_id = 0;
            weave_gateway_attach(worker.weave,WeaveTransportNone);
        }
        weave_ble_cli_poll(storage);
        weave_ringtone_reap_idle();
        weave_usb_reap_orphan();
        weave_ble_reap_orphan();
        furi_delay_ms(WN_POLL_MS);
    }
    return 0;
}
void weave_network_mailbox_init(Weave* weave) {
    /* Permanent worker: use the platform's SRAM2 service pool when available,
     * falling back to the ordinary heap. Do not shrink the stack blindly. */
    const size_t heap_before = memmgr_get_free_heap();
    const size_t pool_before = memmgr_pool_get_free();
    FuriThread* thread = furi_thread_alloc_service("WeaveNetwork",6144,wn_thread,weave);
    FURI_LOG_I("WeaveNetwork", "Worker heap %zu -> %zu, pool %zu -> %zu",
        heap_before, memmgr_get_free_heap(), pool_before, memmgr_pool_get_free());
    furi_thread_start(thread);
}
