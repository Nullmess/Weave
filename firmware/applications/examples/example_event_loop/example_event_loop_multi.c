/* Deux event loops threadées : stream, timers et messages */
/* Back long démarre décompte ; OK long annule/réinitialise */
/* Worker génère données aléatoires puis notifie le main thread */

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_port.h>

#include <furi_hal_random.h>

#define TAG "ExampleEventLoopMulti"

#define COUNTDOWN_START_VALUE   (5UL)
#define COUNTDOWN_INTERVAL_MS   (1000UL)
#define WORKER_DATA_INTERVAL_MS (1500UL)

#define INPUT_QUEUE_SIZE   (8)
#define STREAM_BUFFER_SIZE (16)

typedef struct {
    FuriEventLoop* event_loop;
    FuriEventLoopTimer* timer;
    FuriStreamBuffer* stream_buffer;
} EventLoopMultiAppWorker;

typedef struct {
    Gui* gui;
    ViewPort* view_port;
    FuriThread* worker_thread;
    FuriEventLoop* event_loop;
    FuriMessageQueue* input_queue;
    FuriEventLoopTimer* exit_timer;
    FuriStreamBuffer* stream_buffer;
    uint32_t exit_countdown_value;
} EventLoopMultiApp;

/* Lecture du stream buffer : redémarrage du worker timer */
static void
    event_loop_multi_app_stream_buffer_worker_callback(FuriEventLoopObject* object, void* context) {
    furi_assert(context);
    EventLoopMultiAppWorker* worker = context;

    furi_assert(object == worker->stream_buffer);

    FURI_LOG_I(TAG, "Data was removed from buffer");

    furi_event_loop_timer_start(worker->timer, WORKER_DATA_INTERVAL_MS);
}

/* Timer one-shot : aucun redémarrage automatique */

static void event_loop_multi_app_worker_timer_callback(void* context) {
    furi_assert(context);
    EventLoopMultiAppWorker* worker = context;

    uint8_t data[STREAM_BUFFER_SIZE];
    furi_hal_random_fill_buf(data, sizeof(data));

    /* Aucune attente dans les event handlers */
    furi_check(
        furi_stream_buffer_send(worker->stream_buffer, &data, sizeof(data), 0) == sizeof(data));
}

static EventLoopMultiAppWorker*
    event_loop_multi_app_worker_alloc(FuriStreamBuffer* stream_buffer) {
    EventLoopMultiAppWorker* worker = malloc(sizeof(EventLoopMultiAppWorker));

    worker->event_loop = furi_event_loop_alloc();

    /* One-shot : aucun redémarrage automatique à expiration */
    worker->timer = furi_event_loop_timer_alloc(
        worker->event_loop,
        event_loop_multi_app_worker_timer_callback,
        FuriEventLoopTimerTypeOnce,
        worker);

    /* Stream buffer partagé avec le main thread */
    worker->stream_buffer = stream_buffer;

    furi_event_loop_subscribe_stream_buffer(
        worker->event_loop,
        worker->stream_buffer,
        FuriEventLoopEventOut | FuriEventLoopEventFlagEdge,
        event_loop_multi_app_stream_buffer_worker_callback,
        worker);

    return worker;
}

static void event_loop_multi_app_worker_free(EventLoopMultiAppWorker* worker) {
    /* Désabonnement obligatoire avant suppression event loop */
    /* Sinon, crash */
    furi_event_loop_unsubscribe(worker->event_loop, worker->stream_buffer);
    /* Libérer tous les timers avant l’event loop associée */
    /* Sinon, crash */
    furi_event_loop_timer_free(worker->timer);

    furi_event_loop_free(worker->event_loop);

    free(worker);
}

static void event_loop_multi_app_worker_run(EventLoopMultiAppWorker* worker) {
    furi_event_loop_timer_start(worker->timer, WORKER_DATA_INTERVAL_MS);
    furi_event_loop_run(worker->event_loop);
}

/* Exécution dans le worker thread */
static int32_t event_loop_multi_app_worker_thread(void* context) {
    furi_assert(context);
    EventLoopMultiApp* app = context;

    /* Event loop créée obligatoirement dans son thread d’exécution */
    /* Création/libération worker dans le worker thread */
    EventLoopMultiAppWorker* worker = event_loop_multi_app_worker_alloc(app->stream_buffer);
    event_loop_multi_app_worker_run(worker);
    event_loop_multi_app_worker_free(worker);

    return 0;
}

/* Input callback depuis le contexte GUI */
static void event_loop_multi_app_input_callback(InputEvent* event, void* context) {
    furi_assert(context);
    EventLoopMultiApp* app = context;

    furi_check(furi_message_queue_put(app->input_queue, event, FuriWaitForever) == FuriStatusOk);
}

static void
    event_loop_multi_app_stream_buffer_callback(FuriEventLoopObject* object, void* context) {
    furi_assert(context);
    EventLoopMultiApp* app = context;

    furi_assert(object == app->stream_buffer);

    uint8_t data[STREAM_BUFFER_SIZE];
    /* Aucune attente dans les event handlers */
    furi_check(
        furi_stream_buffer_receive(app->stream_buffer, &data, sizeof(data), 0) == sizeof(data));

    FuriString* tmp_str = furi_string_alloc();
    for(uint32_t i = 0; i < sizeof(data); ++i) {
        furi_string_cat_printf(tmp_str, "%02X ", data[i]);
    }

    FURI_LOG_I(TAG, "Received data: %s", furi_string_get_cstr(tmp_str));
    furi_string_free(tmp_str);
}

