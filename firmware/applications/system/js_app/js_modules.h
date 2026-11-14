#pragma once

#include <stdint.h>
#include "js_thread_i.h"
#include "js_value.h"
#include <flipper_application/flipper_application.h>
#include <flipper_application/plugins/plugin_manager.h>
#include <flipper_application/plugins/composite_resolver.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PLUGIN_APP_ID      "js"
#define PLUGIN_API_VERSION 1

#define JS_SDK_VENDOR "flipperdevices"
#define JS_SDK_MAJOR  1
#define JS_SDK_MINOR  0

/* Pointeur foreign dans obj["_"] */

#define JS_GET_INST(mjs, obj) mjs_get_ptr(mjs, mjs_get(mjs, obj, INST_PROP_NAME, ~0))
/* Pointeur foreign dans this["_"] */

#define JS_GET_CONTEXT(mjs)   JS_GET_INST(mjs, mjs_get_this(mjs))

/* JS_ASSIGN_MULTI construit un objet par champs */

#define JS_ASSIGN_MULTI(mjs, object)     \
    for(struct {                         \
            struct mjs* mjs;             \
            mjs_val_t val;               \
            int i;                       \
        } _ass_multi = {mjs, object, 0}; \
        _ass_multi.i == 0;               \
        _ass_multi.i++)
#define JS_FIELD(name, value) mjs_set(_ass_multi.mjs, _ass_multi.val, name, ~0, value)

/* Magic obligatoirement premier word pour validation struct pointer */

typedef enum {
    JsForeignMagicStart = 0x15BAD000,
    JsForeignMagic_JsEventLoopContract,
} JsForeignMagic;

/* Macro définit erreur/undefined et exécute return */

#define JS_ERROR_AND_RETURN(mjs, error_code, ...)         \
    do {                                                  \
        mjs_prepend_errorf(mjs, error_code, __VA_ARGS__); \
        mjs_return(mjs, MJS_UNDEFINED);                   \
        return;                                           \
    } while(0)

/* Macro définit erreur/undefined et exécute return */

#define JS_ERROR_AND_RETURN_VAL(mjs, error_code, ret_val, ...) \
    do {                                                       \
        mjs_prepend_errorf(mjs, error_code, __VA_ARGS__);      \
        mjs_return(mjs, MJS_UNDEFINED);                        \
        return ret_val;                                        \
    } while(0)

typedef struct JsModules JsModules;

typedef void* (*JsModuleConstructor)(struct mjs* mjs, mjs_val_t* object, JsModules* modules);
typedef void (*JsModuleDestructor)(void* inst);

typedef struct {
    char* name;
    JsModuleConstructor create;
    JsModuleDestructor destroy;
    const ElfApiInterface* api_interface;
} JsModuleDescriptor;

JsModules* js_modules_create(struct mjs* mjs, CompositeApiResolver* resolver);

void js_modules_destroy(JsModules* modules);

mjs_val_t js_module_require(JsModules* modules, const char* name, size_t name_len);

/* NULL si module non instancié */

void* js_module_get(JsModules* modules, const char* name);

/**
 * @brief `sdkCompatibilityStatus` function
 */
void js_sdk_compatibility_status(struct mjs* mjs);

/**
 * @brief `isSdkCompatible` function
 */
void js_is_sdk_compatible(struct mjs* mjs);

/**
 * @brief `checkSdkCompatibility` function
 */
void js_check_sdk_compatibility(struct mjs* mjs);

/**
 * @brief `doesSdkSupport` function
 */
void js_does_sdk_support(struct mjs* mjs);

/**
 * @brief `checkSdkFeatures` function
 */
void js_check_sdk_features(struct mjs* mjs);

#ifdef __cplusplus
}
#endif
