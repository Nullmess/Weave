#include "weave_platform.h"
#include "weave_usb.h"
uint32_t weave_platform_capabilities(void) {
    return WEAVE_CAP_NETWORK | WEAVE_CAP_USB_HID | WEAVE_CAP_USB_CCID |
           WEAVE_CAP_USB_COMPOSITE | WEAVE_CAP_SECURITY_V4 | WEAVE_CAP_PERMISSIONS_V2 |
           WEAVE_CAP_KEYSTORE | WEAVE_CAP_VAULT_RECOVERY | WEAVE_CAP_BLE_GATT | WEAVE_CAP_NFC_ISODEP;
}
uint32_t weave_platform_state(Weave* weave) {
    uint32_t state = 0;
    if(weave) {
        const WeaveTransport transport = weave_get_transport(weave);
        if(transport == WeaveTransportUsb) state |= WEAVE_STATE_GATEWAY_USB;
        if(transport == WeaveTransportBle) state |= WEAVE_STATE_GATEWAY_BLE;
    }
    if(weave_usb_cdc_available()) state |= WEAVE_STATE_USB_CDC_AVAILABLE;
    return state;
}
