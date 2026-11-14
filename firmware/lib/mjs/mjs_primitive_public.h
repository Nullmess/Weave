/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_PRIMITIVE_PUBLIC_H_
#define MJS_PRIMITIVE_PUBLIC_H_

#include "mjs_core_public.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* Valeur JavaScript null */
#define MJS_NULL MJS_TAG_NULL

/* Valeur JavaScript undefined */
#define MJS_UNDEFINED MJS_TAG_UNDEFINED

#define MJS_MK_FN(fn) mjs_mk_foreign_func(mjs, (mjs_func_ptr_t)fn)

typedef void (*mjs_func_ptr_t)(void);

/* Obsolète : utiliser MJS_NULL */

mjs_val_t mjs_mk_null(void);

int mjs_is_null(mjs_val_t v);

/* Obsolète : utiliser MJS_UNDEFINED */

mjs_val_t mjs_mk_undefined(void);

int mjs_is_undefined(mjs_val_t v);

mjs_val_t mjs_mk_number(struct mjs* mjs, double num);

/* NaN pour valeurs non numériques */

double mjs_get_double(struct mjs* mjs, mjs_val_t v);

/* Fraction tronquée ; résultat indéfini pour non-numérique/NaN */

int mjs_get_int(struct mjs* mjs, mjs_val_t v);

/* Garantit entier signé 32 bits */

int32_t mjs_get_int32(struct mjs* mjs, mjs_val_t v);

int mjs_is_number(mjs_val_t v);

/* Foreign opaque ; pointeurs valides architecture requis */
/* Pointeurs 64 bits sur 48 bits avec extension du signe */
/* Données brutes >=8 octets : utiliser byte arrays */

mjs_val_t mjs_mk_foreign(struct mjs* mjs, void* ptr);

mjs_val_t mjs_mk_foreign_func(struct mjs* mjs, mjs_func_ptr_t fn);

/* NULL si valeur non foreign */

void* mjs_get_ptr(struct mjs* mjs, mjs_val_t v);

int mjs_is_foreign(mjs_val_t v);

mjs_val_t mjs_mk_boolean(struct mjs* mjs, int v);
int mjs_get_bool(struct mjs* mjs, mjs_val_t v);
int mjs_is_boolean(mjs_val_t v);

mjs_val_t mjs_mk_function(struct mjs* mjs, size_t off);
int mjs_is_function(mjs_val_t v);

#if defined(__cplusplus)
}
#endif

#endif
