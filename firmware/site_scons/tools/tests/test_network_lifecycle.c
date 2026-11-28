#include "weave_i.h"
#include "weave_permissions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static const char* caller="app-a";
static uint32_t tick;
static bool memory_ok=true;
static unsigned replies, transport_events, socket_events;
int furi_mutex_acquire(FuriMutex* m,int wait){(void)wait;assert(!m->held);m->held=true;return 0;}
int furi_mutex_release(FuriMutex* m){assert(m->held);m->held=false;return 0;}
int32_t furi_kernel_lock(void){return 0;}
void furi_kernel_unlock(void){}
size_t memmgr_heap_get_max_free_block(void){return memory_ok?100000:0;}
uint32_t furi_get_tick(void){return tick;}
uint32_t furi_ms_to_ticks(uint32_t ms){return ms;}
void furi_delay_ms(uint32_t ms){tick+=ms;}
const char* weave_caller_appid(void){return caller;}
bool weave_permission_check(WeavePermission p){(void)p;return true;}
static void reply(const WeaveResponse* r,void* ctx){(void)r;(void)ctx;replies++;}
static void transport(WeaveTransport t,void* ctx){(void)t;(void)ctx;transport_events++;}
static void socket_event(const WeaveSocketEvent* e,void* ctx){(void)e;(void)ctx;socket_events++;}
static WeaveRequest poll(Weave* w){WeaveRequest req={0};assert(weave_gateway_poll(w,&req));return req;}
static void complete(Weave* w,WeaveRequest req,uint16_t session){
    const WeaveResponse r={.id=req.id,.status=WeaveStatusOk,.http_status=session};
    assert(weave_gateway_reply(w,&r));
}
static void open_socket(Weave* w,uint16_t session,void* ctx){
    assert(weave_socket_open(w,WeaveSocketTcp,"example.org",443,reply,ctx)==WeaveStatusOk);
    complete(w,poll(w),session);
}
int main(void){
    FuriMutex m={0}; Weave w={.mutex=&m,.transport=WeaveTransportUsb};
    int context=0;
    /* Shared NULL and non-NULL contexts may not route across app identities. */
    assert(weave_transport_subscribe(&w,transport,NULL));
    assert(weave_socket_subscribe_events(&w,socket_event,NULL));
    open_socket(&w,10,NULL);
    assert(weave_socket_watch(&w,WeaveSocketTcp,10,true,reply,NULL)==WeaveStatusOk);
    complete(&w,poll(&w),0);
    caller="app-b";
    assert(weave_transport_subscribe(&w,transport,NULL));
    assert(weave_socket_subscribe_events(&w,socket_event,NULL));
    assert(weave_socket_send(&w,WeaveSocketTcp,10,(const uint8_t*)"x",1,reply,NULL)==WeaveStatusDenied);
    weave_gateway_socket_event(&w,&(WeaveSocketEvent){.type=WeaveSocketTcp,.session=10});
    assert(socket_events==1);
    /* Queue an OPEN already transmitted by the gateway and another pending A request. */
    caller="app-a";
    assert(weave_socket_open(&w,WeaveSocketTcp,"example.org",443,reply,&context)==WeaveStatusOk);
    WeaveRequest pending=poll(&w);
    assert(weave_http_get(&w,"https://example.org",reply,NULL)==WeaveStatusOk);
    caller="app-b";
    assert(weave_http_get(&w,"https://example.org",reply,&context)==WeaveStatusOk);
    assert(weave_network_release_owner(&w,"app-a",0));
    caller="app-a";
    assert(weave_http_get(&w,"https://example.org",reply,NULL)==WeaveStatusDenied);
    assert(!weave_transport_subscribe(&w,transport,&context));
    const unsigned before=replies;
    complete(&w,pending,11); /* Late OPEN: no FAP callback, CLOSE queued. */
    assert(replies==before);
    WeaveRequest req=poll(&w); assert(!strcmp(req.method,"GET")); complete(&w,req,0);
    assert(replies==before+1);
    req=poll(&w);assert(!strcmp(req.method,"TCP_CLOSE")&&!strcmp(req.target,"10"));complete(&w,req,0);
    req=poll(&w);assert(!strcmp(req.method,"TCP_CLOSE")&&!strcmp(req.target,"11"));complete(&w,req,0);
    assert(!w.queue && !w.closing_count);
    weave_gateway_attach(&w,WeaveTransportBle);assert(transport_events==1); /* only B */
    /* In-progress callbacks make loader wait even with NULL context. */
    furi_mutex_acquire(&m,0);weave_callback_started_locked(&w,NULL,"app-a");weave_callback_started_locked(&w,NULL,"app-b");furi_mutex_release(&m);
    assert(!weave_network_release_owner(&w,"app-a",3));
    weave_callback_finished(&w,NULL,"app-a");assert(weave_network_release_owner(&w,"app-a",0));
    weave_callback_finished(&w,NULL,"app-b");weave_network_finish_owner(&w,"app-a");
    /* All eight slots reserved; ninth OPEN denied, no orphan on exhaustion. */
    for(uint16_t i=1;i<=WEAVE_TRACKED_SESSIONS;i++) open_socket(&w,i,&context);
    assert(weave_socket_open(&w,WeaveSocketTcp,"example.org",443,reply,&context)==WeaveStatusBusy);
    /* Simulated allocation failure: all close IDs retained for retry. */
    memory_ok=false;assert(weave_context_release(&w,&context,0));assert(w.closing_count==WEAVE_TRACKED_SESSIONS);
    WeaveRequest empty;assert(!weave_gateway_poll(&w,&empty));memory_ok=true;
    for(uint16_t i=1;i<=WEAVE_TRACKED_SESSIONS;i++){req=poll(&w);assert(!strcmp(req.method,"TCP_CLOSE"));assert((unsigned)atoi(req.target)==i);complete(&w,req,0);}
    assert(!w.queue&&!w.closing_count&&!w.callbacks_active);
    weave_gateway_attach(&w,WeaveTransportNone);
    puts("Network owner routing, NULL contexts, late OPEN cleanup, draining, capacity, retry: PASS");
}
