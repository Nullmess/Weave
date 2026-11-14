#pragma once

#include <gui/scene_manager.h>

/* Identifiants et nombre de scènes */
#define ADD_SCENE(prefix, name, id) PowerSettingsAppScene##id,
typedef enum {
#include "power_settings_scene_config.h"
    PowerSettingsAppSceneNum,
} PowerSettingsAppScene;
#undef ADD_SCENE

extern const SceneManagerHandlers power_settings_scene_handlers;

/* Déclarations handlers on_enter */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_enter(void*);
#include "power_settings_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_event */
#define ADD_SCENE(prefix, name, id) \
    bool prefix##_scene_##name##_on_event(void* context, SceneManagerEvent event);
#include "power_settings_scene_config.h"
#undef ADD_SCENE

/* Déclarations handlers on_exit */
#define ADD_SCENE(prefix, name, id) void prefix##_scene_##name##_on_exit(void* context);
#include "power_settings_scene_config.h"
#undef ADD_SCENE
