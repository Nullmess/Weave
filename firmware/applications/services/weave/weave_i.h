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

struct Weave {
    FuriMutex* mutex;
    WeaveTransport transport;
    WeaveRequest request;
    WeaveCallback callback;
    void* context;
    uint32_t next_id;
    bool pending;
    bool dispatched;
};

void weave_reset(Weave* weave);
void weave_gateway_attach(Weave* weave, WeaveTransport transport);
bool weave_gateway_poll(Weave* weave, WeaveRequest* request);
bool weave_gateway_reply(Weave* weave, const WeaveResponse* response);
#endif
