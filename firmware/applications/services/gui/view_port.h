

#pragma once

#include <input/input.h>
#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ViewPort ViewPort;

typedef enum {
    ViewPortOrientationHorizontal,
    ViewPortOrientationHorizontalFlip,
    ViewPortOrientationVertical,
    ViewPortOrientationVerticalFlip,
    ViewPortOrientationMAX, /* Valeur spéciale ; ne pas utiliser */
} ViewPortOrientation;

/* Callback depuis thread GUI */

typedef void (*ViewPortDrawCallback)(Canvas* canvas, void* context);

/* Callback depuis thread GUI */

typedef void (*ViewPortInputCallback)(InputEvent* event, void* context);

/* OOM : arrêt système ; retour toujours valide */

ViewPort* view_port_alloc(void);

/* Désinscrire du GUI avant free */

void view_port_free(ViewPort* view_port);

/* Largeur limite canvas/autolayout ; 0 automatique */

void view_port_set_width(ViewPort* view_port, uint8_t width);
uint8_t view_port_get_width(const ViewPort* view_port);

/* Hauteur limite canvas/autolayout ; 0 automatique */

void view_port_set_height(ViewPort* view_port, uint8_t height);
uint8_t view_port_get_height(const ViewPort* view_port);

/* Dispatch automatique event update */

void view_port_enabled_set(ViewPort* view_port, bool enabled);
bool view_port_is_enabled(const ViewPort* view_port);

/* Callback depuis thread GUI */

void view_port_draw_callback_set(ViewPort* view_port, ViewPortDrawCallback callback, void* context);

/* Callback depuis thread GUI */

void view_port_input_callback_set(
    ViewPort* view_port,
    ViewPortInputCallback callback,
    void* context);

/* Rendu différé après traitement signal par GUI */

void view_port_update(ViewPort* view_port);

void view_port_set_orientation(ViewPort* view_port, ViewPortOrientation orientation);
ViewPortOrientation view_port_get_orientation(const ViewPort* view_port);

#ifdef __cplusplus
}
#endif
