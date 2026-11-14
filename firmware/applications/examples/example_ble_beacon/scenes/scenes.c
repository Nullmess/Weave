#include "scenes.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const ble_beacon_app_on_enter_handlers[])(void*) = {
#include "scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const ble_beacon_app_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const ble_beacon_app_on_exit_handlers[])(void* context) = {
#include "scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers ble_beacon_app_scene_handlers = {
    .on_enter_handlers = ble_beacon_app_on_enter_handlers,
    .on_event_handlers = ble_beacon_app_on_event_handlers,
    .on_exit_handlers = ble_beacon_app_on_exit_handlers,
    .scene_num = BleBeaconAppSceneNum,
};
