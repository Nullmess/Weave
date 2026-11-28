#include "weave_i.h"
#include <string.h>
#include <stdlib.h>

static bool session_operation(const char* method, const char* action,
                              WeaveSocketType* type) {
    if(!strncmp(method, "TCP_", 4) && !strcmp(method+4, action)) {
        *type = WeaveSocketTcp;
        return true;
    }
    if(!strncmp(method, "UDP_", 4) && !strcmp(method+4, action)) {
        *type = WeaveSocketUdp;
        return true;
    }
    if(!strncmp(method, "WS_", 3) && !strcmp(method+3, action)) {
        *type = WeaveSocketWebSocket;
        return true;
    }
    return false;
}

static void session_update(Weave* weave, const WeaveOperation* op,
                           const WeaveResponse* response) {
    if(response->status != WeaveStatusOk || response->more) return;
    WeaveSocketType type;
    if(session_operation(op->request.method,"OPEN",&type)) {
        if(!response->http_status) return;
        /* Never reassign an already live remote session ID. */
        for(size_t i=0; i<COUNT_OF(weave->sessions); i++)
            if(weave->sessions[i].id == response->http_status && weave->sessions[i].type == type) return;
        if(op->detached) {
            furi_check(weave->closing_count < COUNT_OF(weave->closing_sessions));
            weave->closing_sessions[weave->closing_count++] = (WeaveTrackedSession){
                .id=response->http_status,.type=type};
            return;
        }
        for(size_t i=0; i<COUNT_OF(weave->sessions); i++) {
            WeaveTrackedSession* session=&weave->sessions[i];
            if(!session->id) {
                *session=(WeaveTrackedSession){.id=response->http_status,.type=type,.owner=op->context};
                strcpy(session->appid,op->appid);
                return;
            }
        }
        /* Admission in submit reserves capacity for every pending OPEN. */
        furi_check(false);
        return;
    }
    const bool watch = session_operation(op->request.method,"WATCH",&type);
    if(!watch && !session_operation(op->request.method,"CLOSE",&type))return;
    char* end = NULL;
    const unsigned long id = strtoul(op->request.target,&end,10);
    if(!id || id > UINT16_MAX || !end || *end) return;
    for(size_t i = 0; i < COUNT_OF(weave->sessions); i++) {
        WeaveTrackedSession* session = &weave->sessions[i];
        if(session->id == id && session->type == type) {
            if(watch) session->watched = op->request.body_size == 1 &&
                                      op->request.body[0] == 1;
            else *session = (WeaveTrackedSession){0};
            return;
        }
    }
}

