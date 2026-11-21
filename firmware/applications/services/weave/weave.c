#include "weave_i.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <strings.h>
#include <stdlib.h>

static void weave_cleanup_reply(const WeaveResponse* response, void* context) {
    (void)response;
    (void)context;
}

/* Queues a best-effort remote CLOSE for sockets owned by an unloaded FAP.
 * Call only while holding the Weave mutex. The worker never runs FAP code
 * for these cleanup requests. */
static void weave_flush_cleanup_locked(Weave* weave) {
    while(weave->closing_count && weave->queue_count < WEAVE_QUEUE_MAX &&
          weave->transport != WeaveTransportNone) {
        const WeaveTrackedSession session = weave->closing_sessions[0];
        for(size_t i = 1; i < weave->closing_count; i++)
            weave->closing_sessions[i-1] = weave->closing_sessions[i];
        weave->closing_count--;
        memset(&weave->closing_sessions[weave->closing_count],0,sizeof(WeaveTrackedSession));
        WeaveOperation* op = &weave->queue[weave->queue_count++];
        memset(op,0,sizeof(*op));
        if(++weave->next_id == 0) ++weave->next_id;
        op->request.id = weave->next_id;
        const char* method = session.type == WeaveSocketTcp ? "TCP_CLOSE" :
                             session.type == WeaveSocketUdp ? "UDP_CLOSE" : "WS_CLOSE";
        strcpy(op->request.method,method);
        snprintf(op->request.target,sizeof(op->request.target),"%u",session.id);
        op->callback = weave_cleanup_reply;
    }
}

/* Release may race with a callback already selected for invocation.
 * Accounting happens under weave->mutex before the callback is dispatched. */
void weave_callback_started_locked(Weave* weave, void* context) {
    WeaveActiveCallback* free_slot = NULL;
    for(size_t i = 0; i < COUNT_OF(weave->callbacks_by_context); i++) {
        WeaveActiveCallback* slot = &weave->callbacks_by_context[i];
        if(slot->count && slot->context == context) {
            furi_check(slot->count != UINT16_MAX);
            slot->count++;
            weave->callbacks_active++;
            return;
        }
        if(!slot->count && !free_slot) free_slot = slot;
    }
    furi_check(free_slot);
    free_slot->context = context;
    free_slot->count = 1;
    weave->callbacks_active++;
}
void weave_callback_finished(Weave* weave, void* context) {
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    furi_check(weave->callbacks_active != 0);
    bool found = false;
    for(size_t i = 0; i < COUNT_OF(weave->callbacks_by_context); i++) {
        WeaveActiveCallback* slot = &weave->callbacks_by_context[i];
        if(slot->count && slot->context == context) {
            if(--slot->count == 0) slot->context = NULL;
            weave->callbacks_active--;
            found = true;
            break;
        }
    }
    furi_check(found);
    furi_mutex_release(weave->mutex);
}

bool weave_context_release(Weave* weave, void* context, uint32_t timeout_ms) {
    if(!weave || !context) return false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    size_t kept = 0;
    for(size_t i = 0; i < weave->queue_count; i++) {
        if(weave->queue[i].context != context) {
            if(kept != i) weave->queue[kept] = weave->queue[i];
            kept++;
        }
    }
    for(size_t i = kept; i < WEAVE_QUEUE_MAX; i++)
        memset(&weave->queue[i], 0, sizeof(WeaveOperation));
    weave->queue_count = kept;
    for(size_t i = 0; i < COUNT_OF(weave->sessions); i++) {
        if(weave->sessions[i].id && weave->sessions[i].owner == context) {
            if(weave->closing_count < COUNT_OF(weave->closing_sessions))
                weave->closing_sessions[weave->closing_count++] = weave->sessions[i];
            weave->sessions[i] = (WeaveTrackedSession){0};
        }
    }
    weave_flush_cleanup_locked(weave);
    for(size_t i = 0; i < COUNT_OF(weave->listeners); i++)
        if(weave->listeners[i].context == context)
            weave->listeners[i] = (WeaveTransportListener){0};
    for(size_t i = 0; i < COUNT_OF(weave->socket_listeners); i++)
        if(weave->socket_listeners[i].context == context)
            weave->socket_listeners[i] = (WeaveSocketEventListener){0};
    furi_mutex_release(weave->mutex);

    const uint32_t start = furi_get_tick();
    for(;;) {
        furi_mutex_acquire(weave->mutex, FuriWaitForever);
        bool idle = true;
        for(size_t i = 0; i < COUNT_OF(weave->callbacks_by_context); i++) {
            const WeaveActiveCallback* slot = &weave->callbacks_by_context[i];
            if(slot->count && slot->context == context) { idle = false; break; }
        }
        furi_mutex_release(weave->mutex);
        if(idle) return true;
        if((uint32_t)(furi_get_tick() - start) >= furi_ms_to_ticks(timeout_ms))
            return false;
        furi_delay_ms(1);
    }
}