static void event_loop_multi_app_input_queue_callback(FuriEventLoopObject* object, void* context) {
    furi_assert(context);
    EventLoopMultiApp* app = context;

    furi_assert(object == app->input_queue);

    InputEvent event;
    /* Aucune attente dans les event handlers */
    furi_check(furi_message_queue_get(app->input_queue, &event, 0) == FuriStatusOk);

    if(event.type == InputTypeLong) {

        if(event.key == InputKeyBack) {
            if(!furi_event_loop_timer_is_running(app->exit_timer)) {

                FURI_LOG_I(TAG, "Starting exit countdown!");
                furi_event_loop_timer_start(app->exit_timer, COUNTDOWN_INTERVAL_MS);

            } else {

                FURI_LOG_W(TAG, "Countdown has already been started");
            }

        } else if(event.key == InputKeyOk) {
            if(furi_event_loop_timer_is_running(app->exit_timer)) {

                FURI_LOG_I(TAG, "Exit countdown cancelled!");
                app->exit_countdown_value = COUNTDOWN_START_VALUE;
                furi_event_loop_timer_stop(app->exit_timer);

            } else {

                FURI_LOG_W(TAG, "Countdown has not been started yet");
            }

        } else {

            FURI_LOG_I(TAG, "Long press: %s", input_get_key_name(event.key));
        }

    } else if(event.type == InputTypeShort) {

        FURI_LOG_I(TAG, "Short press: %s", input_get_key_name(event.key));
    }
}

static void event_loop_multi_app_exit_timer_callback(void* context) {
    furi_assert(context);
    EventLoopMultiApp* app = context;

    FURI_LOG_I(TAG, "Exiting in %lu ...", app->exit_countdown_value);

    if(app->exit_countdown_value == 0) {
        FURI_LOG_I(TAG, "Exiting NOW!");

        /* Handler FuriSignalExit enregistré par défaut */
        furi_thread_signal(app->worker_thread, FuriSignalExit, NULL);

        furi_event_loop_stop(app->event_loop);

    } else {
        app->exit_countdown_value -= 1;
    }
}

static EventLoopMultiApp* event_loop_multi_app_alloc(void) {
    EventLoopMultiApp* app = malloc(sizeof(EventLoopMultiApp));

    app->event_loop = furi_event_loop_alloc();

    /* Event loop worker exécutée dans son thread */
    app->worker_thread = furi_thread_alloc_ex(
        "EventLoopMultiWorker", 1024, event_loop_multi_app_worker_thread, app);

    app->input_queue = furi_message_queue_alloc(INPUT_QUEUE_SIZE, sizeof(InputEvent));

    app->stream_buffer = furi_stream_buffer_alloc(STREAM_BUFFER_SIZE, STREAM_BUFFER_SIZE);

    app->exit_timer = furi_event_loop_timer_alloc(
        app->event_loop,
        event_loop_multi_app_exit_timer_callback,
        FuriEventLoopTimerTypePeriodic,
        app);

    app->gui = furi_record_open(RECORD_GUI);
    app->view_port = view_port_alloc();

    app->exit_countdown_value = COUNTDOWN_START_VALUE;
    /* Accès exclusif aux input events */
    view_port_input_callback_set(app->view_port, event_loop_multi_app_input_callback, app);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    furi_event_loop_subscribe_message_queue(
        app->event_loop,
        app->input_queue,
        FuriEventLoopEventIn,
        event_loop_multi_app_input_queue_callback,
        app);

    furi_event_loop_subscribe_stream_buffer(
        app->event_loop,
        app->stream_buffer,
        FuriEventLoopEventIn | FuriEventLoopEventFlagEdge,
        event_loop_multi_app_stream_buffer_callback,
        app);

    return app;
}

static void event_loop_multi_app_free(EventLoopMultiApp* app) {
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    /* Désabonnement obligatoire avant suppression event loop */
    /* Sinon, crash */
    furi_event_loop_unsubscribe(app->event_loop, app->input_queue);
    furi_event_loop_unsubscribe(app->event_loop, app->stream_buffer);

    view_port_free(app->view_port);
    furi_message_queue_free(app->input_queue);
    furi_stream_buffer_free(app->stream_buffer);
    /* Libérer tous les timers avant l’event loop associée */
    /* Sinon, crash */
    furi_event_loop_timer_free(app->exit_timer);
    furi_thread_free(app->worker_thread);
    furi_event_loop_free(app->event_loop);

    free(app);
}

static void event_loop_multi_app_run(EventLoopMultiApp* app) {
    FURI_LOG_I(TAG, "Press keys to see them printed here.");
    FURI_LOG_I(TAG, "Long press \"Back\" to exit after %lu seconds.", COUNTDOWN_START_VALUE);
    FURI_LOG_I(TAG, "Long press \"Ok\" to cancel the countdown.");

    furi_thread_start(app->worker_thread);
    /* Event loop bloquante jusqu’à la sortie application */
    furi_event_loop_run(app->event_loop);

    furi_thread_join(app->worker_thread);
}

int32_t example_event_loop_multi_app(void* arg) {
    UNUSED(arg);

    EventLoopMultiApp* app = event_loop_multi_app_alloc();
    event_loop_multi_app_run(app);
    event_loop_multi_app_free(app);

    return 0;
}
