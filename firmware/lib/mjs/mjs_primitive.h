/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_PRIMITIVE_H
#define MJS_PRIMITIVE_H

#include "mjs_primitive_public.h"
#include "mjs_internal.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* Pointeur invalide : crash mJS */

MJS_PRIVATE mjs_val_t mjs_legit_pointer_to_value(void* p);

/* Pointeur invalide : erreur dans contexte mJS */

MJS_PRIVATE mjs_val_t mjs_pointer_to_value(struct mjs* mjs, void* p);

MJS_PRIVATE void* get_ptr(mjs_val_t v);

MJS_PRIVATE void mjs_op_isnan(struct mjs* mjs);

MJS_PRIVATE void mjs_number_to_string(struct mjs* mjs);

#if defined(__cplusplus)
}
#endif

#endif