static bool safe_field(const char* value, size_t max) {
    if(!value) return false;
    size_t n = strnlen(value, max);
    if(!n || n >= max) return false;
    for(size_t i = 0; i < n; ++i)
        if((unsigned char)value[i] <= 0x20 || (unsigned char)value[i] >= 0x7f || value[i] == '|') return false;
    return true;
}
static bool http_url(const char* url) {
    return safe_field(url, WEAVE_URL_MAX) &&
        ((!strncmp(url, "https://", 8) && strlen(url) > 8) ||
         (!strncmp(url, "http://", 7) && strlen(url) > 7));
}
static bool http_method(const char* method) {
    return method && (!strcmp(method, "GET") || !strcmp(method, "POST") ||
        !strcmp(method, "PUT") || !strcmp(method, "PATCH") ||
        !strcmp(method, "DELETE") || !strcmp(method, "HEAD"));
}
/* Remove the completed request and advance the bounded FIFO. */
void weave_reset(Weave* weave) {
    if(!weave->queue_count) return;
    for(size_t i = 1; i < weave->queue_count; i++)
        weave->queue[i - 1] = weave->queue[i];
    weave->queue_count--;
    memset(&weave->queue[weave->queue_count], 0, sizeof(WeaveOperation));
    weave_flush_cleanup_locked(weave);
}
bool weave_transport_subscribe(Weave* weave, WeaveTransportCallback cb, void* ctx) {
    if(!weave || !cb) return false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    for(size_t i = 0; i < COUNT_OF(weave->listeners); i++) {
        if(weave->listeners[i].callback == cb && weave->listeners[i].context == ctx) {
            furi_mutex_release(weave->mutex);
            return true;
        }
    }
    for(size_t i = 0; i < COUNT_OF(weave->listeners); i++) {
        if(!weave->listeners[i].callback) {
            weave->listeners[i] = (WeaveTransportListener){cb, ctx};
            furi_mutex_release(weave->mutex);
            return true;
        }
    }
    furi_mutex_release(weave->mutex);
    return false;
}
void weave_transport_unsubscribe(Weave* weave, WeaveTransportCallback cb, void* ctx) {
    if(!weave || !cb) return;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    for(size_t i = 0; i < COUNT_OF(weave->listeners); i++) {
        if(weave->listeners[i].callback == cb && weave->listeners[i].context == ctx)
            weave->listeners[i] = (WeaveTransportListener){0};
    }
    furi_mutex_release(weave->mutex);
}
WeaveTransport weave_get_transport(Weave* weave) {
    furi_check(weave);
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    WeaveTransport transport = weave->transport;
    furi_mutex_release(weave->mutex);
    return transport;
}
bool weave_socket_subscribe_events(Weave* weave, WeaveSocketEventCallback cb, void* ctx) {
    if(!weave || !cb) return false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    for(size_t i = 0; i < COUNT_OF(weave->socket_listeners); i++) {
        if(weave->socket_listeners[i].callback == cb &&
           weave->socket_listeners[i].context == ctx) {
            furi_mutex_release(weave->mutex);
            return true;
        }
    }
    for(size_t i = 0; i < COUNT_OF(weave->socket_listeners); i++) {
        if(!weave->socket_listeners[i].callback) {
            weave->socket_listeners[i] = (WeaveSocketEventListener){cb,ctx};
            furi_mutex_release(weave->mutex);
            return true;
        }
    }
    furi_mutex_release(weave->mutex);
    return false;
}
void weave_socket_unsubscribe_events(Weave* weave, WeaveSocketEventCallback cb, void* ctx) {
    if(!weave || !cb) return;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    for(size_t i = 0; i < COUNT_OF(weave->socket_listeners); i++)
        if(weave->socket_listeners[i].callback == cb &&
           weave->socket_listeners[i].context == ctx)
            weave->socket_listeners[i] = (WeaveSocketEventListener){0};
    furi_mutex_release(weave->mutex);
}
static WeaveStatus submit(Weave* weave, const char* verb, const char* target,
    const uint8_t* data, size_t size, const char* file_path, WeaveCallback cb, void* ctx) {
    if(!weave || !cb || !safe_field(verb, sizeof(((WeaveRequest*)0)->method)) ||
       !safe_field(target, WEAVE_URL_MAX) || size > WEAVE_RESPONSE_MAX || (size && !data))
        return WeaveStatusInvalid;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    WeaveStatus status = WeaveStatusOk;
    if(weave->transport == WeaveTransportNone) status = WeaveStatusOffline;
    else if(weave->queue_count >= WEAVE_QUEUE_MAX) status = WeaveStatusBusy;
    else {
        /* Socket operations are scoped to their creator's callback context.
         * There is no separate-process memory protection on Flipper. */
        const bool is_socket = !strncmp(verb, "TCP_", 4) ||
                               !strncmp(verb, "UDP_", 4) ||
                               !strncmp(verb, "WS_", 3);
        if(is_socket && !strstr(verb,"_OPEN")) {
            const unsigned long id = strtoul(target, NULL, 10);
            bool owned = false;
            for(size_t i = 0; i < COUNT_OF(weave->sessions); i++) {
                const WeaveTrackedSession* session = &weave->sessions[i];
                const char* kind = session->type == WeaveSocketTcp ? "TCP_" :
                                   session->type == WeaveSocketUdp ? "UDP_" : "WS_";
                if(session->id == id && !strncmp(verb, kind, strlen(kind)) &&
                   session->owner == ctx) { owned = true; break; }
            }
            if(!owned) status = WeaveStatusDenied;
        }
        if(status == WeaveStatusOk) {
            if(++weave->next_id == 0) ++weave->next_id;
            WeaveOperation* op = &weave->queue[weave->queue_count++];
            memset(op,0,sizeof(*op));
            op->request.id = weave->next_id;
            strcpy(op->request.method, verb);
            strcpy(op->request.target, target);
            if(size) memcpy(op->request.body, data, size);
            op->request.body_size = (uint16_t)size;
            if(file_path) strcpy(op->request.file_path, file_path);
            op->callback = cb;
            op->context = ctx;
        }
    }
    furi_mutex_release(weave->mutex);
    return status;
}
WeaveStatus weave_http_request(Weave* w, const char* method, const char* url,
    const uint8_t* body, size_t length, WeaveCallback cb, void* ctx) {
    if(!http_method(method) || !http_url(url)) return WeaveStatusInvalid;
    if((!strcmp(method, "GET") || !strcmp(method, "HEAD")) && length)
        return WeaveStatusInvalid;
    if(length > WEAVE_RESPONSE_MAX) return WeaveStatusTooLarge;
    return submit(w, method, url, body, length, NULL, cb, ctx);
}
WeaveStatus weave_http_get(Weave* w, const char* url, WeaveCallback cb, void* ctx) {
    return weave_http_request(w, "GET", url, NULL, 0, cb, ctx);
}
WeaveStatus weave_http_upload_file(Weave* w, const char* method, const char* url,
    const char* sd_path, WeaveCallback cb, void* ctx) {
    if(!http_method(method) || !strcmp(method, "GET") || !strcmp(method, "HEAD") ||
       !http_url(url) || !safe_field(sd_path, WEAVE_URL_MAX) ||
       strncmp(sd_path, "/ext/", 5) || strstr(sd_path, "/../") ||
       strstr(sd_path, "/./") || strstr(sd_path, "//") ||
       strlen(sd_path) < 6 || sd_path[strlen(sd_path)-1] == '/') return WeaveStatusInvalid;
    return submit(w, method, url, NULL, 0, sd_path, cb, ctx);
}
/* HTTPX payload: "HX1" + method length + method + header count +
 * (name length, 16-bit value length, name, value)* + raw entity bytes.
 * The metadata remains in a bounded 512-byte frame; file uploads append
 * their content from the SD directly without retaining it in SRAM.
 */
