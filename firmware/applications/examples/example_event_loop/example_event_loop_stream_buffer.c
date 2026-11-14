/* Worker : dix blocs de 32 bytes ; trigger à demi-capacité */
/* Données lues/imprimées par event loop puis sortie */

#include <furi.h>
#include <furi_hal_random.h>

#define TAG "ExampleEventLoopStreamBuffer"

#define WORKER_ITERATION_COUNT (10)

#define STREAM_BUFFER_SIZE            (32)
#define STREAM_BUFFER_TRIG_LEVEL      (STREAM_BUFFER_SIZE / 2)
#define STREAM_BUFFER_EVENT_AND_FLAGS (FuriEventLoopEventIn | FuriEventLoopEventFlagEdge)

typedef struct {
    FuriEventLoop* event_loop;
    FuriThread* worker_thread;
    FuriStreamBuffer* stream_buffer;
} EventLoopStreamBufferApp;

/* Worker séparé pour simulation du flux entrant */
static int32_t event_loop_stream_buffer_app_worker_thread(void* context) {
    furi_assert(context);
    EventLoopStreamBufferApp* app = context;

    FURI_LOG_I(TAG, "Worker thread started");

    for(uint32_t i = 0; i < WORKER_ITERATION_COUNT; ++i) {
        /* Production de 32 bytes de données simulées */
        for(uint32_t j = 0; j < STREAM_BUFFER_SIZE; ++j) {

            uint8_t data = furi_hal_random_get() % 0xFF;
            /* Attente d’espace libre selon contraintes du producteur */

            furi_check(
                furi_stream_buffer_send(app->stream_buffer, &data, 1, FuriWaitForever) == 1);
            /* Délai 30..50 ms pour ralentir l’affichage */
            furi_delay_ms(30 + furi_hal_random_get() % 20);
        }
    }

    FURI_LOG_I(TAG, "All work done, worker thread out!");

    furi_event_loop_stop(app->event_loop);

    return 0;
}

/* Callback si niveau buffer atteint le seuil trigger */
static void
    event_loop_stream_buffer_app_event_callback(FuriEventLoopObject* object, void* context) {
    furi_assert(context);
    EventLoopStreamBufferApp* app = context;

    furi_assert(object == app->stream_buffer);

    /* Buffer temporaire limité à la demi-capacité stream */
    uint8_t data[STREAM_BUFFER_TRIG_LEVEL];
    /* Données >=trigger garanties ; aucune attente requise */

    furi_check(
        furi_stream_buffer_receive(app->stream_buffer, data, sizeof(data), 0) == sizeof(data));

    FuriString* tmp_str = furi_string_alloc();
    for(uint32_t i = 0; i < sizeof(data); ++i) {
        furi_string_cat_printf(tmp_str, "%02X ", data[i]);
    }

    FURI_LOG_I(TAG, "Received data: %s", furi_string_get_cstr(tmp_str));
    furi_string_free(tmp_str);
}

static EventLoopStreamBufferApp* event_loop_stream_buffer_app_alloc(void) {
    EventLoopStreamBufferApp* app = malloc(sizeof(EventLoopStreamBufferApp));

    app->event_loop = furi_event_loop_alloc();

    app->worker_thread = furi_thread_alloc_ex(
        "EventLoopStreamBufferWorker", 1024, event_loop_stream_buffer_app_worker_thread, app);

    app->stream_buffer = furi_stream_buffer_alloc(STREAM_BUFFER_SIZE, STREAM_BUFFER_TRIG_LEVEL);
    /* Abonnement IN stream buffer en mode edge-triggered */
    furi_event_loop_subscribe_stream_buffer(
        app->event_loop,
        app->stream_buffer,
        STREAM_BUFFER_EVENT_AND_FLAGS,
        event_loop_stream_buffer_app_event_callback,
        app);

    return app;
}

static void event_loop_stream_buffer_app_free(EventLoopStreamBufferApp* app) {
    /* Désabonnement obligatoire avant suppression event loop */
    /* Sinon, crash */
    furi_event_loop_unsubscribe(app->event_loop, app->stream_buffer);

    furi_thread_free(app->worker_thread);
    furi_stream_buffer_free(app->stream_buffer);
    furi_event_loop_free(app->event_loop);

    free(app);
}

static void event_loop_stream_buffer_app_run(EventLoopStreamBufferApp* app) {
    furi_thread_start(app->worker_thread);
    furi_event_loop_run(app->event_loop);
    furi_thread_join(app->worker_thread);
}

int32_t example_event_loop_stream_buffer_app(void* arg) {
    UNUSED(arg);

    EventLoopStreamBufferApp* app = event_loop_stream_buffer_app_alloc();
    event_loop_stream_buffer_app_run(app);
    event_loop_stream_buffer_app_free(app);

    return 0;
}
