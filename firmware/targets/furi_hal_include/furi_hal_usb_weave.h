#pragma once

/* Firmware-private USB device driver used by the Weave FAP USB lease broker.
 * No application callbacks execute in USB IRQ context. All I/O is polled.
 */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <furi_hal_usb.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriHalWeaveUsbHid = 0,
    FuriHalWeaveUsbCcid = 1,
} FuriHalWeaveUsbChannel;

#define FURI_HAL_WEAVE_USB_HID_SIZE 64U
#define FURI_HAL_WEAVE_USB_CCID_SIZE 271U
#define FURI_HAL_WEAVE_USB_HID_DESCRIPTOR_MAX 192U

extern FuriHalUsbInterface usb_weave_hid;
extern FuriHalUsbInterface usb_weave_ccid;
extern FuriHalUsbInterface usb_weave_composite;

/* Configure a bounded HID report descriptor before switching the USB mode.
 * NULL selects the built-in CTAP/FIDO report descriptor (64-byte reports).
 * Only callable by the firmware broker while no Weave lease exists. */
bool furi_hal_weave_usb_configure_hid(
    const uint8_t* descriptor, size_t size, uint8_t packet_size);
bool furi_hal_weave_usb_notify_card(bool present);
bool furi_hal_weave_usb_connected(void);
/* Returns false if no frame available or capacity is too small. */
bool furi_hal_weave_usb_receive(
    FuriHalWeaveUsbChannel channel, uint8_t* data, size_t capacity, size_t* size);
/* Queued transfer; false when busy, disconnected or invalid frame length. */
bool furi_hal_weave_usb_transmit(
    FuriHalWeaveUsbChannel channel, const uint8_t* data, size_t size);

#ifdef __cplusplus
}
#endif