void weave_gateway_attach(Weave* weave, WeaveTransport transport) {
    furi_check(weave);
    if(transport > WeaveTransportUsb) return;
    WeaveCallback callbacks[WEAVE_QUEUE_MAX] = {0};
    void* contexts[WEAVE_QUEUE_MAX] = {0};
    uint32_t ids[WEAVE_QUEUE_MAX] = {0};
    char appids[WEAVE_QUEUE_MAX][WEAVE_OWNER_SIZE] = {0};
    WeaveTransportListener listeners[4] = {0};
    size_t aborted_count = 0;
    bool changed = false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    changed = transport != weave->transport;
    if(changed) {
        aborted_count = weave->queue_count;
        for(size_t i = 0; i < aborted_count; i++) {
            callbacks[i] = weave->queue[i].callback;
            contexts[i] = weave->queue[i].context;
            ids[i] = weave->queue[i].request.id;
            strcpy(appids[i],weave->queue[i].appid);
        }
        /* Aborting a transport frees the whole FIFO, including its unused
         * capacity. Callback IDs/contexts have already been copied above. */
        free(weave->queue);
        weave->queue = NULL;
        memset(weave->sessions, 0, sizeof(weave->sessions));
        memset(weave->closing_sessions, 0, sizeof(weave->closing_sessions));
        weave->closing_count = 0;
        weave->queue_count = 0;
        memcpy(listeners, weave->listeners, sizeof(listeners));
        for(size_t i = 0; i < aborted_count; i++)
            if(callbacks[i]) weave_callback_started_locked(weave, contexts[i], appids[i]);
        for(size_t i = 0; i < COUNT_OF(listeners); i++)
            if(listeners[i].callback) weave_callback_started_locked(weave, listeners[i].context, listeners[i].appid);
    }
    weave->transport = transport;
    furi_mutex_release(weave->mutex);
    for(size_t i = 0; i < aborted_count; i++) {
        if(callbacks[i]) {
            const WeaveResponse response = {.id=ids[i],.status=WeaveStatusOffline};
            callbacks[i](&response,contexts[i]);
            weave_callback_finished(weave, contexts[i], appids[i]);
        }
    }
    if(changed) for(size_t i = 0; i < COUNT_OF(listeners); i++) {
        if(listeners[i].callback) {
            listeners[i].callback(transport,listeners[i].context);
            weave_callback_finished(weave, listeners[i].context, listeners[i].appid);
        }
    }
}
bool weave_gateway_poll(Weave* weave, WeaveRequest* request) {
    if(!weave || !request) return false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    weave_flush_cleanup_locked(weave);
    bool ready = weave->queue_count && !weave->queue[0].dispatched;
    if(ready) {
        *request = weave->queue[0].request;
        weave->queue[0].dispatched = true;
    }
    furi_mutex_release(weave->mutex);
    return ready;
}
bool weave_gateway_active(Weave* weave, uint32_t id) {
    if(!weave || !id) return false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    bool active = weave->queue_count && weave->queue[0].dispatched &&
        weave->queue[0].request.id == id;
    furi_mutex_release(weave->mutex);
    return active;
}
bool weave_gateway_reply(Weave* weave, const WeaveResponse* response) {
    if(!weave || !response || response->size > WEAVE_RESPONSE_MAX ||
       response->total > WEAVE_STREAM_MAX ||
       response->offset > response->total ||
       response->size > response->total - response->offset ||
       (response->size && !response->data)) return false;
    WeaveCallback callback = NULL;
    void* context = NULL;
    char appid[WEAVE_OWNER_SIZE] = {0};
    bool accepted = false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    if(weave->queue_count && weave->queue[0].dispatched &&
       response->id == weave->queue[0].request.id) {
        accepted = true;
        callback = weave->queue[0].callback;
        context = weave->queue[0].context;
        strcpy(appid,weave->queue[0].appid);
        if(callback) weave_callback_started_locked(weave, context, appid);
        session_update(weave,&weave->queue[0],response);
        if(!response->more) weave_reset(weave);
    }
    furi_mutex_release(weave->mutex);
    if(callback) {
        callback(response, context);
        weave_callback_finished(weave, context, appid);
    }
    return accepted;
}

/* Deliver events only to a listener sharing the owner's callback context.
 * NULL owner/listener remains available for legacy FAPs without a context;
 * pointer equality is cooperative routing, not process isolation. */
void weave_gateway_socket_event(Weave* weave, const WeaveSocketEvent* event) {
    if(!weave || !event || (event->size && !event->data) || !event->session ||
       event->type > WeaveSocketWebSocket || event->size > WEAVE_RESPONSE_MAX ||
       event->total > 4096 || event->offset > event->total ||
       event->size > event->total - event->offset ||
       event->more != (event->offset + event->size < event->total)) return;
    WeaveSocketEventListener callbacks[4] = {0};
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    bool valid = false;
    void* owner = NULL;
    const char* appid = NULL;
    if(weave->transport != WeaveTransportNone) {
        for(size_t i = 0; i < COUNT_OF(weave->sessions); i++) {
            const WeaveTrackedSession* s = &weave->sessions[i];
            if(s->id == event->session && s->type == event->type && s->watched) {
                owner = s->owner;
                appid = s->appid;
                valid = true;
                break;
            }
        }
    }
    if(valid) {
        for(size_t i = 0; i < COUNT_OF(callbacks); i++) {
            const WeaveSocketEventListener* l = &weave->socket_listeners[i];
            if(l->callback && l->context == owner && !strcmp(l->appid,appid)) {
                callbacks[i] = *l;
                weave_callback_started_locked(weave, l->context, l->appid);
            }
        }
    }
    furi_mutex_release(weave->mutex);
    for(size_t i = 0; i < COUNT_OF(callbacks); i++) {
        if(callbacks[i].callback) {
            callbacks[i].callback(event,callbacks[i].context);
            weave_callback_finished(weave, callbacks[i].context, callbacks[i].appid);
        }
    }
}
