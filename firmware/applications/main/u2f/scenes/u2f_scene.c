#include "u2f_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const u2f_scene_on_enter_handlers[])(void*) = {
#include "u2f_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const u2f_scene_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "u2f_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const u2f_scene_on_exit_handlers[])(void* context) = {
#include "u2f_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers u2f_scene_handlers = {
    .on_enter_handlers = u2f_scene_on_enter_handlers,
    .on_event_handlers = u2f_scene_on_event_handlers,
    .on_exit_handlers = u2f_scene_on_exit_handlers,
    .scene_num = U2fSceneNum,
};
