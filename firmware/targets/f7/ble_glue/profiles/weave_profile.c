#ifdef SRV_WEAVE
#include <weave/weave_ble_profile.h>
#include <weave/weave_ble_queue.h>
#include <furi.h>
#include <ble/ble.h>
#include <furi_ble/event_dispatcher.h>
#include <furi_ble/gatt.h>
#include <string.h>
/* Static service-owned state: CPU2 events never retain an application pointer.
 * Broker serializes start/stop/update. Queue access is short and IRQ-safe. */
typedef struct {
    FuriHalBleProfileBase base;
    WeaveBleConfig config;
    uint16_t service, handles[WEAVE_BLE_CHAR_MAX];
    GapSvcEventHandler* handler;
    bool active;
    uint16_t connection_handle;
    WeaveBleQueue rx;
} Profile;
static Profile profile;
static BleEventAckStatus on_event(void* packet, void* context) {
    (void)context;
    const hci_event_pckt* hci = (const hci_event_pckt*)(((hci_uart_pckt*)packet)->data);
    if(hci->evt != HCI_VENDOR_SPECIFIC_DEBUG_EVT_CODE) return BleEventNotAck;
    const evt_blecore_aci* aci = (const evt_blecore_aci*)hci->data;
    if(aci->ecode != ACI_GATT_ATTRIBUTE_MODIFIED_VSEVT_CODE) return BleEventNotAck;
    const aci_gatt_attribute_modified_event_rp0* event =
        (const aci_gatt_attribute_modified_event_rp0*)aci->data;
    BleEventAckStatus result = BleEventNotAck;
    FURI_CRITICAL_ENTER();
    if(profile.active && profile.rx.state.connected &&
       event->Connection_Handle == profile.connection_handle) for(uint8_t i = 0; i < profile.config.count; i++) {
        if(event->Attr_Handle == profile.handles[i] + 1U) {
            const WeaveBleCharacteristic* c = &profile.config.characteristics[i];
            if(c->properties & (WEAVE_BLE_WRITE | WEAVE_BLE_WRITE_COMMAND)) {
                const size_t n = event->Attr_Data_Length;
                /* Reject partial or oversized writes rather than concatenate. */
                if(event->Offset || n > c->max_size)
                    weave_ble_queue_push(&profile.rx, i, NULL, 0);
                else weave_ble_queue_push(&profile.rx, i, event->Attr_Data, n);
            }
            result = BleEventAckFlowEnable;
            break;
        }
    }
    FURI_CRITICAL_EXIT();
    return result;
}
static void gap_config(GapConfig* config, FuriHalBleProfileParams params) {
    const WeaveBleConfig* c = params;
    memset(config, 0, sizeof(*config));
    config->adv_service.UUID_Type = c->service.size == 2 ? UUID_TYPE_16 : UUID_TYPE_128;
    if(c->service.size == 2) config->adv_service.Service_UUID_16 =
        (uint16_t)c->service.bytes[0] | ((uint16_t)c->service.bytes[1] << 8);
    else memcpy(config->adv_service.Service_UUID_128, c->service.bytes, 16);
    config->bonding_mode = false;
    config->pairing_method = GapPairingPinCodeVerifyYesNo;
    config->conn_param.conn_int_min = 0x06;
    config->conn_param.conn_int_max = 0x24;
    memcpy(config->mac_address, furi_hal_version_get_ble_mac(), sizeof(config->mac_address));
    strlcpy(config->adv_name, furi_hal_version_get_ble_local_device_name_ptr(), sizeof(config->adv_name));
}
static FuriHalBleProfileBase* start(FuriHalBleProfileParams params) {
    const WeaveBleConfig* c = params;
    if(!weave_ble_config_valid(c)) return NULL;
    profile.base.config = weave_ble_profile;
    profile.config = *c;
    Service_UUID_t svc = {0};
    memcpy(&svc, c->service.bytes, c->service.size);
    if(!ble_gatt_service_add(c->service.size == 2 ? UUID_TYPE_16 : UUID_TYPE_128,
            &svc, PRIMARY_SERVICE, 1 + 4 * c->count, &profile.service)) return NULL;
    for(uint8_t i = 0; i < c->count; i++) {
        const WeaveBleCharacteristic* ch = &c->characteristics[i];
        Char_UUID_t uuid = {0}; memcpy(&uuid, ch->uuid.bytes, ch->uuid.size);
        uint8_t props = 0;
        if(ch->properties & WEAVE_BLE_READ) props |= CHAR_PROP_READ;
        if(ch->properties & WEAVE_BLE_WRITE) props |= CHAR_PROP_WRITE;
        if(ch->properties & WEAVE_BLE_WRITE_COMMAND) props |= CHAR_PROP_WRITE_WITHOUT_RESP;
        if(ch->properties & WEAVE_BLE_NOTIFY) props |= CHAR_PROP_NOTIFY;
        if(ch->properties & WEAVE_BLE_INDICATE) props |= CHAR_PROP_INDICATE;
        const tBleStatus status = aci_gatt_add_char(profile.service,
            ch->uuid.size == 2 ? UUID_TYPE_16 : UUID_TYPE_128, &uuid, ch->max_size, props,
            ATTR_PERMISSION_AUTHEN_READ | ATTR_PERMISSION_AUTHEN_WRITE,
            GATT_NOTIFY_ATTRIBUTE_WRITE, 16, CHAR_VALUE_LEN_VARIABLE, &profile.handles[i]);
        if(status != BLE_STATUS_SUCCESS) {
            (void)ble_gatt_service_delete(profile.service);
            memset(profile.handles, 0, sizeof(profile.handles));
            profile.service = 0;
            return NULL;
        }
    }
    profile.handler = ble_event_dispatcher_register_svc_handler(on_event, &profile);
    FURI_CRITICAL_ENTER();
    weave_ble_queue_connection(&profile.rx, false);
    profile.active = true;
    FURI_CRITICAL_EXIT();
    return &profile.base;
}
static void stop(FuriHalBleProfileBase* base) {
    furi_check(base == &profile.base);
    { FURI_CRITICAL_ENTER(); profile.active = false;
      weave_ble_queue_connection(&profile.rx, false); FURI_CRITICAL_EXIT(); }
    ble_event_dispatcher_unregister_svc_handler(profile.handler);
    (void)ble_gatt_service_delete(profile.service);
    profile.handler = NULL; profile.service = 0;
    memset(profile.handles, 0, sizeof(profile.handles));
    memset(&profile.config, 0, sizeof(profile.config));
}
static const FuriHalBleProfileTemplate callbacks = {.start=start,.stop=stop,.get_gap_config=gap_config};
const FuriHalBleProfileTemplate* const weave_ble_profile = &callbacks;
void weave_ble_profile_gap(GapEvent event) {
    if(event.type != GapEventTypeConnected && event.type != GapEventTypeDisconnected) return;
    FURI_CRITICAL_ENTER();
    if(profile.active) {
        profile.connection_handle = event.type == GapEventTypeConnected ? event.data.connection_handle : 0xFFFF;
        weave_ble_queue_connection(&profile.rx, event.type == GapEventTypeConnected);
    }
    FURI_CRITICAL_EXIT();
}
WeaveBleState weave_ble_profile_state(void) {
    FURI_CRITICAL_ENTER(); WeaveBleState result = profile.rx.state;
    if(!profile.active) result.connected = false;
    FURI_CRITICAL_EXIT(); return result;
}
WeaveBleResult weave_ble_profile_receive(uint32_t connection, uint8_t* characteristic,
    uint8_t* data, size_t capacity, size_t* size) {
    FURI_CRITICAL_ENTER();
    WeaveBleResult result = weave_ble_queue_pop(&profile.rx, connection, characteristic, data, capacity, size);
    FURI_CRITICAL_EXIT(); return result;
}
WeaveBleResult weave_ble_profile_update(uint32_t connection, uint8_t characteristic,
    const uint8_t* data, size_t size) {
    if(!profile.active || characteristic >= profile.config.count || !data || !size ||
       size > profile.config.characteristics[characteristic].max_size ||
       !(profile.config.characteristics[characteristic].properties &
         (WEAVE_BLE_READ | WEAVE_BLE_NOTIFY | WEAVE_BLE_INDICATE))) return WeaveBleInvalid;
    WeaveBleState state;
    uint16_t handle;
    { FURI_CRITICAL_ENTER(); state = profile.rx.state; handle = profile.connection_handle;
      FURI_CRITICAL_EXIT(); }
    const uint8_t properties = profile.config.characteristics[characteristic].properties;
    /* Readable values persist in the controller's GATT database. Only public
     * static metadata may be put in a READ characteristic. */
    if(properties == WEAVE_BLE_READ && !state.connected && connection == 0) {
        return aci_gatt_update_char_value(profile.service, profile.handles[characteristic],
            0, (uint8_t)size, data) == BLE_STATUS_SUCCESS ? WeaveBleOk : WeaveBleBusy;
    }
    if(!state.connected || connection != state.connection) return WeaveBleDisconnected;
    const uint8_t update_type = (properties & WEAVE_BLE_INDICATE) ? 0x02 :
        (properties & WEAVE_BLE_NOTIFY) ? 0x01 : 0x00;
    const tBleStatus status = aci_gatt_update_char_value_ext(handle, profile.service,
        profile.handles[characteristic], update_type, size, 0, (uint8_t)size, (uint8_t*)data);
    WeaveBleState after = weave_ble_profile_state();
    if(!after.connected || after.connection != connection) return WeaveBleDisconnected;
    return status == BLE_STATUS_SUCCESS ? WeaveBleOk : WeaveBleBusy;
}

#endif
