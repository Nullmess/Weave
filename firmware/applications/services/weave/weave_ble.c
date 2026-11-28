#include "weave_ble.h"
#include "weave_ble_profile.h"
#include "weave_ble_queue.h"
#include "weave_permissions.h"
#include "weave_internal.h"
#include <bt/bt_service/bt.h>
#include <furi_hal_bt.h>
#include <furi.h>
#include <string.h>
extern FuriHalBleProfileBase* bt_weave_profile_start(Bt* bt,
    const FuriHalBleProfileTemplate* profile, FuriHalBleProfileParams params, bool* reserved);
extern bool bt_weave_profile_restore(Bt* bt);
static FuriMutex* mutex;
static void* lease_owner;
static char appid[64];
static bool orphaned;
static Bt* bt;
static FuriHalBleProfileBase* instance;
void weave_ble_broker_init(void) {
    if(!mutex) mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(mutex);
}
static bool owns(void* owner) {
    const char* id = weave_caller_appid();
    return !orphaned && lease_owner && lease_owner == owner && id && !strcmp(id, appid);
}
static bool restore_locked(void) {
    if(!bt) return true;
    orphaned = true;
    /* Stop serving the application before restoring RPC. On failure keep
     * the lease quarantined; later attempts can retry restoration. */
    bt_disconnect(bt);
    if(!bt_weave_profile_restore(bt)) { instance = NULL; return false; }
    furi_record_close(RECORD_BT);
    bt = NULL; instance = NULL; lease_owner = NULL; orphaned = false;
    memset(appid, 0, sizeof(appid));
    return true;
}
WeaveBleResult weave_ble_acquire(void* owner, const WeaveBleConfig* config) {
    if(!mutex || !owner || !weave_ble_config_valid(config)) return WeaveBleInvalid;
    const char* id = weave_caller_appid();
    if(!id) return WeaveBleDenied;
    if(!furi_record_exists(RECORD_BT) || !furi_hal_bt_is_gatt_gap_supported())
        return WeaveBleUnavailable;
    if(!weave_permission_check(WeavePermissionBle)) return WeaveBleDenied;
    WeaveBleConfig copy = *config;
    if(!weave_ble_config_valid(&copy)) return WeaveBleInvalid;
    furi_mutex_acquire(mutex, FuriWaitForever);
    if(orphaned) (void)restore_locked();
    WeaveBleResult result = WeaveBleBusy;
    if(!bt) {
        bt = furi_record_open(RECORD_BT);
        lease_owner = owner; strcpy(appid, id);
        bool reserved = false;
        instance = bt_weave_profile_start(bt, weave_ble_profile, &copy, &reserved);
        if(instance) result = WeaveBleOk;
        else if(reserved) { (void)restore_locked(); result = WeaveBleUnavailable; }
        else {
            furi_record_close(RECORD_BT);
            bt = NULL; lease_owner = NULL; memset(appid, 0, sizeof(appid));
        }
    }
    furi_mutex_release(mutex);
    return result;
}
WeaveBleResult weave_ble_release(void* owner) {
    if(!mutex || !owner) return WeaveBleInvalid;
    furi_mutex_acquire(mutex, FuriWaitForever);
    WeaveBleResult result = !owns(owner) ? WeaveBleNotOwner :
        restore_locked() ? WeaveBleOk : WeaveBleBusy;
    furi_mutex_release(mutex);
    return result;
}
bool weave_ble_release_app(const char* id) {
    if(!mutex || !id) return true;
    furi_mutex_acquire(mutex, FuriWaitForever);
    bool result = !bt || strcmp(id, appid) || restore_locked();
    furi_mutex_release(mutex);
    return result;
}
void weave_ble_reap_orphan(void) {
    if(!mutex) return;
    furi_mutex_acquire(mutex, FuriWaitForever);
    if(orphaned) (void)restore_locked();
    furi_mutex_release(mutex);
}
WeaveBleResult weave_ble_state(void* owner, WeaveBleState* state) {
    if(!mutex || !owner || !state) return WeaveBleInvalid;
    *state = (WeaveBleState){0};
    furi_mutex_acquire(mutex, FuriWaitForever);
    WeaveBleResult result = WeaveBleNotOwner;
    if(owns(owner) && instance) { *state = weave_ble_profile_state(); result = WeaveBleOk; }
    furi_mutex_release(mutex);
    return result;
}
WeaveBleResult weave_ble_receive(void* owner, uint32_t connection, uint8_t* characteristic,
    uint8_t* data, size_t capacity, size_t* size) {
    if(!mutex || !owner || !size || !data || !characteristic) return WeaveBleInvalid;
    *size = 0;
    furi_mutex_acquire(mutex, FuriWaitForever);
    WeaveBleResult result = owns(owner) && instance ?
        weave_ble_profile_receive(connection, characteristic, data, capacity, size) : WeaveBleNotOwner;
    furi_mutex_release(mutex);
    return result;
}
WeaveBleResult weave_ble_update(void* owner, uint32_t connection, uint8_t characteristic,
    const uint8_t* data, size_t size) {
    if(!mutex || !owner || !data || !size) return WeaveBleInvalid;
    furi_mutex_acquire(mutex, FuriWaitForever);
    WeaveBleResult result = owns(owner) && instance ?
        weave_ble_profile_update(connection, characteristic, data, size) : WeaveBleNotOwner;
    furi_mutex_release(mutex);
    return result;
}