static bool http_header_name(const char* name, size_t* len) {
    if(!name) return false;
    size_t n = strnlen(name, 65);
    if(!n || n > 64) return false;
    for(size_t i = 0; i < n; i++) {
        const char c = name[i];
        if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
             (c >= '0' && c <= '9') || c == '-')) return false;
    }
    const char* forbidden[] = {"Host", "Content-Length", "Connection", "Transfer-Encoding",
        "Upgrade", "Keep-Alive", "TE", "Trailer", "Proxy-Authorization", "Proxy-Connection"};
    for(size_t i = 0; i < COUNT_OF(forbidden); i++)
        if(!strcasecmp(name, forbidden[i])) return false;
    *len = n;
    return true;
}
static WeaveStatus httpx_pack(const char* method, const WeaveHttpHeader* headers,
    size_t count, const uint8_t* body, size_t size, uint8_t* out, size_t* encoded) {
    if(!http_method(method) || count > WEAVE_HTTP_HEADERS_MAX || (count && !headers) ||
       (size && !body)) return WeaveStatusInvalid;
    const size_t ml = strlen(method);
    size_t pos = 0;
    memcpy(out, "HX1", 3);
    pos = 3;
    out[pos++] = (uint8_t)ml;
    memcpy(out + pos, method, ml);
    pos += ml;
    out[pos++] = (uint8_t)count;
    for(size_t i = 0; i < count; i++) {
        size_t nl = 0;
        if(!http_header_name(headers[i].name, &nl) || !headers[i].value)
            return WeaveStatusInvalid;
        for(size_t j = 0; j < i; j++) {
            if(!strcasecmp(headers[j].name, headers[i].name)) return WeaveStatusInvalid;
        }
        size_t vl = strnlen(headers[i].value, 257);
        if(vl > 256) return WeaveStatusTooLarge;
        for(size_t j = 0; j < vl; j++)
            if((unsigned char)headers[i].value[j] < 0x20 ||
               (unsigned char)headers[i].value[j] > 0x7e) return WeaveStatusInvalid;
        if(pos + nl + vl + 3 > WEAVE_RESPONSE_MAX) return WeaveStatusTooLarge;
        out[pos++] = (uint8_t)nl;
        out[pos++] = (uint8_t)(vl & 255);
        out[pos++] = (uint8_t)(vl >> 8);
        memcpy(out + pos, headers[i].name, nl);
        pos += nl;
        memcpy(out + pos, headers[i].value, vl);
        pos += vl;
    }
    if(size > WEAVE_RESPONSE_MAX - pos) return WeaveStatusTooLarge;
    if(size) memcpy(out + pos, body, size);
    *encoded = pos + size;
    return WeaveStatusOk;
}
WeaveStatus weave_http_request_ex(Weave* w, const char* method, const char* url,
    const WeaveHttpHeader* headers, size_t header_count,
    const uint8_t* body, size_t length, WeaveCallback cb, void* ctx) {
    if(!http_url(url) || strncmp(url,"https://",8)) return WeaveStatusInvalid;
    if((!strcmp(method ? method : "", "GET") || !strcmp(method ? method : "", "HEAD")) && length)
        return WeaveStatusInvalid;
    uint8_t bytes[WEAVE_RESPONSE_MAX];
    size_t total = 0;
    WeaveStatus status = httpx_pack(method, headers, header_count, body, length, bytes, &total);
    if(status != WeaveStatusOk) return status;
    return submit(w, "HTTPX", url, bytes, total, NULL, cb, ctx);
}
WeaveStatus weave_http_upload_file_ex(Weave* w, const char* method, const char* url,
    const WeaveHttpHeader* headers, size_t header_count,
    const char* sd_path, WeaveCallback cb, void* ctx) {
    if(!http_url(url) || strncmp(url,"https://",8) || !http_method(method) ||
       !strcmp(method,"GET") || !strcmp(method,"HEAD") ||
       !safe_field(sd_path, WEAVE_URL_MAX) || strncmp(sd_path,"/ext/",5) ||
       strstr(sd_path,"/../") || strstr(sd_path,"/./") || strstr(sd_path,"//") ||
       sd_path[strlen(sd_path)-1] == '/') return WeaveStatusInvalid;
    uint8_t bytes[WEAVE_RESPONSE_MAX];
    size_t total = 0;
    WeaveStatus status = httpx_pack(method,headers,header_count,NULL,0,bytes,&total);
    if(status != WeaveStatusOk) return status;
    return submit(w, "HTTPX", url, bytes, total, sd_path, cb, ctx);
}
static const char* socket_verb(WeaveSocketType type, const char* action) {
    if(type == WeaveSocketTcp) {
        if(!strcmp(action,"OPEN")) return "TCP_OPEN";
        if(!strcmp(action,"SEND")) return "TCP_SEND";
        if(!strcmp(action,"RECV")) return "TCP_RECV";
        if(!strcmp(action,"WATCH")) return "TCP_WATCH";
        if(!strcmp(action,"CLOSE")) return "TCP_CLOSE";
        return NULL;
    }
    if(type == WeaveSocketUdp) {
        if(!strcmp(action,"OPEN")) return "UDP_OPEN";
        if(!strcmp(action,"SEND")) return "UDP_SEND";
        if(!strcmp(action,"RECV")) return "UDP_RECV";
        if(!strcmp(action,"WATCH")) return "UDP_WATCH";
        if(!strcmp(action,"CLOSE")) return "UDP_CLOSE";
        return NULL;
    }
    if(type == WeaveSocketWebSocket) {
        if(!strcmp(action,"OPEN")) return "WS_OPEN";
        if(!strcmp(action,"SEND")) return "WS_SEND";
        if(!strcmp(action,"RECV")) return "WS_RECV";
        if(!strcmp(action,"WATCH")) return "WS_WATCH";
        if(!strcmp(action,"CLOSE")) return "WS_CLOSE";
        return NULL;
    }
    return NULL;
}
WeaveStatus weave_socket_open(Weave* w, WeaveSocketType type, const char* host,
    uint16_t port, WeaveCallback cb, void* ctx) {
    const char* verb = socket_verb(type, "OPEN");
    if(!verb || !safe_field(host, WEAVE_URL_MAX)) return WeaveStatusInvalid;
    if(type == WeaveSocketWebSocket) {
        if(strncmp(host, "wss://", 6) && strncmp(host, "ws://", 5)) return WeaveStatusInvalid;
        return submit(w, verb, host, NULL, 0, NULL, cb, ctx);
    }
    if(!port || strlen(host) > WEAVE_URL_MAX - 8 || strchr(host, ':') || strchr(host, '/'))
        return WeaveStatusInvalid;
    char target[WEAVE_URL_MAX];
    snprintf(target, sizeof(target), "%s:%u", host, port);
    return submit(w, verb, target, NULL, 0, NULL, cb, ctx);
}
static WeaveStatus socket_op(Weave* w, WeaveSocketType type, const char* action,
    uint16_t session, const uint8_t* bytes, size_t length, WeaveCallback cb, void* ctx) {
    const char* verb = socket_verb(type, action);
    if(!verb || !session) return WeaveStatusInvalid;
    if(length > WEAVE_RESPONSE_MAX) return WeaveStatusTooLarge;
    char target[8];
    snprintf(target, sizeof(target), "%u", session);
    return submit(w, verb, target, bytes, length, NULL, cb, ctx);
}
WeaveStatus weave_socket_send(Weave* w, WeaveSocketType type, uint16_t session,
    const uint8_t* bytes, size_t length, WeaveCallback cb, void* ctx) {
    return socket_op(w, type, "SEND", session, bytes, length, cb, ctx);
}
WeaveStatus weave_socket_receive(Weave* w, WeaveSocketType type,
    uint16_t session, WeaveCallback cb, void* ctx) {
    return socket_op(w, type, "RECV", session, NULL, 0, cb, ctx);
}
WeaveStatus weave_socket_close(Weave* w, WeaveSocketType type,
    uint16_t session, WeaveCallback cb, void* ctx) {
    return socket_op(w, type, "CLOSE", session, NULL, 0, cb, ctx);
}

WeaveStatus weave_socket_watch(Weave* w, WeaveSocketType type,
    uint16_t session, bool enabled, WeaveCallback cb, void* ctx) {
    const uint8_t mode = enabled ? 1 : 0;
    return socket_op(w,type,"WATCH",session,&mode,1,cb,ctx);
}
