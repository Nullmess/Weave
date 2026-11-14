#pragma once

#include <gui/scene_manager.h>

/* Identifiants et nombre de scènes */
#define ADD_SCENE(prefix, name, id) ExampleDateTimeInputScene##id,
typedef enum {
#include "example_date_time_input_scene_config.h"
    ExampleDateTimeInputSceneNum,
} ExampleDateTimeInputScene;
#undef ADD_SCENE

extern const SceneManagerHandlers example_date_time_input_scene_handlers;

/* Déclarations handlers on_enter */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "example_date_time_input_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_event */
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "example_date_time_input_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_exit */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "example_date_time_input_scene_config.h"
#undef ADD_SCENE
