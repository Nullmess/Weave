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
