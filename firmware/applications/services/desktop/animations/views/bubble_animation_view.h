#pragma once

#include <gui/view.h>
#include "../animation_manager.h"

/** Bubble Animation instance */
typedef struct BubbleAnimationView BubbleAnimationView;

typedef void (*BubbleAnimationInteractCallback)(void*);

/* Animation bubble : phases active/passive */

BubbleAnimationView* bubble_animation_view_alloc(void);

void bubble_animation_view_free(BubbleAnimationView* view);

/* Interact callback : bouton droit actuellement */

void bubble_animation_view_set_interact_callback(
    BubbleAnimationView* view,
    BubbleAnimationInteractCallback callback,
    void* context);

/* BubbleAnimation ne possède ni ne libère les animations */

void bubble_animation_view_set_animation(
    BubbleAnimationView* view,
    const BubbleAnimation* new_bubble_animation);

View* bubble_animation_get_view(BubbleAnimationView* view);

/* Freeze supprime toute référence animation courante, ensuite libérable */
/* Assigner nouvelle animation avant unfreeze */

void bubble_animation_freeze(BubbleAnimationView* view);

void bubble_animation_unfreeze(BubbleAnimationView* view);
