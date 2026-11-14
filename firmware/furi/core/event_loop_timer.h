/* Tous les appels depuis le thread créateur de l’event loop */

#pragma once

#include "event_loop.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriEventLoopTimerTypeOnce = 0,
    FuriEventLoopTimerTypePeriodic = 1,
} FuriEventLoopTimerType;

/* Callback : démarrage/arrêt, création/libération de timers permis */
typedef void (*FuriEventLoopTimerCallback)(void* context);

typedef struct FuriEventLoopTimer FuriEventLoopTimer;

FuriEventLoopTimer* furi_event_loop_timer_alloc(
    FuriEventLoop* instance,
    FuriEventLoopTimerCallback callback,
    FuriEventLoopTimerType type,
    void* context);

/* Libérer tous les timers avant furi_event_loop_free */
/* Libération autorisée avec loop active ou arrêtée */
void furi_event_loop_timer_free(FuriEventLoopTimer* timer);

/* Intervalle en ticks */
void furi_event_loop_timer_start(FuriEventLoopTimer* timer, uint32_t interval);

void furi_event_loop_timer_restart(FuriEventLoopTimer* timer);

/* Aucun callback ; timer déjà arrêté : aucune opération */
void furi_event_loop_timer_stop(FuriEventLoopTimer* timer);

/* Ticks restants ; 0 si timer arrêté ou expiré */
uint32_t furi_event_loop_timer_get_remaining_time(const FuriEventLoopTimer* timer);

/* Intervalle en ticks */
uint32_t furi_event_loop_timer_get_interval(const FuriEventLoopTimer* timer);

bool furi_event_loop_timer_is_running(const FuriEventLoopTimer* timer);

#ifdef __cplusplus
}
#endif
