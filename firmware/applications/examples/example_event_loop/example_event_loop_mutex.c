/* Event loop/mutex : dix opérations simulées dans un worker */

#include <furi.h>
#include <furi_hal_random.h>

#define TAG "ExampleEventLoopMutex"

#define WORKER_ITERATION_COUNT (10)
/* IN pour mutex : release détecté */
/* Edge-triggered : uniquement changements d’état mutex */
/* One-shot : désabonnement avant traitement */
#define MUTEX_EVENT_AND_FLAGS \
    (FuriEventLoopEventIn | FuriEventLoopEventFlagEdge | FuriEventLoopEventFlagOnce)

typedef struct {
    FuriEventLoop* event_loop;
    FuriThread* worker_thread;
    FuriMutex* worker_mutex;
    uint8_t worker_result;
} EventLoopMutexApp;

/* Worker séparé pour opérations bloquantes */
static int32_t event_loop_mutex_app_worker_thread(void* context) {
    furi_assert(context);
    EventLoopMutexApp* app = context;

    FURI_LOG_I(TAG, "Worker thread started");

    for(uint32_t i = 0; i < WORKER_ITERATION_COUNT; ++i) {
        FURI_LOG_I(TAG, "Doing work ...");

        furi_check(furi_mutex_acquire(app->worker_mutex, FuriWaitForever) == FuriStatusOk);
        /* Délai simulé aléatoire de 900 à 1100 ms */
        const uint32_t work_time_ms = 900 + furi_hal_random_get() % 200;
        furi_delay_ms(work_time_ms);
        /* Résultat simulé aléatoire dans [0,255] */
        app->worker_result = furi_hal_random_get() % 0xFF;

        FURI_LOG_I(TAG, "Work done in %lu ms", work_time_ms);
        /* Release mutex notifie que résultat disponible */
        furi_check(furi_mutex_release(app->worker_mutex) == FuriStatusOk);
        /* Yield permet à l’event loop d’acquérir le mutex */
        furi_thread_yield();
    }

    FURI_LOG_I(TAG, "All work done, worker thread out!");

    furi_event_loop_stop(app->event_loop);

    return 0;
}

static void event_loop_mutex_app_event_callback(FuriEventLoopObject* object, void* context) {
    furi_assert(context);

    EventLoopMutexApp* app = context;
    furi_assert(object == app->worker_mutex);

    /* Timeout impérativement 0 dans les event callbacks */
    /* Aucun event callback bloquant ; gérer les autres détenteurs */

    furi_check(furi_mutex_acquire(app->worker_mutex, 0) == FuriStatusOk);

    FURI_LOG_I(TAG, "Result available! Value: %u", app->worker_result);

    furi_check(furi_mutex_release(app->worker_mutex) == FuriStatusOk);
    /* Réabonnement après traitement one-shot */

    furi_event_loop_subscribe_mutex(
        app->event_loop,
        app->worker_mutex,
        MUTEX_EVENT_AND_FLAGS,
        event_loop_mutex_app_event_callback,
        app);
}

static EventLoopMutexApp* event_loop_mutex_app_alloc(void) {
    EventLoopMutexApp* app = malloc(sizeof(EventLoopMutexApp));

    app->event_loop = furi_event_loop_alloc();

    app->worker_thread = furi_thread_alloc_ex(
        "EventLoopMutexWorker", 1024, event_loop_mutex_app_worker_thread, app);

    app->worker_mutex = furi_mutex_alloc(FuriMutexTypeNormal);

    /* One-shot désabonne avant callback pour éviter notification du release */

    furi_event_loop_subscribe_mutex(
        app->event_loop,
        app->worker_mutex,
        MUTEX_EVENT_AND_FLAGS,
        event_loop_mutex_app_event_callback,
        app);

    return app;
}

static void event_loop_mutex_app_free(EventLoopMutexApp* app) {
    /* Désabonnement obligatoire avant suppression event loop */
    /* Sinon, crash */
    furi_event_loop_unsubscribe(app->event_loop, app->worker_mutex);

    furi_thread_free(app->worker_thread);
    furi_mutex_free(app->worker_mutex);
    furi_event_loop_free(app->event_loop);

    free(app);
}

static void event_loop_mutex_app_run(EventLoopMutexApp* app) {
    furi_thread_start(app->worker_thread);
    furi_event_loop_run(app->event_loop);
    furi_thread_join(app->worker_thread);
}

int32_t example_event_loop_mutex_app(void* arg) {
    UNUSED(arg);

    EventLoopMutexApp* app = event_loop_mutex_app_alloc();
    event_loop_mutex_app_run(app);
    event_loop_mutex_app_free(app);

    return 0;
}
