#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*FuriPubSubCallback)(const void* message, void* context);

typedef struct FuriPubSub FuriPubSub;

typedef struct FuriPubSubSubscription FuriPubSubSubscription;

/* Réentrant, non thread-safe ; propriétaire unique */
FuriPubSub* furi_pubsub_alloc(void);

void furi_pubsub_free(FuriPubSub* pubsub);

/* Thread-safe et réentrant */
FuriPubSubSubscription*
    furi_pubsub_subscribe(FuriPubSub* pubsub, FuriPubSubCallback callback, void* callback_context);

/* Thread-safe, réentrant ; subscription invalide après désabonnement */
void furi_pubsub_unsubscribe(FuriPubSub* pubsub, FuriPubSubSubscription* pubsub_subscription);

/* Thread-safe et réentrant */
void furi_pubsub_publish(FuriPubSub* pubsub, void* message);

#ifdef __cplusplus
}
#endif
