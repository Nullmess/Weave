#include "archive_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const archive_on_enter_handlers[])(void*) = {
#include "archive_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const archive_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "archive_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const archive_on_exit_handlers[])(void* context) = {
#include "archive_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers archive_scene_handlers = {
    .on_enter_handlers = archive_on_enter_handlers,
    .on_event_handlers = archive_on_event_handlers,
    .on_exit_handlers = archive_on_exit_handlers,
    .scene_num = ArchiveAppSceneNum,
};
