#include "../bt_settings_app.h"
#include <furi_hal_bt.h>

enum BtSettingIndex {
    BtSettingIndexForgetDev = 1,
};

static void bt_settings_scene_start_var_list_enter_callback(void* context, uint32_t index) {
    furi_assert(context);
    BtSettingsApp* app = context;
    if(index == BtSettingIndexForgetDev) {
        view_dispatcher_send_custom_event(
            app->view_dispatcher, BtSettingsCustomEventForgetDevices);
    }
}

void bt_settings_scene_start_on_enter(void* context) {
    BtSettingsApp* app = context;
    VariableItemList* var_item_list = app->var_item_list;
    VariableItem* item;

    if(furi_hal_bt_is_gatt_gap_supported()) {
        // The firmware enables BLE at each boot and deliberately does not offer
        // an OFF switch that would immediately be overridden by the BT service.
        item = variable_item_list_add(var_item_list, "Bluetooth", 1, NULL, NULL);
        variable_item_set_current_value_text(item, "ON (Weave)");
        variable_item_list_add(var_item_list, "Unpair All Devices", 1, NULL, NULL);
        variable_item_list_set_enter_callback(
            var_item_list, bt_settings_scene_start_var_list_enter_callback, app);
    } else {
        item = variable_item_list_add(var_item_list, "Bluetooth", 1, NULL, NULL);
        variable_item_set_current_value_text(item, "Broken");
    }

    view_dispatcher_switch_to_view(app->view_dispatcher, BtSettingsAppViewVarItemList);
}

bool bt_settings_scene_start_on_event(void* context, SceneManagerEvent event) {
    BtSettingsApp* app = context;
    if(event.type == SceneManagerEventTypeCustom &&
       event.event == BtSettingsCustomEventForgetDevices) {
        scene_manager_next_scene(app->scene_manager, BtSettingsAppSceneForgetDevConfirm);
        return true;
    }
    return false;
}

void bt_settings_scene_start_on_exit(void* context) {
    BtSettingsApp* app = context;
    variable_item_list_reset(app->var_item_list);
}
