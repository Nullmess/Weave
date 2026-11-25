#pragma once

#include "usb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriHalUsbInterface FuriHalUsbInterface;

struct FuriHalUsbInterface {
    void (*init)(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx);
    void (*deinit)(usbd_device* dev);
    void (*wakeup)(usbd_device* dev);
    void (*suspend)(usbd_device* dev);

    struct usb_device_descriptor* dev_descr;

    void* str_manuf_descr;
    void* str_prod_descr;
    void* str_serial_descr;

    void* cfg_descr;
};

extern FuriHalUsbInterface usb_cdc_single;
extern FuriHalUsbInterface usb_cdc_dual;
extern FuriHalUsbInterface usb_hid;
extern FuriHalUsbInterface usb_hid_u2f;

typedef enum {
    FuriHalUsbStateEventReset,
    FuriHalUsbStateEventWakeup,
    FuriHalUsbStateEventSuspend,
    FuriHalUsbStateEventDescriptorRequest,
} FuriHalUsbStateEvent;

typedef void (*FuriHalUsbStateCallback)(FuriHalUsbStateEvent state, void* context);

void furi_hal_usb_init(void);

/* Retour true si changement lancé, false si verrouillé */
bool furi_hal_usb_set_config(FuriHalUsbInterface* new_if, void* ctx);

/* Atomically switch mode and lock it in the USB worker. The expected mode
 * protects against a competing USB client between read and acquisition.
 * Returns false if USB is locked or no longer in the expected mode. */
bool furi_hal_usb_acquire_config(
    FuriHalUsbInterface* expected, FuriHalUsbInterface* requested, void* ctx);
/* Atomically restore the previous mode and unlock; only succeeds if the
 * currently active mode is the one this lease installed. */
bool furi_hal_usb_release_config(
    FuriHalUsbInterface* owned, FuriHalUsbInterface* previous);

FuriHalUsbInterface* furi_hal_usb_get_config(void);

void furi_hal_usb_lock(void);

void furi_hal_usb_unlock(void);

bool furi_hal_usb_is_locked(void);

void furi_hal_usb_disable(void);

void furi_hal_usb_enable(void);

void furi_hal_usb_set_state_callback(FuriHalUsbStateCallback cb, void* ctx);

void furi_hal_usb_reinit(void);

#ifdef __cplusplus
}
#endif
