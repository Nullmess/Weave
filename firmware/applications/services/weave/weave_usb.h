/** Weave USB device API for FAPs (v1, Weave 1.0.2).
 *
 * USB is one exclusive physical device. Acquire an explicit lease before using
 * HID (CTAP-sized 64 byte reports), CCID (raw CCID message frames), or both.
 * An active USB RPC session will cause BUSY, not be disconnected forcibly.
 * USB RPC is unavailable while a lease is active; BLE RPC remains independent.
 * A FAP must close its lease before unloading, including on every error path.
 *
 * This is a transport API, not a FIDO2, PIV, smartcard, or security API.
 * CCID commands, ATR, slot state, APDU status, and CTAP are handled by the FAP.
 * Short APDUs only (261-byte payload, 10-byte CCID header).
 * Never use the pointer passed as `owner` as an authentication credential.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WEAVE_USB_API_VERSION 1U
#define WEAVE_USB_HID_REPORT_SIZE 64U
#define WEAVE_USB_CCID_FRAME_MAX 271U

typedef enum {
    WeaveUsbModeHid = 1,
    WeaveUsbModeCcid = 2,
    WeaveUsbModeHidCcid = 3,
} WeaveUsbMode;

typedef enum {
    WeaveUsbChannelHid = 0,
    WeaveUsbChannelCcid = 1,
} WeaveUsbChannel;

typedef enum {
    WeaveUsbOk = 0,
    WeaveUsbBusy,
    WeaveUsbInvalid,
    WeaveUsbNotOwner,
    WeaveUsbDisconnected,
    WeaveUsbNoData,
} WeaveUsbResult;

/* Optional HID configuration; descriptor memory is copied into firmware.
 * For a custom report descriptor, report_size is the maximum report length
 * (1..64). NULL uses the built-in 64-byte CTAP-compatible HID descriptor.
 * Custom HID works in HID and HID+CCID modes. */
WeaveUsbResult weave_usb_acquire_ex(
    void* owner, WeaveUsbMode mode, const uint8_t* hid_report_descriptor,
    size_t hid_descriptor_size, uint8_t report_size);

/* Acquire exclusive device mode. The owner must be a stable, non-NULL pointer.
 * While active, normal USB CDC/RPC will temporarily disconnect. If the USB
 * interface is locked by the active RPC service, returns WeaveUsbBusy.
 * The existing USB mode is restored on weave_usb_release(). */
WeaveUsbResult weave_usb_acquire(void* owner, WeaveUsbMode mode);
WeaveUsbResult weave_usb_release(void* owner);
/* Channel frames are buffered in the firmware, never in a callback into a FAP.
 * Pass a buffer of 64 bytes for HID or 271 bytes for CCID; the actual frame
 * length is returned in *size. HID reports can be shorter for custom HID.
 * Poll from the FAP worker; returns NoData when there is nothing to read. */
WeaveUsbResult weave_usb_receive(
    void* owner, WeaveUsbChannel channel, uint8_t* buffer, size_t capacity, size_t* size);
/* Nonblocking. Busy indicates another frame still being transmitted. */
WeaveUsbResult weave_usb_transmit(
    void* owner, WeaveUsbChannel channel, const uint8_t* buffer, size_t size);
/* Notify the host that the emulated CCID slot is present/absent.
 * The FAP remains responsible for CCID SlotStatus, PowerOn/ATR, APDU and
 * protocol negotiation; this function only emits NotifySlotChange. */
WeaveUsbResult weave_usb_ccid_notify(void* owner, bool present);
/* USB host configured and port active. Independent of Bluetooth connectivity. */
bool weave_usb_connected(void* owner);
/* True only when a CDC USB mode is active and no Weave USB lease exists. */
bool weave_usb_cdc_available(void);

#ifdef __cplusplus
}
#endif
