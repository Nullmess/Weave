#pragma once

#include <furi.h>
#include "js_modules.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    // literal types
    JsValueTypeAny, //<! Literal term
    JsValueTypeAnyArray, /* Terme littéral validé comme array */
    JsValueTypeAnyObject, /* Terme littéral validé comme objet */
    JsValueTypeFunction, /* Terme littéral validé comme fonction */

    // primitive types
    JsValueTypeRawPointer, //<! Unchecked `void*`
    JsValueTypeInt32, /* Conversion Number vers int32_t */
    JsValueTypeDouble, /* Conversion Number vers double */
    JsValueTypeString, /* Conversion string vers const char* */
    JsValueTypeBool, /* Conversion Bool vers bool */

    JsValueTypeEnum, /* String parmi valeurs prédéfinies mapped vers enum C */
    JsValueTypeObject, /* Champs objet récursifs mapped vers valeurs C */

    JsValueTypeMask = 0xff,

    // enum sizes
    JsValueTypeEnumSize1 = (1 << 8),
    JsValueTypeEnumSize2 = (2 << 8),
    JsValueTypeEnumSize4 = (4 << 8),

    // flags
    JsValueTypePermitNull = (1 << 16), /* Valeur absente : assigner défaut */
} JsValueType;

#define JS_VALUE_TYPE_ENUM_SIZE(x) ((x) << 8)

typedef struct {
    const char* string_value;
    size_t num_value;
} JsValueEnumVariant;

typedef union {
    void* ptr_val;
    int32_t int32_val;
    double double_val;
    const char* str_val;
    size_t enum_val;
    bool bool_val;
} JsValueDefaultValue;

typedef struct JsValueObjectField JsValueObjectField;

typedef struct {
    JsValueType type;
    JsValueDefaultValue default_value;

    size_t n_children;
    union {
        const JsValueEnumVariant* enum_variants;
        const JsValueObjectField* object_fields;
    };
} JsValueDeclaration;

struct JsValueObjectField {
    const char* field_name;
    const JsValueDeclaration* value;
};

typedef struct {
    size_t n_children;
    const JsValueDeclaration* arguments;
} JsValueArguments;

#define JS_VALUE_ENUM(c_type, variants)                                    \
    {                                                                      \
        .type = JsValueTypeEnum | JS_VALUE_TYPE_ENUM_SIZE(sizeof(c_type)), \
        .n_children = COUNT_OF(variants),                                  \
        .enum_variants = variants,                                         \
    }

#define JS_VALUE_ENUM_W_DEFAULT(c_type, variants, default) \
    {                                                      \
        .type = JsValueTypeEnum | JsValueTypePermitNull |  \
                JS_VALUE_TYPE_ENUM_SIZE(sizeof(c_type)),   \
        .default_value.enum_val = default,                 \
        .n_children = COUNT_OF(variants),                  \
        .enum_variants = variants,                         \
    }

#define JS_VALUE_OBJECT(fields)         \
    {                                   \
        .type = JsValueTypeObject,      \
        .n_children = COUNT_OF(fields), \
        .object_fields = fields,        \
    }

#define JS_VALUE_OBJECT_W_DEFAULTS(fields)                 \
    {                                                      \
        .type = JsValueTypeObject | JsValueTypePermitNull, \
        .n_children = COUNT_OF(fields),                    \
        .object_fields = fields,                           \
    }

#define JS_VALUE_SIMPLE(t) {.type = t}

#define JS_VALUE_SIMPLE_W_DEFAULT(t, name, val) \
    {.type = (t) | JsValueTypePermitNull, .default_value.name = (val)}

#define JS_VALUE_ARGS(args)           \
    {                                 \
        .n_children = COUNT_OF(args), \
        .arguments = args,            \
    }

typedef enum {
    JsValueParseFlagNone = 0,
    JsValueParseFlagReturnOnError =
        (1
         << 0),
} JsValueParseFlag;

typedef enum {
    JsValueParseStatusOk, //<! Parsing completed successfully
    JsValueParseStatusJsError, /* Parsing échoué : input JS incorrect */
} JsValueParseStatus;

typedef enum {
    JsValueParseSourceValue,
    JsValueParseSourceArguments,
} JsValueParseSource;

typedef struct {
    JsValueParseSource source;
    union {
        const JsValueDeclaration* value_decl;
        const JsValueArguments* argument_decl;
    };
} JsValueParseDeclaration;

#define JS_VALUE_PARSE_SOURCE_VALUE(declaration) \
    ((JsValueParseDeclaration){.source = JsValueParseSourceValue, .value_decl = declaration})
#define JS_VALUE_PARSE_SOURCE_ARGS(declaration) \
    ((JsValueParseDeclaration){                 \
        .source = JsValueParseSourceArguments, .argument_decl = declaration})

size_t js_value_buffer_size(const JsValueParseDeclaration declaration);

/* Buffer temporaire dimensionné via js_value_buffer_size */
/* Valeurs invalides après fin de vie du buffer */
/* buf_size : nombre entrées, pas octets */
/* source NULL admis si declaration.source=Arguments */
/* Ordre valeurs C selon ordre déclaration */

JsValueParseStatus js_value_parse(
    struct mjs* mjs,
    const JsValueParseDeclaration declaration,
    JsValueParseFlag flags,
    mjs_val_t* buffer,
    size_t buf_size,
    mjs_val_t* source,
    size_t n_c_vals,
    ...);

#define JS_VALUE_PARSE(mjs, declaration, flags, status_ptr, value_ptr, ...) \
    void* _args[] = {__VA_ARGS__};                                          \
    size_t _n_args = COUNT_OF(_args);                                       \
    size_t _temp_buf_len = js_value_buffer_size(declaration);               \
    mjs_val_t _temp_buffer[_temp_buf_len];                                  \
    *(status_ptr) = js_value_parse(                                         \
        mjs, declaration, flags, _temp_buffer, _temp_buf_len, value_ptr, _n_args, __VA_ARGS__);

#define JS_VALUE_PARSE_ARGS_OR_RETURN(mjs, declaration, ...) \
    JsValueParseStatus _status;                              \
    JS_VALUE_PARSE(                                          \
        mjs,                                                 \
        JS_VALUE_PARSE_SOURCE_ARGS(declaration),             \
        JsValueParseFlagReturnOnError,                       \
        &_status,                                            \
        NULL,                                                \
        __VA_ARGS__);                                        \
    if(_status != JsValueParseStatusOk) return;

#ifdef __cplusplus
}
#endif
