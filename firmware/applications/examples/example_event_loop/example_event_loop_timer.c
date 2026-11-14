/* Timer périodique : décompte 10..0 ; timers multiples possibles */
#include <furi.h>

#define TAG "ExampleEventLoopTimer"

#define COUNTDOWN_START_VALUE (10)
#define COUNTDOWN_INTERVAL_MS (1000)

typedef struct {
    FuriEventLoop* event_loop;
    FuriEventLoopTimer* timer;
    uint32_t countdown_value;
} EventLoopTimerApp;

/* Callback expiration chaque seconde */
static void event_loop_timer_callback(void* context) {
    furi_assert(context);
    EventLoopTimerApp* app = context;

    FURI_LOG_I(TAG, "T-00:00:%02lu", app->countdown_value);

    if(app->countdown_value == 0) {

        FURI_LOG_I(TAG, "Blast off to adventure!");

        furi_event_loop_stop(app->event_loop);

    } else {

        app->countdown_value -= 1;
    }
}

static EventLoopTimerApp* event_loop_timer_app_alloc(void) {
    EventLoopTimerApp* app = malloc(sizeof(EventLoopTimerApp));

    app->event_loop = furi_event_loop_alloc();

    /* Timer lié au contexte de l’event loop */
    /* Timer périodique : redémarrage automatique après expiration */
    app->timer = furi_event_loop_timer_alloc(
        app->event_loop, event_loop_timer_callback, FuriEventLoopTimerTypePeriodic, app);

    app->countdown_value = COUNTDOWN_START_VALUE;

    return app;
}

static void event_loop_timer_app_free(EventLoopTimerApp* app) {
    /* Libérer tous les timers avant suppression de l’event loop */
    /* Sinon, crash */
    furi_event_loop_timer_free(app->timer);

    furi_event_loop_free(app->event_loop);
    free(app);
}

static void event_loop_timer_app_run(EventLoopTimerApp* app) {
    FURI_LOG_I(TAG, "All systems go! Prepare for countdown!");

    /* Timers démarrables avant run ou depuis un callback */

    furi_event_loop_timer_start(app->timer, COUNTDOWN_INTERVAL_MS);
    /* Bloquant jusqu’à furi_event_loop_stop */
    furi_event_loop_run(app->event_loop);
}

int32_t example_event_loop_timer_app(void* arg) {
    UNUSED(arg);

    EventLoopTimerApp* app = event_loop_timer_app_alloc();
    event_loop_timer_app_run(app);
    event_loop_timer_app_free(app);

    return 0;
}
