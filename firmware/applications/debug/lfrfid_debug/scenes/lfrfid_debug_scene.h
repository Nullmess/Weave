#pragma once

#include <gui/scene_manager.h>

/* Identifiants et nombre de scènes */
#define ADD_SCENE(prefix, name, id) LfRfidDebugScene##id,
typedef enum {
#include "lfrfid_debug_scene_config.h"
    LfRfidDebugSceneNum,
} LfRfidDebugScene;
#undef ADD_SCENE

extern const SceneManagerHandlers lfrfid_debug_scene_handlers;

/* Déclarations handlers on_enter */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "lfrfid_debug_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_event */
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "lfrfid_debug_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_exit */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "lfrfid_debug_scene_config.h"
#undef ADD_SCENE
