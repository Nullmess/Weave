#include "weave_usb.h"
#include "weave_permissions.h"
#include "weave_internal.h"
#include <string.h>
#include <furi.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_weave.h>

/* The broker owns the device mode, not any FAP callbacks or protocol secrets.
 * Initialization is done by Weave's STARTUP service, before applications run.
 */
/* Internal HAL hooks. Do not expose in the FAP SDK. */
extern bool weave_usb_buffers_prepare(void);
extern void weave_usb_buffers_release(void);
static FuriMutex* usb_mutex;
static void* usb_owner;
static char usb_appid[64];
static bool usb_orphaned;
static WeaveUsbMode usb_mode;
static uint8_t usb_hid_packet_size;
static FuriHalUsbInterface* usb_previous;

void weave_usb_broker_init(void) {
    if(!usb_mutex) usb_mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    furi_check(usb_mutex);
}

static FuriHalUsbInterface* weave_usb_interface(WeaveUsbMode mode) {
    switch(mode) {
    case WeaveUsbModeHid: return &usb_weave_hid;
    case WeaveUsbModeCcid: return &usb_weave_ccid;
    case WeaveUsbModeHidCcid: return &usb_weave_composite;
    default: return NULL;
    }
}

static bool weave_usb_has_channel(WeaveUsbMode mode, WeaveUsbChannel channel) {
    return (channel == WeaveUsbChannelHid &&
            (mode == WeaveUsbModeHid || mode == WeaveUsbModeHidCcid)) ||
           (channel == WeaveUsbChannelCcid &&
            (mode == WeaveUsbModeCcid || mode == WeaveUsbModeHidCcid));
}

/* Called with usb_mutex held. Pointer tokens alone do not identify an app. */
static bool usb_is_owner(void* owner, const char* appid) {
    return !usb_orphaned && usb_owner && usb_owner == owner && appid && !strcmp(usb_appid, appid);
}

static WeaveUsbResult usb_release_locked(void) {
    if(!furi_hal_usb_release_config(weave_usb_interface(usb_mode), usb_previous))
        return WeaveUsbBusy;
    weave_usb_buffers_release();
    usb_owner = NULL;
    usb_appid[0] = 0;
    usb_orphaned = false;
    usb_mode = 0;
    usb_hid_packet_size = 0;
    usb_previous = NULL;
    return WeaveUsbOk;
}

/* The loader calls this after the application's main thread has joined.
 * No callback into FAP code; never release another application's lease. */
bool weave_usb_release_app(const char* appid) {
    if(!usb_mutex || !appid) return true;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    bool ok = true;
    if(usb_owner && !strcmp(usb_appid, appid)) {
        usb_orphaned = true;
        ok = usb_release_locked() == WeaveUsbOk;
    }
    furi_mutex_release(usb_mutex);
    return ok;
}

void weave_usb_reap_orphan(void) {
    if(!usb_mutex) return;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    if(usb_orphaned) (void)usb_release_locked();
    furi_mutex_release(usb_mutex);
}

WeaveUsbResult weave_usb_acquire_ex(
    void* owner, WeaveUsbMode mode, const uint8_t* hid_descriptor,
    size_t descriptor_size, uint8_t report_size) {
    FuriHalUsbInterface* iface = weave_usb_interface(mode);
    if(!usb_mutex || !owner || !iface ||
       (hid_descriptor == NULL && descriptor_size != 0) ||
       (hid_descriptor != NULL && (mode == WeaveUsbModeCcid ||
                                  !descriptor_size ||
                                  descriptor_size > FURI_HAL_WEAVE_USB_HID_DESCRIPTOR_MAX ||
                                  !report_size ||
                                  report_size > WEAVE_USB_HID_REPORT_SIZE)))
        return WeaveUsbInvalid;
    const char* appid = weave_caller_appid();
    if(!appid) return WeaveUsbDenied;
    /* Do not open a consent dialog while holding the USB mutex. */
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    if(usb_orphaned) (void)usb_release_locked();
    const bool occupied = usb_owner && !usb_is_owner(owner, appid);
    furi_mutex_release(usb_mutex);
    if(occupied) return WeaveUsbBusy;
    if(!weave_permission_check(WeavePermissionUsb)) return WeaveUsbDenied;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    WeaveUsbResult result = WeaveUsbBusy;
    if(usb_is_owner(owner, appid)) {
        /* Idempotent acquisition in the same mode, never silently switch. */
        result = usb_mode == mode ? WeaveUsbOk : WeaveUsbBusy;
    } else if(!usb_owner) {
        FuriHalUsbInterface* previous = furi_hal_usb_get_config();
        bool descriptor_ok = mode == WeaveUsbModeCcid ||
            furi_hal_weave_usb_configure_hid(
                hid_descriptor, descriptor_size, report_size);
        const bool buffers_ready = descriptor_ok && previous && weave_usb_buffers_prepare();
        if(buffers_ready && furi_hal_usb_acquire_config(previous, iface, NULL)) {
            /* HAL atomically locks the USB interface during mode switching. */
            usb_previous = previous;
            usb_mode = mode;
            usb_hid_packet_size = hid_descriptor ? report_size : WEAVE_USB_HID_REPORT_SIZE;
            usb_owner = owner;
            strcpy(usb_appid, appid);
            result = WeaveUsbOk;
        } else if(buffers_ready) {
            weave_usb_buffers_release();
        }
    }
    furi_mutex_release(usb_mutex);
    return result;
}

