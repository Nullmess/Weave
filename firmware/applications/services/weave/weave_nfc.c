#include "weave_nfc.h"
#include "weave_nfc_mailbox.h"
#include "weave_isodep.h"
#include "weave_permissions.h"
#include "weave_internal.h"
#include <furi.h>
#include <nfc/nfc_listener.h>
#include <nfc/protocols/iso14443_4a/iso14443_4a_listener.h>
#include <furi_hal_random.h>
#include <string.h>
#include <stdlib.h>
/* Internal f7 APIs, not exported to FAPs. */
extern Nfc* nfc_try_alloc(uint32_t timeout);
extern uint16_t iso14443_4a_listener_peer_frame_size(const Iso14443_4aListener* instance);
extern bool nfc_listener_enable_weave_isodep(NfcListener* instance);
extern bool iso14443_4a_listener_send_raw(Iso14443_4aListener* instance, const BitBuffer* frame);
static FuriMutex* mutex;
static FuriThread* worker;
static void* lease_owner;
static char appid[64];
static WeaveNfcMailbox mailbox;
static bool stopping, started, failed;
static uint32_t deadline_start;
static BitBuffer* tx;
static WeaveIsoDep* isodep;
void weave_nfc_broker_init(void) {
    if(!mutex) mutex=furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(mutex);
}
static bool owns(void* owner) {
    const char* id=weave_caller_appid();
    return lease_owner && lease_owner==owner && id && !strcmp(id,appid);
}
static bool expired(void) {
    return (uint32_t)(furi_get_tick()-deadline_start)>=furi_ms_to_ticks(WEAVE_NFC_RESPONSE_MS);
}
static void reset_exchange(void) {
    FURI_CRITICAL_ENTER();weave_nfc_mailbox_clear(&mailbox);FURI_CRITICAL_EXIT();
    weave_isodep_reset(isodep);
}
static NfcCommand exchange(NfcGenericEvent event, void* context) {
    (void)context;
    Iso14443_4aListenerEvent* e=event.event_data;
    if(e->type!=Iso14443_4aListenerEventTypeReceivedData) {
        reset_exchange();return NfcCommandContinue;
    }
    bool stop;
    { FURI_CRITICAL_ENTER();stop=stopping || (isodep->phase==WeaveIsoApp && expired());FURI_CRITICAL_EXIT(); }
    if(stop) { reset_exchange();return NfcCommandSleep; }
    const BitBuffer* rx=e->data->buffer;
    if(bit_buffer_get_size(rx)%8U) return NfcCommandContinue;
    WeaveIsoAction action=weave_isodep_input(isodep,bit_buffer_get_data(rx),bit_buffer_get_size_bytes(rx),
        iso14443_4a_listener_peer_frame_size(event.instance));
    if(action==WeaveIsoRequest) {
        bool ready;
        { FURI_CRITICAL_ENTER();
          ready=!stopping && weave_nfc_mailbox_begin(&mailbox,isodep->request,isodep->request_size,WEAVE_NFC_PACKET_MAX);
          deadline_start=furi_get_tick();FURI_CRITICAL_EXIT(); }
        if(!ready) action=WeaveIsoAbort;
        else {
            /* The mailbox now owns the request. */
            weave_security_wipe(isodep->request,sizeof(isodep->request));
            isodep->request_size=0;
        }
    }
    if(action==WeaveIsoRequest||action==WeaveIsoWait) {
        /* FWI=8 (~77 ms). Each acknowledged WTXM=8 grants ~618 ms.
         * Leave margin for native radio/RTOS scheduling; bound total time. */
        const uint32_t budget=furi_ms_to_ticks(action==WeaveIsoRequest?25U:400U);
        const uint32_t start=furi_get_tick();
        uint8_t response[WEAVE_NFC_PACKET_MAX];size_t size=0;
        bool abort;
        for(;;) {
            { FURI_CRITICAL_ENTER();abort=stopping||expired();
              if(!abort) size=weave_nfc_mailbox_take(&mailbox,response,sizeof(response));
              FURI_CRITICAL_EXIT(); }
            if(abort||size||(uint32_t)(furi_get_tick()-start)>=budget) break;
            furi_delay_tick(1);
        }
        action=abort?WeaveIsoAbort:size?weave_isodep_reply(isodep,response,size):weave_isodep_wtx(isodep);
        weave_security_wipe(response,sizeof(response));
    }
    if(action==WeaveIsoSend||action==WeaveIsoHalt) {
        bit_buffer_copy_bytes(tx,isodep->frame,isodep->frame_size);
        bool sent=iso14443_4a_listener_send_raw(event.instance,tx);
        /* Wipe native staging buffer after transmission; cache lives in state. */
        uint8_t zero[WEAVE_ISODEP_FRAME_MAX]={0};
        bit_buffer_copy_bytes(tx,zero,isodep->frame_size);bit_buffer_reset(tx);
        if(!sent) action=WeaveIsoAbort;
    }
    if(action==WeaveIsoHalt||action==WeaveIsoAbort) { reset_exchange();return NfcCommandSleep; }
    return NfcCommandContinue;
}
static int32_t worker_run(void* context) {
    (void)context;
    /* This worker owns both acquire and release of the native NFC mutex. */
    Nfc* nfc=nfc_try_alloc(furi_ms_to_ticks(100));
    if(!nfc) {
        FURI_CRITICAL_ENTER();failed=true;FURI_CRITICAL_EXIT();return 0;
    }
    Iso14443_4aData* data=iso14443_4a_alloc();iso14443_4a_reset(data);
    uint8_t uid[4];furi_hal_random_fill_buf(uid,sizeof(uid));uid[0]=0x08;
    iso14443_4a_set_uid(data,uid,sizeof(uid));
    data->iso14443_3a_data->atqa[0]=0x04;data->iso14443_3a_data->atqa[1]=0;
    data->iso14443_3a_data->sak=0x20;
    /* FSCI=8, TB(1) present, FWI=8 (~77 ms), no CID/NAD. */
    data->ats_data.tl=3;data->ats_data.t0=0x28;data->ats_data.tb_1=0x80;
    NfcListener* listener=nfc_listener_alloc(nfc,NfcProtocolIso14443_4a,data);
    iso14443_4a_free(data);tx=bit_buffer_alloc(WEAVE_ISODEP_FRAME_MAX);
    isodep=malloc(sizeof(*isodep));furi_check(isodep);weave_isodep_reset(isodep);
    furi_check(nfc_listener_enable_weave_isodep(listener));
    nfc_listener_start(listener,exchange,NULL);
    { FURI_CRITICAL_ENTER();started=true;FURI_CRITICAL_EXIT(); }
    for(;;) {
        bool stop;
        { FURI_CRITICAL_ENTER();stop=stopping;FURI_CRITICAL_EXIT(); }
        if(stop) break;
        furi_delay_tick(1);
    }
    nfc_listener_stop(listener);reset_exchange();free(isodep);isodep=NULL;
    nfc_listener_free(listener);bit_buffer_free(tx);tx=NULL;nfc_free(nfc);
    { FURI_CRITICAL_ENTER();weave_nfc_mailbox_clear(&mailbox);started=false;FURI_CRITICAL_EXIT(); }
    return 0;
}
static void release_locked(void) {
    if(worker) {
        { FURI_CRITICAL_ENTER();stopping=true;weave_nfc_mailbox_clear(&mailbox);FURI_CRITICAL_EXIT(); }
        furi_thread_join(worker);furi_thread_free(worker);worker=NULL;
    }
    lease_owner=NULL;memset(appid,0,sizeof(appid));
}
WeaveNfcResult weave_nfc_acquire(void* owner) {
    if(!mutex || !owner) return WeaveNfcInvalid;
    const char* id=weave_caller_appid();
    if(!id || !weave_permission_check(WeavePermissionNfc)) return WeaveNfcDenied;
    furi_mutex_acquire(mutex,FuriWaitForever);
    WeaveNfcResult result=WeaveNfcBusy;
    if(!worker && !lease_owner) {
        /* Native NFC allocates an 8 KiB stack plus protocol objects. Reserve
         * conservative headroom; this is not a deterministic heap reservation. */
        if(memmgr_heap_get_max_free_block()<12U*1024U || memmgr_get_free_heap()<25U*1024U)
            result=WeaveNfcUnavailable;
        else {
            { FURI_CRITICAL_ENTER();stopping=started=failed=false;FURI_CRITICAL_EXIT(); }
            lease_owner=owner;strcpy(appid,id);
            worker=furi_thread_alloc_ex("WeaveNFC",3072,worker_run,NULL);furi_thread_start(worker);
            const uint32_t start=furi_get_tick();
            for(;;) {
                bool ok,bad;
                { FURI_CRITICAL_ENTER();ok=started;bad=failed;FURI_CRITICAL_EXIT(); }
                if(ok) {result=WeaveNfcOk;break;}
                if(bad || (uint32_t)(furi_get_tick()-start)>=furi_ms_to_ticks(1000)) {
                    release_locked();result=WeaveNfcUnavailable;break;
                }
                furi_delay_tick(1);
            }
        }
    }
    furi_mutex_release(mutex);return result;
}
WeaveNfcResult weave_nfc_release(void* owner) {
    if(!mutex || !owner) return WeaveNfcInvalid;
    furi_mutex_acquire(mutex,FuriWaitForever);
    const bool ok=owns(owner);if(ok)release_locked();
    furi_mutex_release(mutex);return ok?WeaveNfcOk:WeaveNfcNotOwner;
}
bool weave_nfc_release_app(const char* id) {
    if(!mutex || !id) return true;
    furi_mutex_acquire(mutex,FuriWaitForever);
    if(lease_owner && !strcmp(id,appid)) release_locked();
    furi_mutex_release(mutex);return true;
}
WeaveNfcResult weave_nfc_receive(void* owner, WeaveNfcRequest* request, uint8_t* data, size_t capacity) {
    if(!mutex || !owner || !request || !data) return WeaveNfcInvalid;
    *request=(WeaveNfcRequest){0};
    furi_mutex_acquire(mutex,FuriWaitForever);
    WeaveNfcResult result=WeaveNfcNotOwner;
    if(owns(owner)) {
        FURI_CRITICAL_ENTER();
        if(mailbox.active && expired()) weave_nfc_mailbox_clear(&mailbox);
        result=weave_nfc_mailbox_receive(&mailbox,request,data,capacity);
        FURI_CRITICAL_EXIT();
    }
    furi_mutex_release(mutex);return result;
}
WeaveNfcResult weave_nfc_respond(void* owner, uint32_t id, const uint8_t* data, size_t size) {
    if(!mutex || !owner || !data || !id || !size) return WeaveNfcInvalid;
    furi_mutex_acquire(mutex,FuriWaitForever);
    WeaveNfcResult result=WeaveNfcNotOwner;
    if(owns(owner)) {
        FURI_CRITICAL_ENTER();
        if(mailbox.active && expired()) weave_nfc_mailbox_clear(&mailbox);
        result=weave_nfc_mailbox_respond(&mailbox,id,data,size);
        FURI_CRITICAL_EXIT();
    }
    furi_mutex_release(mutex);return result;
}
