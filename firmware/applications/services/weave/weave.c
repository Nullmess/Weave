#include "weave_i.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

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
void weave_reset(Weave* weave) {
    weave->pending = false;
    weave->dispatched = false;
    weave->callback = NULL;
    weave->context = NULL;
}
WeaveTransport weave_get_transport(Weave* weave) {
    furi_check(weave);
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    WeaveTransport transport = weave->transport;
    furi_mutex_release(weave->mutex);
    return transport;
}
static WeaveStatus submit(Weave* weave, const char* verb, const char* target,
    const uint8_t* data, size_t size, const char* file_path, WeaveCallback cb, void* ctx) {
    if(!weave || !cb || !safe_field(verb, sizeof(weave->request.method)) ||
       !safe_field(target, WEAVE_URL_MAX) || size > WEAVE_RESPONSE_MAX || (size && !data))
        return WeaveStatusInvalid;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    WeaveStatus status = WeaveStatusOk;
    if(weave->transport == WeaveTransportNone) status = WeaveStatusOffline;
    else if(weave->pending) status = WeaveStatusBusy;
    else {
        if(++weave->next_id == 0) ++weave->next_id;
        WeaveRequest* r = &weave->request;
        memset(r, 0, sizeof(*r));
        r->id = weave->next_id;
        strcpy(r->method, verb);
        strcpy(r->target, target);
        if(size) memcpy(r->body, data, size);
        r->body_size = (uint16_t)size;
        if(file_path) strcpy(r->file_path, file_path);
        weave->callback = cb;
        weave->context = ctx;
        weave->pending = true;
        weave->dispatched = false;
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
static const char* socket_verb(WeaveSocketType type, const char* action) {
    if(type == WeaveSocketTcp) {
        if(!strcmp(action,"OPEN")) return "TCP_OPEN";
        if(!strcmp(action,"SEND")) return "TCP_SEND";
        if(!strcmp(action,"RECV")) return "TCP_RECV";
        return "TCP_CLOSE";
    }
    if(type == WeaveSocketUdp) {
        if(!strcmp(action,"OPEN")) return "UDP_OPEN";
        if(!strcmp(action,"SEND")) return "UDP_SEND";
        if(!strcmp(action,"RECV")) return "UDP_RECV";
        return "UDP_CLOSE";
    }
    if(type == WeaveSocketWebSocket) {
        if(!strcmp(action,"OPEN")) return "WS_OPEN";
        if(!strcmp(action,"SEND")) return "WS_SEND";
        if(!strcmp(action,"RECV")) return "WS_RECV";
        return "WS_CLOSE";
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
