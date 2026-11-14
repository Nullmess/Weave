#include "updater_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const updater_on_enter_handlers[])(void*) = {
#include "updater_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const updater_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "updater_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const updater_on_exit_handlers[])(void* context) = {
#include "updater_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers updater_scene_handlers = {
    .on_enter_handlers = updater_on_enter_handlers,
    .on_event_handlers = updater_on_event_handlers,
    .on_exit_handlers = updater_on_exit_handlers,
    .scene_num = UpdaterSceneNum,
};
