/* SDK import and runtime smoke. No persistent secret created: invalid inputs
 * exercise all keystore entry points, and discovery reads the live service. */
#include <furi.h>
#include <dialogs/dialogs.h>
#include <weave/weave_platform.h>
#include <weave/weave_keystore.h>
#include <weave/weave_permissions.h>
#include <weave/weave_usb.h>
#include <weave/weave_ble.h>
#include <weave/weave_nfc.h>
int32_t weave_smoke_app(void* arg) {
    UNUSED(arg);
    Weave* weave = furi_record_open(RECORD_WEAVE);
    const uint32_t required = WEAVE_CAP_NETWORK | WEAVE_CAP_USB_HID | WEAVE_CAP_USB_CCID |
        WEAVE_CAP_USB_COMPOSITE | WEAVE_CAP_SECURITY_V4 | WEAVE_CAP_PERMISSIONS_V2 |
        WEAVE_CAP_KEYSTORE | WEAVE_CAP_VAULT_RECOVERY | WEAVE_CAP_BLE_GATT | WEAVE_CAP_NFC_ISODEP;
    bool ok = (weave_platform_capabilities() & required) == required;
    const uint32_t state = weave_platform_state(weave);
    furi_record_close(RECORD_WEAVE);
    ok &= !((state & WEAVE_STATE_GATEWAY_USB) && (state & WEAVE_STATE_GATEWAY_BLE));
    ok &= weave_keystore_generate(NULL,NULL,WeaveKeyEd25519,NULL,0,NULL,0)==WeaveSecurityInvalid;
    ok &= weave_keystore_public(NULL,NULL,WeaveKeyEd25519,NULL,0,NULL,0)==WeaveSecurityInvalid;
    ok &= weave_keystore_sign(NULL,NULL,WeaveKeyEd25519,NULL,0,false,0,NULL,0,NULL,0)==WeaveSecurityInvalid;
    ok &= weave_keystore_derive(NULL,NULL,WeaveKeyX25519,NULL,0,NULL,0,NULL,0)==WeaveSecurityInvalid;
    ok &= weave_keystore_decrypt_oaep(NULL,NULL,WeaveKeyRsa2048Oaep,NULL,0,NULL,0,NULL,0,NULL,0,NULL)==WeaveSecurityInvalid;
    ok &= weave_keystore_delete(NULL,NULL)==WeaveSecurityInvalid;
    ok &= weave_usb_acquire(NULL,WeaveUsbModeHid)==WeaveUsbInvalid;
    ok &= weave_ble_acquire(NULL,NULL)==WeaveBleInvalid;
    ok &= weave_nfc_acquire(NULL)==WeaveNfcInvalid;
    ok &= !weave_permission_has(WeavePermissionCount);
    FURI_LOG_I("WeavePlatform", "%s, state=%lu", ok?"PASS":"FAIL",(unsigned long)state);
    DialogsApp* dialogs=furi_record_open(RECORD_DIALOGS);
    DialogMessage* message=dialog_message_alloc();
    dialog_message_set_header(message,ok?"PASS":"FAIL",64,5,AlignCenter,AlignTop);
    dialog_message_set_text(message,"Platform API imports",64,30,AlignCenter,AlignCenter);
    dialog_message_set_buttons(message,NULL,"OK",NULL);
    dialog_message_show(dialogs,message);
    dialog_message_free(message);
    furi_record_close(RECORD_DIALOGS);
    return ok?0:1;
}
