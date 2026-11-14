/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_OBJECT_PUBLIC_H_
#define MJS_OBJECT_PUBLIC_H_

#include <stddef.h>
#include "mjs_core_public.h"
#include "mjs_ffi_public.h"

#if defined(__cplusplus)
extern "C" {
#endif

int mjs_is_object(mjs_val_t v);

int mjs_is_object_based(mjs_val_t v);

mjs_val_t mjs_mk_object(struct mjs* mjs);

enum mjs_struct_field_type {
    MJS_STRUCT_FIELD_TYPE_INVALID,
    MJS_STRUCT_FIELD_TYPE_STRUCT, /* Struct ; arg désigne définition */
    MJS_STRUCT_FIELD_TYPE_STRUCT_PTR, /* Pointeur struct ; arg désigne définition */
    MJS_STRUCT_FIELD_TYPE_INT,
    MJS_STRUCT_FIELD_TYPE_BOOL,
    MJS_STRUCT_FIELD_TYPE_DOUBLE,
    MJS_STRUCT_FIELD_TYPE_FLOAT,
    MJS_STRUCT_FIELD_TYPE_CHAR_PTR, /* String terminée par NUL */
    MJS_STRUCT_FIELD_TYPE_VOID_PTR, /* Converti en pointeur externe */
    MJS_STRUCT_FIELD_TYPE_MG_STR_PTR, /* Converti en string */
    MJS_STRUCT_FIELD_TYPE_MG_STR, /* Converti en string */
    MJS_STRUCT_FIELD_TYPE_DATA, /* Données converties en string ; arg désigne longueur */
    MJS_STRUCT_FIELD_TYPE_INT8,
    MJS_STRUCT_FIELD_TYPE_INT16,
    MJS_STRUCT_FIELD_TYPE_UINT8,
    MJS_STRUCT_FIELD_TYPE_UINT16,
    /* Fonction utilisateur : mjs_val_t depuis pointeur champ struct */

    MJS_STRUCT_FIELD_TYPE_CUSTOM,
};

/* Descripteur layout C pour mjs_struct_to_obj */
struct mjs_c_struct_member {
    const char* name;
    int offset;
    enum mjs_struct_field_type type;
    const void* arg;
};

mjs_val_t
    mjs_struct_to_obj(struct mjs* mjs, const void* base, const struct mjs_c_struct_member* members);

/* Propriété absente : undefined ; name_len=~0 : strlen */

mjs_val_t mjs_get(struct mjs* mjs, mjs_val_t obj, const char* name, size_t name_len);

mjs_val_t mjs_get_v(struct mjs* mjs, mjs_val_t obj, mjs_val_t name);

/* Recherche dans chaîne prototypes */

mjs_val_t mjs_get_v_proto(struct mjs* mjs, mjs_val_t obj, mjs_val_t key);

mjs_err_t mjs_set(struct mjs* mjs, mjs_val_t obj, const char* name, size_t len, mjs_val_t val);

mjs_err_t mjs_set_v(struct mjs* mjs, mjs_val_t obj, mjs_val_t name, mjs_val_t val);

/* Propriété propre uniquement ; name_len=~0 : strlen ; 0 succès, -1 erreur */

int mjs_del(struct mjs* mjs, mjs_val_t obj, const char* name, size_t len);

/* Initialiser iterator=MJS_UNDEFINED ; fin : MJS_UNDEFINED */
/* Ne pas modifier objet pendant itération */

mjs_val_t mjs_next(struct mjs* mjs, mjs_val_t obj, mjs_val_t* iterator);

typedef void (*mjs_custom_obj_destructor_t)(struct mjs* mjs, mjs_val_t object);

/* Destructor : pointeur foreign appelé avant libération objet */

#define MJS_DESTRUCTOR_PROP_NAME "__d"

#if defined(__cplusplus)
}
#endif

#endif
