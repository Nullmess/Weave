#pragma once

#include <gui/scene_manager.h>

/* Identifiants et nombre de scènes */
#define ADD_SCENE(prefix, name, id) DesktopScene##id,
typedef enum {
#include "desktop_scene_config.h"
    DesktopSceneNum,
} DesktopScene;
#undef ADD_SCENE

extern const SceneManagerHandlers desktop_scene_handlers;

/* Déclarations handlers on_enter */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "desktop_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_event */
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "desktop_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_exit */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "desktop_scene_config.h"
#undef ADD_SCENE
