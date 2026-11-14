#include "power_settings_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const power_settings_on_enter_handlers[])(void*) = {
#include "power_settings_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const power_settings_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "power_settings_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const power_settings_on_exit_handlers[])(void* context) = {
#include "power_settings_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers power_settings_scene_handlers = {
    .on_enter_handlers = power_settings_on_enter_handlers,
    .on_event_handlers = power_settings_on_event_handlers,
    .on_exit_handlers = power_settings_on_exit_handlers,
    .scene_num = PowerSettingsAppSceneNum,
};
