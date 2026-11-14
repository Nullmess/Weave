#include "../../js_modules.h"
#include <gui/view.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    JsViewPropTypeString,
    JsViewPropTypeNumber,
    JsViewPropTypeArr,
    JsViewPropTypeTypedArr,
    JsViewPropTypeBool,
} JsViewPropType;

typedef union {
    const char* string;
    int32_t number;
    bool boolean;
    mjs_val_t term;
} JsViewPropValue;

/* Correspondance enum fonts JS/C */

extern const JsValueDeclaration js_gui_font_declaration;

/* Nom/type property définis implicitement par descriptor */

typedef bool (
    *JsViewPropAssign)(struct mjs* mjs, void* specific_view, JsViewPropValue value, void* context);

/** @brief Property descriptor */
typedef struct {
    const char* name; /* Nom property visible depuis JS */
    JsViewPropType type; /* Type property garanti par module GUI */
    JsViewPropAssign assign; // <! Property assignment callback
} JsViewPropDescriptor;

// View method signatures

/** @brief View's `_alloc` method */
typedef void* (*JsViewAlloc)(void);
/** @brief View's `_get_view` method */
typedef View* (*JsViewGetView)(void* specific_view);
/** @brief View's `_free` method */
typedef void (*JsViewFree)(void* specific_view);

// Glue code method signatures

typedef void* (*JsViewCustomMake)(struct mjs* mjs, void* specific_view, mjs_val_t view_obj);

typedef void (*JsViewCustomDestroy)(void* specific_view, void* custom_state, FuriEventLoop* loop);

typedef bool (
    *JsViewAddChild)(struct mjs* mjs, void* specific_view, void* custom_state, mjs_val_t child_obj);

typedef void (*JsViewResetChildren)(void* specific_view, void* custom_state);

/* Descriptor : méthodes view, ctor/dtor custom, properties JS */

typedef struct {
    JsViewAlloc alloc;
    JsViewGetView get_view;
    JsViewFree free;

    JsViewCustomMake custom_make; /* Peut être NULL */
    JsViewCustomDestroy custom_destroy; /* Peut être NULL */

    JsViewAddChild add_child; /* Peut être NULL */
    JsViewResetChildren reset_children; /* Peut être NULL */

    size_t prop_cnt; /* Nombre properties visibles depuis JS */
    JsViewPropDescriptor props[]; /* Descriptors properties visibles depuis JS */
} JsViewDescriptor;

// Callback ordering:
//                                   +-> add_child       -+
//                                   +-> reset_children  -+
// alloc -> get_view -> custom_make -+-> props[i].assign -+> custom_destroy -> free
// \__________ creation __________/      \____ use ____/     \___ destruction ____/

/* Factory via JS_GUI_VIEW_DEF ; make/makeWith retournent View */

mjs_val_t js_gui_make_view_factory(struct mjs* mjs, const JsViewDescriptor* view_descriptor);

#define JS_GUI_VIEW_DEF(name, descriptor)                                                \
    static void* view_mod_ctor(struct mjs* mjs, mjs_val_t* object, JsModules* modules) { \
        UNUSED(modules);                                                                 \
        *object = js_gui_make_view_factory(mjs, descriptor);                             \
        return NULL;                                                                     \
    }                                                                                    \
    static const JsModuleDescriptor js_mod_desc = {                                      \
        "gui__" #name,                                                                   \
        view_mod_ctor,                                                                   \
        NULL,                                                                            \
        NULL,                                                                            \
    };                                                                                   \
    static const FlipperAppPluginDescriptor plugin_descriptor = {                        \
        .appid = PLUGIN_APP_ID,                                                          \
        .ep_api_version = PLUGIN_API_VERSION,                                            \
        .entry_point = &js_mod_desc,                                                     \
    };                                                                                   \
    const FlipperAppPluginDescriptor* js_view_##name##_ep(void) {                        \
        return &plugin_descriptor;                                                       \
    }

#ifdef __cplusplus
}
#endif
