#include <furi.h>
#include <stdio.h>
#include "app_common.h"
#include <ble/ble.h>
#include <furi_ble/event_dispatcher.h>
#include <bt/bt_service/bt.h>
#include "weave_ble_profile.h"
#include "weave_ble_queue.h"
#include "weave_internal.h"
#include "weave_permissions.h"
static FuriMutex mutex;
static int records, starts, stops, created, chars, fail_char, updates;
static bool consent=true, available=true, native_busy, fail_restore;
static const char* app="alpha";
struct Bt {int unused;}; static Bt bt;
struct GapEventHandler {BleSvcEventHandlerCb cb;void* ctx;};
static struct GapEventHandler handler;
static FuriHalBleProfileBase* active;
static bool reserved;
FuriMutex* furi_mutex_alloc(int t){(void)t;return &mutex;}
int furi_mutex_acquire(FuriMutex* m,int w){(void)w;assert(!m->held);m->held=1;return 0;}
int furi_mutex_release(FuriMutex* m){assert(m->held);m->held=0;return 0;}
bool furi_record_exists(const char* n){assert(!strcmp(n,"bt"));return available;}
void* furi_record_open(const char* n){assert(!strcmp(n,"bt"));records++;return &bt;}
void furi_record_close(const char* n){assert(!strcmp(n,"bt")&&records==1);records--;}
const char* weave_caller_appid(void){return app;}
bool weave_permission_check(WeavePermission c){assert(c==WeavePermissionBle);return consent;}
bool furi_hal_bt_is_gatt_gap_supported(void){return available;}
void bt_disconnect(Bt* b){assert(b==&bt);weave_ble_profile_gap((GapEvent){.type=GapEventTypeDisconnected});}
FuriHalBleProfileBase* bt_weave_profile_start(Bt* b,const FuriHalBleProfileTemplate* profile,
    FuriHalBleProfileParams params,bool* r){
    assert(b==&bt&&profile==weave_ble_profile&&!active&&!reserved);*r=false;
    if(native_busy)return NULL;
    *r=reserved=true;starts++;
    GapConfig cfg;profile->get_gap_config(&cfg,params);
    assert(!cfg.bonding_mode&&cfg.pairing_method==GapPairingPinCodeVerifyYesNo);
    assert(cfg.adv_service.UUID_Type==UUID_TYPE_16&&cfg.adv_service.Service_UUID_16==0xfff0);
    active=profile->start(params);return active;
}
bool bt_weave_profile_restore(Bt* b){assert(b==&bt&&reserved);stops++;
    if(active){active->config->stop(active);active=NULL;}
    if(fail_restore)return false;
    reserved=false;return true;
}
size_t strlcpy(char* d,const char* s,size_t cap){size_t n=strlen(s);if(cap){size_t k=n<cap-1?n:cap-1;memcpy(d,s,k);d[k]=0;}return n;}
const uint8_t* furi_hal_version_get_ble_mac(void){static uint8_t m[6]={1};return m;}
const char* furi_hal_version_get_ble_local_device_name_ptr(void){return "Flipper";}
GapSvcEventHandler* ble_event_dispatcher_register_svc_handler(BleSvcEventHandlerCb cb,void* ctx){assert(!handler.cb);handler.cb=cb;handler.ctx=ctx;return &handler;}
void ble_event_dispatcher_unregister_svc_handler(GapSvcEventHandler* h){assert(h==&handler&&h->cb);memset(h,0,sizeof(*h));}
bool ble_gatt_service_add(uint8_t type,const Service_UUID_t* uuid,uint8_t service,uint8_t records_,uint16_t* handle){assert(type==UUID_TYPE_16&&uuid->uuid16==0xfff0&&service==1&&records_==13&&!created);created=1;chars=0;*handle=1;return true;}
bool ble_gatt_service_delete(uint16_t handle){assert(handle==1&&created);created=0;return true;}
tBleStatus aci_gatt_add_char(uint16_t svc,uint8_t type,const Char_UUID_t* uuid,uint16_t max,uint8_t props,uint8_t auth,uint8_t evt,uint8_t key,uint8_t variable,uint16_t* handle){
    assert(svc==1&&type==UUID_TYPE_16&&uuid->uuid16==(uint16_t)(0xfff1+chars)&&max<=20&&auth==3&&evt==1&&key==16&&variable==1);
    assert(props==(chars==0?CHAR_PROP_WRITE:chars==1?CHAR_PROP_INDICATE:CHAR_PROP_READ));
    if(++chars==fail_char)return 1;
    *handle=(uint16_t)(10*chars);return 0;
}
tBleStatus aci_gatt_update_char_value(uint16_t svc,uint16_t ch,uint8_t off,uint8_t n,const uint8_t* data){assert(svc==1&&ch==30&&!off&&n==2&&data);updates++;return 0;}
tBleStatus aci_gatt_update_char_value_ext(uint16_t conn,uint16_t svc,uint16_t ch,uint8_t typ,uint16_t n,uint16_t off,uint8_t len,uint8_t* data){assert(conn==42&&svc==1&&ch==20&&typ==2&&n==len&&!off&&data);updates++;return 0;}
static void peer_write(uint16_t conn,uint16_t handle,uint16_t off,uint16_t n){
    uint8_t buf[64]={0};hci_uart_pckt* u=(void*)buf;hci_event_pckt* h=(void*)u->data;h->evt=255;
    evt_blecore_aci* a=(void*)h->data;a->ecode=12;aci_gatt_attribute_modified_event_rp0* e=(void*)a->data;
    e->Connection_Handle=conn;e->Attr_Handle=handle;e->Offset=off;e->Attr_Data_Length=n;
    for(size_t i=0;i<n;i++)e->Attr_Data[i]=(uint8_t)i;
    assert(handler.cb);handler.cb(buf,handler.ctx);
}
int main(void){
    int owner,other;WeaveBleState state;size_t n;uint8_t index,buf[20],value[]={1,2};
    WeaveBleConfig c={.service={.size=2,.bytes={0xf0,0xff}},.count=3,.characteristics={
        {.uuid={.size=2,.bytes={0xf1,0xff}},.properties=WEAVE_BLE_WRITE,.max_size=20},
        {.uuid={.size=2,.bytes={0xf2,0xff}},.properties=WEAVE_BLE_INDICATE,.max_size=20},
        {.uuid={.size=2,.bytes={0xf3,0xff}},.properties=WEAVE_BLE_READ,.max_size=2}}};
    weave_ble_broker_init();assert(weave_ble_config_valid(&c));
    WeaveBleConfig bad=c;bad.characteristics[1].uuid=bad.characteristics[0].uuid;assert(!weave_ble_config_valid(&bad));
    bad=c;bad.characteristics[1].properties=WEAVE_BLE_NOTIFY|WEAVE_BLE_READ;assert(!weave_ble_config_valid(&bad));
    bad=c;bad.characteristics[0].max_size=21;assert(!weave_ble_config_valid(&bad));
    consent=false;assert(weave_ble_acquire(&owner,&c)==WeaveBleDenied&&!records);consent=true;
    available=false;assert(weave_ble_acquire(&owner,&c)==WeaveBleUnavailable&&!records);available=true;
    native_busy=true;assert(weave_ble_acquire(&owner,&c)==WeaveBleBusy&&!records&&!stops);native_busy=false;
    fail_char=2;assert(weave_ble_acquire(&owner,&c)==WeaveBleUnavailable&&!records&&!created&&!handler.cb);fail_char=0;
    assert(weave_ble_acquire(&owner,&c)==WeaveBleOk&&records==1&&created&&handler.cb);
    assert(weave_ble_acquire(&other,&c)==WeaveBleBusy&&records==1);
    assert(weave_ble_state(&owner,&state)==WeaveBleOk&&!state.connected);
    assert(weave_ble_update(&owner,0,2,value,2)==WeaveBleOk);
    peer_write(42,11,0,2);assert(weave_ble_receive(&owner,state.connection,&index,buf,sizeof(buf),&n)==WeaveBleDisconnected);
    weave_ble_profile_gap((GapEvent){.type=GapEventTypeConnected,.data.connection_handle=42});
    assert(weave_ble_state(&owner,&state)==WeaveBleOk&&state.connected);uint32_t gen=state.connection;
    peer_write(41,11,0,2);assert(weave_ble_receive(&owner,gen,&index,buf,sizeof(buf),&n)==WeaveBleNoData);
    peer_write(42,11,0,20);assert(weave_ble_receive(&owner,gen,&index,buf,1,&n)==WeaveBleInvalid&&!n);
    assert(weave_ble_receive(&owner,gen,&index,buf,sizeof(buf),&n)==WeaveBleOk&&index==0&&n==20&&buf[19]==19);
    for(int i=0;i<5;i++)peer_write(42,11,0,1);
    assert(weave_ble_receive(&owner,gen,&index,buf,sizeof(buf),&n)==WeaveBleOverflow);
    assert(weave_ble_receive(&owner,gen,&index,buf,sizeof(buf),&n)==WeaveBleNoData);
    peer_write(42,11,1,1);assert(weave_ble_receive(&owner,gen,&index,buf,sizeof(buf),&n)==WeaveBleOverflow);
    app="beta";assert(weave_ble_update(&owner,gen,1,value,2)==WeaveBleNotOwner);assert(weave_ble_release(&owner)==WeaveBleNotOwner);app="alpha";
    assert(weave_ble_update(&owner,gen,1,value,2)==WeaveBleOk&&updates==2);
    peer_write(42,11,0,3);weave_ble_profile_gap((GapEvent){.type=GapEventTypeDisconnected});
    weave_ble_profile_gap((GapEvent){.type=GapEventTypeConnected,.data.connection_handle=42});
    assert(weave_ble_receive(&owner,gen,&index,buf,sizeof(buf),&n)==WeaveBleDisconnected);
    assert(weave_ble_state(&owner,&state)==WeaveBleOk&&state.connection!=gen);
    assert(weave_ble_receive(&owner,state.connection,&index,buf,sizeof(buf),&n)==WeaveBleNoData);
    assert(weave_ble_release_app("beta")&&records==1);fail_restore=true;
    assert(!weave_ble_release_app("alpha")&&records==1&&!created&&!handler.cb);
    assert(weave_ble_state(&owner,&state)==WeaveBleNotOwner);fail_restore=false;
    weave_ble_reap_orphan();assert(!records&&!reserved&&!created&&!handler.cb);
    assert(weave_ble_acquire(&owner,&c)==WeaveBleOk);assert(weave_ble_release(&owner)==WeaveBleOk&&!records);
    printf("BLE GATT mock: config, owner, pairing settings, write queue, overflow, stale generation, rollback, release PASS (%d starts, %d restorations)\n",starts,stops);
    return 0;
}
