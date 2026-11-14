

#pragma once

#include "view_port.h"
#include "canvas.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GuiLayerDesktop, /* Desktop interne : fullscreen avec status bar */

    GuiLayerWindow, /* Window avec status bar */

    GuiLayerStatusBarLeft, /* Status bar gauche, auto-layout */
    GuiLayerStatusBarRight, /* Status bar droite, auto-layout */

    GuiLayerFullscreen, /* Fullscreen sans status bar */

    GuiLayerMAX /* Valeur spéciale ; ne pas utiliser ni déplacer */
} GuiLayer;

typedef void (*GuiCanvasCommitCallback)(
    uint8_t* data,
    size_t size,
    CanvasOrientation orientation,
    void* context);

#define RECORD_GUI "gui"

typedef struct Gui Gui;

/* Thread-safe */

void gui_add_view_port(Gui* gui, ViewPort* view_port, GuiLayer layer);

/* Thread-safe */

void gui_remove_view_port(Gui* gui, ViewPort* view_port);

void gui_view_port_send_to_front(Gui* gui, ViewPort* view_port);

void gui_view_port_send_to_back(Gui* gui, ViewPort* view_port);

/* Callback thread GUI, timing critique */

void gui_add_framebuffer_callback(Gui* gui, GuiCanvasCommitCallback callback, void* context);

void gui_remove_framebuffer_callback(Gui* gui, GuiCanvasCommitCallback callback, void* context);

/* Taille framebuffer en octets */

size_t gui_get_framebuffer_size(const Gui* gui);

/* Lockdown : seul GuiLayerDesktop visible */

void gui_set_lockdown(Gui* gui, bool lockdown);

/* Direct draw exclusif : bloque inputs et dessin GUI jusqu’à release */

Canvas* gui_direct_draw_acquire(Gui* gui);

/* Release réactive GUI ; canvas acquis devient invalide */

void gui_direct_draw_release(Gui* gui);

#ifdef __cplusplus
}
#endif
