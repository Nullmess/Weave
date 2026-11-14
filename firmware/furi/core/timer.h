#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FuriTimerCallback)(void* context);

typedef enum {
    FuriTimerTypeOnce = 0,
    FuriTimerTypePeriodic = 1
} FuriTimerType;

typedef struct FuriTimer FuriTimer;

FuriTimer* furi_timer_alloc(FuriTimerCallback func, FuriTimerType type, void* context);

void furi_timer_free(FuriTimer* instance);

/* Attend le traitement de toutes les commandes timer précédentes */
void furi_timer_flush(void);

/* Asynchrone via service timer ; intervalle en ticks */
FuriStatus furi_timer_start(FuriTimer* instance, uint32_t ticks);

/* Asynchrone via service timer ; intervalle précédent */
FuriStatus furi_timer_restart(FuriTimer* instance, uint32_t ticks);

/* Synchrone : attend le traitement de la queue timer */
FuriStatus furi_timer_stop(FuriTimer* instance);

/* État potentiellement périmé si commandes timer en attente */
uint32_t furi_timer_is_running(FuriTimer* instance);

/* Tick d’expiration */
uint32_t furi_timer_get_expire_time(FuriTimer* instance);

typedef void (*FuriTimerPendigCallback)(void* context, uint32_t arg);

void furi_timer_pending_callback(FuriTimerPendigCallback callback, void* context, uint32_t arg);

typedef enum {
    FuriTimerThreadPriorityNormal, /* Priorité inférieure aux autres threads */
    FuriTimerThreadPriorityElevated, /* Priorité identique aux autres threads */
} FuriTimerThreadPriority;

void furi_timer_set_thread_priority(FuriTimerThreadPriority priority);

#ifdef __cplusplus
}
#endif
