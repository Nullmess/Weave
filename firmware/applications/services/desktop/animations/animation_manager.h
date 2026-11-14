#pragma once

#include <gui/view.h>
#include <gui/icon_i.h>
#include <stdint.h>
#include <dolphin/dolphin.h>

typedef struct AnimationManager AnimationManager;

typedef struct {
    uint8_t x;
    uint8_t y;
    const char* text;
    Align align_h;
    Align align_v;
} Bubble;

typedef struct FrameBubble {
    Bubble bubble;
    uint8_t start_frame;
    uint8_t end_frame;
    const struct FrameBubble* next_bubble;
} FrameBubble;

typedef struct {
    const FrameBubble* const* frame_bubble_sequences;
    uint8_t frame_bubble_sequences_count;
    const Icon icon_animation;
    const uint8_t* frame_order;
    uint8_t passive_frames;
    uint8_t active_frames;
    uint8_t active_cycles;
    uint16_t duration;
    uint16_t active_cooldown;
} BubbleAnimation;

typedef void (*AnimationManagerSetNewIdleAnimationCallback)(void* context);
typedef void (*AnimationManagerCheckBlockingCallback)(void* context);
typedef void (*AnimationManagerInteractCallback)(void*);

AnimationManager* animation_manager_alloc(void);

void animation_manager_free(AnimationManager* animation_manager);

View* animation_manager_get_animation_view(AnimationManager* animation_manager);

void animation_manager_set_context(AnimationManager* animation_manager, void* context);

/* Animation Manager sans thread dédié */
/* Callback externe notifie thread principal */
/* Thread principal appelle new_idle_process dans son contexte */

void animation_manager_set_new_idle_callback(
    AnimationManager* animation_manager,
    AnimationManagerSetNewIdleAnimationCallback callback);

/* Appeler dans thread principal après set_new_idle_callback */

void animation_manager_new_idle_process(AnimationManager* animation_manager);

void animation_manager_set_check_callback(
    AnimationManager* animation_manager,
    AnimationManagerCheckBlockingCallback callback);

/* Appeler dans thread principal après set_new_idle_callback */

void animation_manager_check_blocking_process(AnimationManager* animation_manager);

void animation_manager_set_interact_callback(
    AnimationManager* animation_manager,
    AnimationManagerInteractCallback callback);

/* Thread principal ; true si event consommé */

bool animation_manager_interact_process(AnimationManager* animation_manager);

bool animation_manager_is_animation_loaded(AnimationManager* animation_manager);

/* Stall fige première frame jusqu’à load_and_continue */
/* Pas de Stall répété ; chaque Stall exige Continue */

void animation_manager_unload_and_stall_animation(AnimationManager* animation_manager);

void animation_manager_load_and_continue_animation(AnimationManager* animation_manager);

void animation_manager_set_dummy_mode_state(AnimationManager* animation_manager, bool enabled);
