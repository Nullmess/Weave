/* Pipe bidirectionnel entre deux threads ; utilisable dans un seul */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <furi.h>
#include <stddef.h>

/* Alice/Bob : rôles égaux, chacun lit et écrit */

typedef enum {
    PipeRoleAlice,
    PipeRoleBob,
} PipeRole;

/* Peer libéré : pipe Broken irréversible, aucune reconnexion */

typedef enum {
    PipeStateOpen,
    PipeStateBroken,
} PipeState;

typedef struct PipeSide PipeSide;

typedef struct {
    PipeSide* alices_side;
    PipeSide* bobs_side;
} PipeSideBundle;

typedef struct {
    size_t capacity;
    size_t trigger_level;
} PipeSideReceiveSettings;

/* Seule allocation paire connecte deux sides, sans reconnexion */
/* Capacity/trigger_level identiques pour les deux directions */
/* Trigger level : octets nécessaires pour débloquer réception */

PipeSideBundle pipe_alloc(size_t capacity, size_t trigger_level);

/* Capacity/trigger_level distincts possibles, par buffer réception Alice/Bob */

PipeSideBundle pipe_alloc_ex(PipeSideReceiveSettings alice, PipeSideReceiveSettings bob);

PipeRole pipe_role(PipeSide* pipe);

/* Broken : données reçues résiduelles ; données envoyées perdues */

PipeState pipe_state(PipeSide* pipe);

/* Un side libéré : Broken ; deux libérés : structures détruites */

void pipe_free(PipeSide* pipe);

/* Pipe Broken : stdio renvoie EOF si possible */
/* Déconnexion : callbacks stdin/stdout du thread remis à NULL */

void pipe_install_as_stdio(PipeSide* pipe);

/* Période état : ticks, 100 ms par défaut */
/* Vérifier Broken si zéro octet transféré pendant période */

void pipe_set_state_check_period(PipeSide* pipe, FuriWait check_period);

/* Réception complète tentée ; Broken : retour partiel possible */

size_t pipe_receive(PipeSide* pipe, void* data, size_t length);

/* Envoi complet tenté ; Broken : retour partiel possible */

size_t pipe_send(PipeSide* pipe, const void* data, size_t length);

size_t pipe_bytes_available(PipeSide* pipe);

size_t pipe_spaces_available(PipeSide* pipe);

void pipe_attach_to_event_loop(PipeSide* pipe, FuriEventLoop* event_loop);

void pipe_detach_from_event_loop(PipeSide* pipe);

typedef void (*PipeSideDataArrivedCallback)(PipeSide* pipe, void* context);

/* Callback quand peer consomme des données */

typedef void (*PipeSideSpaceFreedCallback)(PipeSide* pipe, void* context);

/* Callback quand peer libéré, pipe devenu Broken */

typedef void (*PipeSideBrokenCallback)(PipeSide* pipe, void* context);

void pipe_set_callback_context(PipeSide* pipe, void* context);

/* Attacher event loop avant abonnement ; NULL désabonne */
/* event : flags uniquement, Edge/Once admis */

void pipe_set_data_arrived_callback(
    PipeSide* pipe,
    PipeSideDataArrivedCallback callback,
    FuriEventLoopEvent event);

/* Attacher event loop avant abonnement ; NULL désabonne */
/* event : flags uniquement, Edge/Once admis */

void pipe_set_space_freed_callback(
    PipeSide* pipe,
    PipeSideSpaceFreedCallback callback,
    FuriEventLoopEvent event);

/* Attacher event loop avant abonnement ; NULL désabonne */
/* event : flags uniquement, Edge/Once admis */

void pipe_set_broken_callback(
    PipeSide* pipe,
    PipeSideBrokenCallback callback,
    FuriEventLoopEvent event);

#ifdef __cplusplus
}
#endif
