#include "example_number_input_scene.h"

/* Table handlers on_enter */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_enter,
void (*const example_number_input_on_enter_handlers[])(void*) = {
#include "example_number_input_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_event */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_event,
bool (*const example_number_input_on_event_handlers[])(void* context, SceneManagerEvent event) = {
#include "example_number_input_scene_config.h"
};
#undef ADD_SCENE

/* Table handlers on_exit */
#define ADD_SCENE(prefix, name, id) prefix##_scene_##name##_on_exit,
void (*const example_number_input_on_exit_handlers[])(void* context) = {
#include "example_number_input_scene_config.h"
};
#undef ADD_SCENE

/* Configuration handlers de scènes */
const SceneManagerHandlers example_number_input_scene_handlers = {
    .on_enter_handlers = example_number_input_on_enter_handlers,
    .on_event_handlers = example_number_input_on_event_handlers,
    .on_exit_handlers = example_number_input_on_exit_handlers,
    .scene_num = ExampleNumberInputSceneNum,
};
