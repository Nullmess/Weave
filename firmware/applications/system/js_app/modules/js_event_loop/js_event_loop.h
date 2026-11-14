#include "../../js_modules.h" // IWYU pragma: keep
#include <furi/core/event_loop.h>
#include <furi/core/event_loop_timer.h>

/* Modules intégrés via contracts JS ou FuriEventLoop directement */
/* Module responsable de créer, désabonner puis libérer objets */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct JsEventLoop JsEventLoop;

typedef enum {
    JsEventLoopObjectTypeTimer,
    JsEventLoopObjectTypeQueue,
    JsEventLoopObjectTypeMutex,
    JsEventLoopObjectTypeSemaphore,
    JsEventLoopObjectTypeStream,
} JsEventLoopObjectType;

typedef mjs_val_t (
    *JsEventLoopTransformer)(struct mjs* mjs, FuriEventLoopObject* object, void* context);

typedef struct {
    FuriEventLoopEvent event;
    JsEventLoopTransformer transformer;
    void* transformer_context;
} JsEventLoopNonTimerContract;

typedef struct {
    FuriEventLoopTimerType type;
    uint32_t interval_ticks;
} JsEventLoopTimerContract;

/* Contract timer : champ timer valide ; sinon non_timer valide */
/* non_timer.event transmis à event_loop_subscribe */
/* Transformer traduit objet en item JS */
/* Sans transformer : loop acquire sémaphores/mutex */
/* Producteur libère contract et objet à destruction interpréteur */

typedef struct {
    JsForeignMagic magic; // <! `JsForeignMagic_JsEventLoopContract`
    JsEventLoopObjectType object_type;
    FuriEventLoopObject* object;
    union {
        JsEventLoopNonTimerContract non_timer;
        JsEventLoopTimerContract timer;
    };
} JsEventLoopContract;

static_assert(offsetof(JsEventLoopContract, magic) == 0);

/* Importer event_loop avant module ; obtenir instance via js_module_get */

FuriEventLoop* js_event_loop_get_loop(JsEventLoop* loop);

#ifdef __cplusplus
}
#endif
