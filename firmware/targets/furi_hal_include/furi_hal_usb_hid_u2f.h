#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#define HID_U2F_PACKET_LEN 64

typedef enum {
    HidU2fDisconnected,
    HidU2fConnected,
    HidU2fRequest,
} HidU2fEvent;

typedef void (*HidU2fCallback)(HidU2fEvent ev, void* context);

bool furi_hal_hid_u2f_is_connected(void);

void furi_hal_hid_u2f_set_callback(HidU2fCallback cb, void* ctx);

uint32_t furi_hal_hid_u2f_get_request(uint8_t* data);

void furi_hal_hid_u2f_send_response(uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif
