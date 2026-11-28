#include "weave_permissions.h"
#include "weave_internal.h"
#include <furi.h>
#include <dialogs/dialogs.h>
#include <string.h>
#include <stdio.h>

#define WEAVE_GRANTS_MAX 16
#define WEAVE_APP_ID_MAX 63

typedef struct {
    char appid[WEAVE_APP_ID_MAX + 1];
    uint8_t grants;
} WeaveGrant;
static WeaveGrant* grants;
static FuriMutex* grants_mutex;
/* Serialize approvals so two threads cannot show duplicate consent dialogs. */
static FuriMutex* approval_mutex;

void weave_permissions_init(void) {
    if(!grants_mutex) grants_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    if(!approval_mutex) approval_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(grants_mutex && approval_mutex);
}

const char* weave_caller_appid(void) {
    const FuriThreadId id = furi_thread_get_current_id();
    if(!id) return NULL;
    const char* name = furi_thread_get_appid(id);
    if(!name) return NULL;
    const size_t n = strnlen(name, WEAVE_APP_ID_MAX + 1);
    if(!n || n > WEAVE_APP_ID_MAX || !strcmp(name, "unknown") ||
       !strcmp(name, "driver") || !strcmp(name, "system")) return NULL;
    for(size_t i = 0; i < n; ++i) {
        const char c = name[i];
        if(!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
             (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.')) return NULL;
    }
    return name;
}

static bool granted_locked(const char* appid, WeavePermission capability) {
    if(!grants) return false;
    for(size_t i = 0; i < WEAVE_GRANTS_MAX; ++i) {
        if(grants[i].appid[0] && !strcmp(grants[i].appid, appid))
            return (grants[i].grants & (1U << capability)) != 0;
    }
    return false;
}

bool weave_permission_has(WeavePermission capability) {
    if(!grants_mutex || (unsigned)capability >= WeavePermissionCount) return false;
    const char* appid = weave_caller_appid();
    if(!appid) return false;
    furi_mutex_acquire(grants_mutex, FuriWaitForever);
    bool result = granted_locked(appid, capability);
    furi_mutex_release(grants_mutex);
    return result;
}

bool weave_permission_check(WeavePermission capability) {
    if(!grants_mutex || !approval_mutex ||
       (unsigned)capability >= WeavePermissionCount) return false;
    const char* appid = weave_caller_appid();
    if(!appid) return false;
    if(weave_permission_has(capability)) return true;
    furi_mutex_acquire(approval_mutex, FuriWaitForever);
    if(weave_permission_has(capability)) {
        furi_mutex_release(approval_mutex);
        return true;
    }
    static const char* const captions[WeavePermissionCount] = {"HTTP requests", "Network sockets", "File uploads", "Secret vault", "USB device", "Key storage", "BLE GATT server", "NFC ISO-DEP"};
    char body[96];
    snprintf(body, sizeof(body), "App: %.32s\nAccess: %s\nUntil reboot", appid, captions[capability]);
    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, "Weave permission", 64, 2, AlignCenter, AlignTop);
    dialog_message_set_text(message, body, 64, 29, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, "Deny", NULL, "Allow");
    bool approved = dialog_message_show(dialogs, message) == DialogMessageButtonRight;
    dialog_message_free(message);
    furi_record_close(RECORD_DIALOGS);
    if(approved) {
        furi_mutex_acquire(grants_mutex, FuriWaitForever);
        if(!grants) {
            const int32_t lock = furi_kernel_lock();
            if(memmgr_heap_get_max_free_block() >= WEAVE_GRANTS_MAX * sizeof(WeaveGrant) + 64U) {
                grants = malloc(WEAVE_GRANTS_MAX * sizeof(WeaveGrant));
                if(grants) memset(grants, 0, WEAVE_GRANTS_MAX * sizeof(WeaveGrant));
            }
            if(lock == 0) furi_kernel_unlock();
        }
        WeaveGrant* entry = NULL;
        for(size_t i = 0; grants && i < WEAVE_GRANTS_MAX; ++i) {
            if(grants[i].appid[0] && !strcmp(grants[i].appid, appid)) {
                entry = &grants[i]; break;
            }
            if(!grants[i].appid[0] && !entry) entry = &grants[i];
        }
        if(entry) {
            if(!entry->appid[0]) strcpy(entry->appid, appid);
            entry->grants |= (uint8_t)(1U << capability);
        } else approved = false;
        furi_mutex_release(grants_mutex);
    }
    furi_mutex_release(approval_mutex);
    return approved;
}

void weave_permission_revoke_self(void) {
    if(!grants_mutex || !approval_mutex) return;
    const char* appid = weave_caller_appid();
    if(!appid) return;
    furi_mutex_acquire(approval_mutex, FuriWaitForever);
    furi_mutex_acquire(grants_mutex, FuriWaitForever);
    bool any = false;
    for(size_t i = 0; grants && i < WEAVE_GRANTS_MAX; ++i) {
        if(grants[i].appid[0] && !strcmp(grants[i].appid, appid))
            memset(&grants[i], 0, sizeof(grants[i]));
        if(grants[i].appid[0]) any = true;
    }
    if(grants && !any) {
        free(grants);
        grants = NULL;
    }
    furi_mutex_release(grants_mutex);
    furi_mutex_release(approval_mutex);
    /* Revoke the current physical lease as well as future grants. */
    (void)weave_usb_release_app(appid);
    (void)weave_ble_release_app(appid);
    (void)weave_nfc_release_app(appid);
}
