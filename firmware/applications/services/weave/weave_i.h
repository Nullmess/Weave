#ifndef WEAVE_I_H
#define WEAVE_I_H
#include "weave.h"
#include <furi.h>

typedef struct {
    uint32_t id;
    char method[16]; /* HTTP method or socket operation */
    char target[WEAVE_URL_MAX];
    uint8_t body[WEAVE_RESPONSE_MAX];
    uint16_t body_size;
    char file_path[WEAVE_URL_MAX]; /* optional /ext/ upload source */
} WeaveRequest;

#define WEAVE_OWNER_SIZE 64
typedef struct {
    WeaveRequest request;
    WeaveCallback callback;
    void* context;
    bool dispatched;
    bool detached; /* In-flight operation retained only for remote cleanup. */
    char appid[WEAVE_OWNER_SIZE];
} WeaveOperation;
typedef struct {
    WeaveTransportCallback callback;
    void* context;
    char appid[WEAVE_OWNER_SIZE];
} WeaveTransportListener;
typedef struct {
    WeaveSocketEventCallback callback;
    void* context;
    char appid[WEAVE_OWNER_SIZE];
} WeaveSocketEventListener;
/* Companion sessions are ephemeral and scoped to the opening FAP context.
 * This filters unsolicited events; it is not an application sandbox. */
typedef struct {
    void* context;
    uint16_t count;
    char appid[WEAVE_OWNER_SIZE];
} WeaveActiveCallback;
typedef struct {
    uint16_t id;
    WeaveSocketType type;
    void* owner;
    bool watched;
    char appid[WEAVE_OWNER_SIZE];
} WeaveTrackedSession;
#define WEAVE_TRACKED_SESSIONS 8
struct Weave {
    FuriMutex* mutex;
    WeaveTransport transport;
    /* Keep the network request buffer out of RAM while idle. NFC is a large
     * external FAP and must be able to preload without an unused ~5 KiB FIFO
     * permanently occupying the heap. Allocations are protected by mutex. */
    WeaveOperation* queue;
    uint8_t queue_count;
    WeaveTransportListener listeners[4];
    WeaveSocketEventListener socket_listeners[4];
    WeaveTrackedSession sessions[WEAVE_TRACKED_SESSIONS];
    WeaveTrackedSession closing_sessions[WEAVE_TRACKED_SESSIONS];
    uint8_t closing_count;
    char draining_app[WEAVE_OWNER_SIZE]; /* Loader owns the single FAP unload path. */
    uint32_t next_id;
    uint32_t callbacks_active; /* total active callbacks (diagnostics) */
    /* At most 4 queued replies + 4 transport + 4 socket listeners. */
    WeaveActiveCallback callbacks_by_context[12];
};

void weave_reset(Weave* weave);
/* Internal helpers; call only while holding weave->mutex. */
bool weave_queue_reserve_locked(Weave* weave);
void weave_queue_release_if_idle_locked(Weave* weave);
void weave_callback_started_locked(Weave* weave, void* context, const char* appid);
void weave_callback_finished(Weave* weave, void* context, const char* appid);
void weave_flush_cleanup_locked(Weave* weave);
bool weave_network_release_owner(Weave* weave, const char* appid, uint32_t timeout_ms);
void weave_network_finish_owner(Weave* weave, const char* appid);
void weave_gateway_attach(Weave* weave, WeaveTransport transport);
bool weave_gateway_poll(Weave* weave, WeaveRequest* request);
bool weave_gateway_active(Weave* weave, uint32_t id);
void weave_gateway_socket_event(Weave* weave, const WeaveSocketEvent* event);
bool weave_gateway_reply(Weave* weave, const WeaveResponse* response);
#endif
