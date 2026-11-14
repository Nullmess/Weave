#include "desktop_settings_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const desktop_settings_on_enter_handlers[])(void*) = {
#include "desktop_settings_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const desktop_settings_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "desktop_settings_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const desktop_settings_on_exit_handlers[])(void* context) = {
#include "desktop_settings_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers desktop_settings_scene_handlers = {
    .on_enter_handlers = desktop_settings_on_enter_handlers,
    .on_event_handlers = desktop_settings_on_event_handlers,
    .on_exit_handlers = desktop_settings_on_exit_handlers,
    .scene_num = DesktopSettingsAppSceneNum,
};
