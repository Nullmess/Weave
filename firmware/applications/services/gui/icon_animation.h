/**
 * @file icon_animation.h
 * GUI: IconAnimation API
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <gui/icon.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Icon Animation */
typedef struct IconAnimation IconAnimation;

/* Animation callback déclenche notification update */
typedef void (*IconAnimationCallback)(IconAnimation* instance, void* context);

/* OOM : arrêt système ; retour toujours valide */

IconAnimation* icon_animation_alloc(const Icon* icon);

/** Release icon animation instance
 *
 * @param      instance  IconAnimation instance
 */
void icon_animation_free(IconAnimation* instance);

/* Préférer view_tie_icon_animation */

void icon_animation_set_update_callback(
    IconAnimation* instance,
    IconAnimationCallback callback,
    void* context);

uint8_t icon_animation_get_width(const IconAnimation* instance);

uint8_t icon_animation_get_height(const IconAnimation* instance);

void icon_animation_start(IconAnimation* instance);

void icon_animation_stop(IconAnimation* instance);

bool icon_animation_is_last_frame(const IconAnimation* instance);

#ifdef __cplusplus
}
#endif
