#include "weave_i.h"
#include <string.h>

void weave_gateway_attach(Weave* weave, WeaveTransport transport) {
    WeaveCallback callback = NULL;
    void* context = NULL;
    WeaveResponse response = {0};
    response.status = WeaveStatusOffline;
    furi_check(weave);
    if(transport > WeaveTransportUsb) return;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    if(transport != weave->transport && weave->pending) {
        response.id = weave->request.id;
        callback = weave->callback;
        context = weave->context;
        weave_reset(weave);
    }
    weave->transport = transport;
    furi_mutex_release(weave->mutex);
    if(callback) callback(&response, context);
}
bool weave_gateway_poll(Weave* weave, WeaveRequest* request) {
    if(!weave || !request) return false;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    bool ready = weave->pending && !weave->dispatched;
    if(ready) {
        *request = weave->request;
        weave->dispatched = true;
    }
    furi_mutex_release(weave->mutex);
    return ready;
}
bool weave_gateway_reply(Weave* weave, const WeaveResponse* response) {
    if(!weave || !response || response->size > WEAVE_RESPONSE_MAX ||
       response->total > WEAVE_STREAM_MAX ||
       response->offset > response->total ||
       response->size > response->total - response->offset ||
       (response->size && !response->data)) return false;
    WeaveCallback callback = NULL;
    void* context = NULL;
    furi_mutex_acquire(weave->mutex, FuriWaitForever);
    if(weave->pending && weave->dispatched && response->id == weave->request.id) {
        callback = weave->callback;
        context = weave->context;
        if(!response->more) weave_reset(weave);
    }
    furi_mutex_release(weave->mutex);
    if(callback) callback(response, context);
    return callback != NULL;
}
