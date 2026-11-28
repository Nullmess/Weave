#!/usr/bin/env python3
"""Actual BLE queue and broker + radio profile compiled with strict host doubles.
Does not validate the STM32WB radio stack, RF traffic or actual pairing."""
from pathlib import Path
import tempfile, subprocess
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
SRC=ROOT/'applications/services/weave'
HEADERS={
'furi.h':r'''#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#define furi_check(x) assert(x)
#define FuriWaitForever 0
#define FuriMutexTypeNormal 0
#define FURI_CRITICAL_ENTER() do {} while(0)
#define FURI_CRITICAL_EXIT() do {} while(0)
typedef struct { int held; } FuriMutex;
FuriMutex* furi_mutex_alloc(int);
int furi_mutex_acquire(FuriMutex*,int);
int furi_mutex_release(FuriMutex*);
bool furi_record_exists(const char*);
void* furi_record_open(const char*);
void furi_record_close(const char*);
size_t strlcpy(char*,const char*,size_t);
''',
'gap.h':r'''#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef enum {GapEventTypeConnected,GapEventTypeDisconnected,GapEventTypeUpdateMTU} GapEventType;
typedef struct {GapEventType type; union {uint16_t connection_handle;uint16_t max_packet_size;} data;} GapEvent;
typedef enum {GapPairingNone,GapPairingPinCodeShow,GapPairingPinCodeVerifyYesNo} GapPairing;
typedef struct {struct {uint8_t UUID_Type;uint16_t Service_UUID_16;uint8_t Service_UUID_128[16];} adv_service;
    bool bonding_mode;GapPairing pairing_method;struct {uint16_t conn_int_min,conn_int_max;} conn_param;
    uint8_t mac_address[6];char adv_name[16];} GapConfig;
const uint8_t* furi_hal_version_get_ble_mac(void);
const char* furi_hal_version_get_ble_local_device_name_ptr(void);
''',
'furi_ble/profile_interface.h':r'''#pragma once
#include "gap.h"
typedef void* FuriHalBleProfileParams;
typedef struct FuriHalBleProfileTemplate FuriHalBleProfileTemplate;
typedef struct {const FuriHalBleProfileTemplate* config;} FuriHalBleProfileBase;
struct FuriHalBleProfileTemplate {FuriHalBleProfileBase*(*start)(FuriHalBleProfileParams);
    void(*stop)(FuriHalBleProfileBase*);void(*get_gap_config)(GapConfig*,FuriHalBleProfileParams);};
''',
'bt/bt_service/bt.h':r'''#pragma once
#include <furi_ble/profile_interface.h>
#define RECORD_BT "bt"
typedef struct Bt Bt;
void bt_disconnect(Bt*);
''',
'furi_hal_bt.h':'#pragma once\n#include <stdbool.h>\nbool furi_hal_bt_is_gatt_gap_supported(void);\n',
'furi_ble/event_dispatcher.h':r'''#pragma once
typedef enum {BleEventNotAck,BleEventAckFlowEnable,BleEventAckFlowDisable} BleEventAckStatus;
typedef BleEventAckStatus (*BleSvcEventHandlerCb)(void*,void*);
typedef struct GapEventHandler GapSvcEventHandler;
GapSvcEventHandler* ble_event_dispatcher_register_svc_handler(BleSvcEventHandlerCb,void*);
void ble_event_dispatcher_unregister_svc_handler(GapSvcEventHandler*);
''',
'ble/ble.h':r'''#pragma once
#include <stdint.h>
#define UUID_TYPE_16 1
#define UUID_TYPE_128 2
#define PRIMARY_SERVICE 1
#define CHAR_PROP_READ 2
#define CHAR_PROP_WRITE 8
#define CHAR_PROP_WRITE_WITHOUT_RESP 4
#define CHAR_PROP_NOTIFY 16
#define CHAR_PROP_INDICATE 32
#define ATTR_PERMISSION_AUTHEN_READ 1
#define ATTR_PERMISSION_AUTHEN_WRITE 2
#define GATT_NOTIFY_ATTRIBUTE_WRITE 1
#define CHAR_VALUE_LEN_VARIABLE 1
#define BLE_STATUS_SUCCESS 0
#define HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE 255
#define ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE 12
typedef uint8_t tBleStatus;
typedef union {uint16_t uuid16;uint8_t uuid128[16];} Service_UUID_t;
typedef Service_UUID_t Char_UUID_t;
typedef struct __attribute__((packed)) {uint8_t type;uint8_t data[];} hci_uart_pckt;
typedef struct __attribute__((packed)) {uint8_t evt,plen;uint8_t data[];} hci_event_pckt;
typedef struct __attribute__((packed)) {uint16_t ecode;uint8_t data[];} evt_blecore_aci;
typedef struct __attribute__((packed)) {uint16_t Connection_Handle,Attr_Handle,Offset,Attr_Data_Length;uint8_t Attr_Data[];} aci_gatt_attribute_modified_event_rp0;
tBleStatus aci_gatt_add_char(uint16_t,uint8_t,const Char_UUID_t*,uint16_t,uint8_t,uint8_t,uint8_t,uint8_t,uint8_t,uint16_t*);
tBleStatus aci_gatt_update_char_value(uint16_t,uint16_t,uint8_t,uint8_t,const uint8_t*);
tBleStatus aci_gatt_update_char_value_ext(uint16_t,uint16_t,uint16_t,uint8_t,uint16_t,uint16_t,uint8_t,uint8_t*);
''',
'furi_ble/gatt.h':r'''#pragma once
#include <stdbool.h>
#include <ble/ble.h>
bool ble_gatt_service_add(uint8_t,const Service_UUID_t*,uint8_t,uint8_t,uint16_t*);
bool ble_gatt_service_delete(uint16_t);
''',
}
with tempfile.TemporaryDirectory(prefix='weave-ble-') as temp:
    base=Path(temp)
    for name,contents in HEADERS.items():
        p=base/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_text(contents)
    exe=base/'test'
    subprocess.run(['gcc','-DSRV_WEAVE','-std=c11','-Wall','-Wextra','-Werror','-O1','-g','-no-pie',
        '-fsanitize=address,undefined','-fno-sanitize-recover=all','-I',str(base),'-I',str(SRC),
        '-I',str(ROOT/'applications/services'),str(SRC/'weave_ble.c'),str(SRC/'weave_ble_queue.c'),
        str(ROOT/'targets/f7/ble_glue/profiles/weave_profile.c'),str(HERE/'test_ble_transport.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
