#include "infrared_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const infrared_on_enter_handlers[])(void*) = {
#include "infrared_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const infrared_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "infrared_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const infrared_on_exit_handlers[])(void* context) = {
#include "infrared_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers infrared_scene_handlers = {
    .on_enter_handlers = infrared_on_enter_handlers,
    .on_event_handlers = infrared_on_event_handlers,
    .on_exit_handlers = infrared_on_exit_handlers,
    .scene_num = InfraredSceneNum,
};
