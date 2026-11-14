/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_OBJECT_H_
#define MJS_OBJECT_H_

#include "mjs_object_public.h"
#include "mjs_internal.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct mjs;

struct mjs_property {
    struct mjs_property* next;
    mjs_val_t name;
    mjs_val_t value;
};

struct mjs_object {
    struct mjs_property* properties;
};

MJS_PRIVATE struct mjs_object* get_object_struct(mjs_val_t v);
MJS_PRIVATE struct mjs_property*
    mjs_get_own_property(struct mjs* mjs, mjs_val_t obj, const char* name, size_t len);

MJS_PRIVATE struct mjs_property*
    mjs_get_own_property_v(struct mjs* mjs, mjs_val_t obj, mjs_val_t key);

/* name non NULL prioritaire sur name_v */

MJS_PRIVATE mjs_err_t mjs_set_internal(
    struct mjs* mjs,
    mjs_val_t obj,
    mjs_val_t name_v,
    char* name,
    size_t name_len,
    mjs_val_t val);

MJS_PRIVATE void mjs_op_create_object(struct mjs* mjs);

MJS_PRIVATE void mjs_op_object_define_property(struct mjs* mjs);

MJS_PRIVATE void mjs_obj_destructor(struct mjs* mjs, void* cell);

#define MJS_PROTO_PROP_NAME "__p" /* Longueur doit être <5 caractères */

#if defined(__cplusplus)
}
#endif

#endif
