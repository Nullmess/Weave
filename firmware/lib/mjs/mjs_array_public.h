/*
 * Copyright (c) 2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * === Arrays
 */

#ifndef MJS_ARRAY_PUBLIC_H_
#define MJS_ARRAY_PUBLIC_H_

#include "mjs_core_public.h"

#if defined(__cplusplus)
extern "C" {
#endif

mjs_val_t mjs_mk_array(struct mjs* mjs);

/* 0 si arr n’est pas un array */
unsigned long mjs_array_length(struct mjs* mjs, mjs_val_t arr);

mjs_err_t mjs_array_push(struct mjs* mjs, mjs_val_t arr, mjs_val_t v);

/* Index hors limites : undefined */

mjs_val_t mjs_array_get(struct mjs*, mjs_val_t arr, unsigned long index);

mjs_err_t mjs_array_set(struct mjs* mjs, mjs_val_t arr, unsigned long index, mjs_val_t v);

int mjs_is_array(mjs_val_t v);

void mjs_array_del(struct mjs* mjs, mjs_val_t arr, unsigned long index);

#if defined(__cplusplus)
}
#endif

#endif