WeaveUsbResult weave_usb_acquire(void* owner, WeaveUsbMode mode) {
    return weave_usb_acquire_ex(owner, mode, NULL, 0, 0);
}

WeaveUsbResult weave_usb_release(void* owner) {
    if(!usb_mutex || !owner) return WeaveUsbInvalid;
    const char* appid = weave_caller_appid();
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    const WeaveUsbResult result = usb_is_owner(owner, appid) ?
        usb_release_locked() : WeaveUsbNotOwner;
    furi_mutex_release(usb_mutex);
    return result;
}

WeaveUsbResult weave_usb_receive(
    void* owner, WeaveUsbChannel channel, uint8_t* buffer, size_t capacity, size_t* size) {
    if(!usb_mutex || !owner || !buffer || !size) return WeaveUsbInvalid;
    *size = 0;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    WeaveUsbResult result = WeaveUsbNotOwner;
    if(usb_is_owner(owner, weave_caller_appid())) {
        result = WeaveUsbInvalid;
        if(weave_usb_has_channel(usb_mode, channel)) {
            if(capacity < (channel == WeaveUsbChannelHid ?
                           WEAVE_USB_HID_REPORT_SIZE : WEAVE_USB_CCID_FRAME_MAX))
                result = WeaveUsbInvalid;
            else if(!furi_hal_weave_usb_connected()) result = WeaveUsbDisconnected;
            else if(furi_hal_weave_usb_receive(
                        channel == WeaveUsbChannelHid ? FuriHalWeaveUsbHid : FuriHalWeaveUsbCcid,
                        buffer, capacity, size)) result = WeaveUsbOk;
            else result = WeaveUsbNoData;
        }
    }
    furi_mutex_release(usb_mutex);
    return result;
}

WeaveUsbResult weave_usb_transmit(
    void* owner, WeaveUsbChannel channel, const uint8_t* buffer, size_t size) {
    if(!usb_mutex || !owner || !buffer || !size) return WeaveUsbInvalid;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    WeaveUsbResult result = WeaveUsbNotOwner;
    if(usb_is_owner(owner, weave_caller_appid())) {
        result = WeaveUsbInvalid;
        if(weave_usb_has_channel(usb_mode, channel)) {
            if((channel == WeaveUsbChannelHid && size > usb_hid_packet_size) ||
               (channel == WeaveUsbChannelCcid &&
                (size < 10 || size > WEAVE_USB_CCID_FRAME_MAX ||
                 ((uint32_t)buffer[1] | ((uint32_t)buffer[2] << 8) |
                  ((uint32_t)buffer[3] << 16) | ((uint32_t)buffer[4] << 24)) + 10U != size)))
                result = WeaveUsbInvalid;
            else if(!furi_hal_weave_usb_connected()) result = WeaveUsbDisconnected;
            else if(furi_hal_weave_usb_transmit(
                        channel == WeaveUsbChannelHid ? FuriHalWeaveUsbHid : FuriHalWeaveUsbCcid,
                        buffer, size)) result = WeaveUsbOk;
            else result = WeaveUsbBusy;
        }
    }
    furi_mutex_release(usb_mutex);
    return result;
}

WeaveUsbResult weave_usb_ccid_notify(void* owner, bool present) {
    if(!usb_mutex || !owner) return WeaveUsbInvalid;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    WeaveUsbResult result = WeaveUsbNotOwner;
    if(usb_is_owner(owner, weave_caller_appid())) {
        if(!weave_usb_has_channel(usb_mode, WeaveUsbChannelCcid))
            result = WeaveUsbInvalid;
        else if(!furi_hal_weave_usb_connected())
            result = WeaveUsbDisconnected;
        else
            result = furi_hal_weave_usb_notify_card(present) ? WeaveUsbOk : WeaveUsbBusy;
    }
    furi_mutex_release(usb_mutex);
    return result;
}

bool weave_usb_connected(void* owner) {
    if(!usb_mutex || !owner) return false;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    bool result = usb_is_owner(owner, weave_caller_appid()) && furi_hal_weave_usb_connected();
    furi_mutex_release(usb_mutex);
    return result;
}

bool weave_usb_cdc_available(void) {
    if(!usb_mutex) return true;
    furi_mutex_acquire(usb_mutex, FuriWaitForever);
    FuriHalUsbInterface* current = furi_hal_usb_get_config();
    bool result = usb_owner == NULL &&
        (current == &usb_cdc_single || current == &usb_cdc_dual);
    furi_mutex_release(usb_mutex);
    return result;
}
