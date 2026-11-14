/* Une seule event loop par thread */
/* Toutes les opérations depuis le thread créateur */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Un abonnement par direction ; flags combinables par OR */
typedef enum {
    /* In : queue non vide, trigger stream atteint */
    /* In : semaphore ou mutex libéré */
    FuriEventLoopEventIn = 0x00000001U,
    /* Out : item retiré ou données stream lues */
    /* Out : semaphore ou mutex acquis */
    FuriEventLoopEventOut = 0x00000002U,
    FuriEventLoopEventMask = 0x00000003U,
    /* Par défaut : level-triggered, répété tant que niveau non nul */
    /* In : données disponibles ou primitive libre */
    /* Out : espace disponible ou primitive acquise */
    /* Edge-triggered : uniquement les changements dans la direction choisie */
    FuriEventLoopEventFlagEdge = 0x00000004U,
    /* Désabonnement automatique après le premier événement */
    FuriEventLoopEventFlagOnce = 0x00000008U,
    FuriEventLoopEventFlagMask = 0xFFFFFFFCU,
    /* Force un enum sur 32 bits */
    FuriEventLoopEventReserved = UINT32_MAX,
} FuriEventLoopEvent;

typedef struct FuriEventLoop FuriEventLoop;

/* Une event loop par thread ; usage par son créateur */
/* Aucune API bloquante sur les objets confiés à l’event loop */
FuriEventLoop* furi_event_loop_alloc(void);

void furi_event_loop_free(FuriEventLoop* instance);

/* Polling continu jusqu’à furi_event_loop_stop */
void furi_event_loop_run(FuriEventLoop* instance);

void furi_event_loop_stop(FuriEventLoop* instance);

typedef void (*FuriEventLoopTickCallback)(void* context);

/* Tick après inactivité, inférieur aux primitives et timers */
/* Non monotone : ticks ignorés si loop occupée */
void furi_event_loop_tick_set(
    FuriEventLoop* instance,
    uint32_t interval,
    FuriEventLoopTickCallback callback,
    void* context);

typedef void (*FuriEventLoopPendingCallback)(void* context);

/* Callback après toutes les commandes timer précédentes */
void furi_event_loop_pend_callback(
    FuriEventLoop* instance,
    FuriEventLoopPendingCallback callback,
    void* context);

typedef void FuriEventLoopObject;

typedef void (*FuriEventLoopEventCallback)(FuriEventLoopObject* object, void* context);

typedef void (*FuriEventLoopThreadFlagsCallback)(void* context);

typedef struct FuriEventFlag FuriEventFlag;

/* Un seul abonnement par type d’événement */

void furi_event_loop_subscribe_event_flag(
    FuriEventLoop* instance,
    FuriEventFlag* event_flag,
    FuriEventLoopEvent event,
    FuriEventLoopEventCallback callback,
    void* context);

typedef struct FuriMessageQueue FuriMessageQueue;

/* Un seul abonnement par type d’événement */
void furi_event_loop_subscribe_message_queue(
    FuriEventLoop* instance,
    FuriMessageQueue* message_queue,
    FuriEventLoopEvent event,
    FuriEventLoopEventCallback callback,
    void* context);

typedef struct FuriStreamBuffer FuriStreamBuffer;

/* Un seul abonnement par type d’événement */
void furi_event_loop_subscribe_stream_buffer(
    FuriEventLoop* instance,
    FuriStreamBuffer* stream_buffer,
    FuriEventLoopEvent event,
    FuriEventLoopEventCallback callback,
    void* context);

typedef struct FuriSemaphore FuriSemaphore;

/* Un seul abonnement par type d’événement */
void furi_event_loop_subscribe_semaphore(
    FuriEventLoop* instance,
    FuriSemaphore* semaphore,
    FuriEventLoopEvent event,
    FuriEventLoopEventCallback callback,
    void* context);

typedef struct FuriMutex FuriMutex;

/* Un seul abonnement par type d’événement */
void furi_event_loop_subscribe_mutex(
    FuriEventLoop* instance,
    FuriMutex* mutex,
    FuriEventLoopEvent event,
    FuriEventLoopEventCallback callback,
    void* context);

void furi_event_loop_subscribe_thread_flags(
    FuriEventLoop* instance,
    FuriEventLoopThreadFlagsCallback callback,
    void* context);

void furi_event_loop_unsubscribe_thread_flags(FuriEventLoop* instance);

void furi_event_loop_unsubscribe(FuriEventLoop* instance, FuriEventLoopObject* object);

bool furi_event_loop_is_subscribed(FuriEventLoop* instance, FuriEventLoopObject* object);

static inline void
    furi_event_loop_maybe_unsubscribe(FuriEventLoop* instance, FuriEventLoopObject* object) {
    if(furi_event_loop_is_subscribed(instance, object))
        furi_event_loop_unsubscribe(instance, object);
}

#ifdef __cplusplus
}
#endif
