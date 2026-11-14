/*
 * Copyright (c) 2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_FFI_H_
#define MJS_FFI_H_

#include "ffi/ffi.h"
#include "mjs_ffi_public.h"
#include "mjs_internal.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define MJS_CB_ARGS_MAX_CNT 6
#define MJS_CB_SIGNATURE_MAX_SIZE (MJS_CB_ARGS_MAX_CNT + 1  )

typedef uint8_t mjs_ffi_ctype_t;

enum ffi_sig_type {
    FFI_SIG_FUNC,
    FFI_SIG_CALLBACK,
};

struct mjs_ffi_sig {
    /* NOTE(dfrank): première entrée pointeur requise pour bits GC */
    /* Signatures regroupées en arenas GC */
    /* TODO(dfrank): supporter plusieurs paires callback/userdata */

    struct mjs_ffi_sig* cb_sig;

    /* Élément 0 : type retour ; NONE termine arguments ou désigne void */

    mjs_ffi_ctype_t val_types[MJS_CB_SIGNATURE_MAX_SIZE];

    /* fn : dlsym ou implémentation callback selon is_callback */

    ffi_fn_t* fn;

    int8_t args_cnt;

    /* is_callback : fn désigne implémentation callback */

    unsigned is_callback : 1;
    unsigned is_valid : 1;
};
typedef struct mjs_ffi_sig mjs_ffi_sig_t;

MJS_PRIVATE void mjs_ffi_sig_init(mjs_ffi_sig_t* sig);

MJS_PRIVATE void mjs_ffi_sig_copy(mjs_ffi_sig_t* to, const mjs_ffi_sig_t* from);
/* NOTE: sig lui-même n’est pas libéré */
MJS_PRIVATE void mjs_ffi_sig_free(mjs_ffi_sig_t* sig);

/* Signature allouée dans arena GC, encapsulée en mjs_val_t */

MJS_PRIVATE mjs_val_t mjs_mk_ffi_sig(struct mjs* mjs);

MJS_PRIVATE int mjs_is_ffi_sig(mjs_val_t v);

MJS_PRIVATE mjs_val_t mjs_ffi_sig_to_value(struct mjs_ffi_sig* psig);

MJS_PRIVATE struct mjs_ffi_sig* mjs_get_ffi_sig_struct(mjs_val_t v);

MJS_PRIVATE void mjs_ffi_sig_destructor(struct mjs* mjs, void* psig);

MJS_PRIVATE int mjs_ffi_sig_set_val_type(mjs_ffi_sig_t* sig, int idx, mjs_ffi_ctype_t type);
MJS_PRIVATE int
    mjs_ffi_sig_validate(struct mjs* mjs, mjs_ffi_sig_t* sig, enum ffi_sig_type sig_type);
MJS_PRIVATE int mjs_ffi_is_regular_word(mjs_ffi_ctype_t type);
MJS_PRIVATE int mjs_ffi_is_regular_word_or_void(mjs_ffi_ctype_t type);

struct mjs_ffi_cb_args {
    struct mjs_ffi_cb_args* next;
    struct mjs* mjs;
    mjs_ffi_sig_t sig;
    mjs_val_t func;
    mjs_val_t userdata;
};
typedef struct mjs_ffi_cb_args ffi_cb_args_t;

MJS_PRIVATE mjs_err_t mjs_ffi_call(struct mjs* mjs);

MJS_PRIVATE mjs_err_t mjs_ffi_call2(struct mjs* mjs);

MJS_PRIVATE void mjs_ffi_cb_free(struct mjs*);
MJS_PRIVATE void mjs_ffi_args_free_list(struct mjs* mjs);

#if defined(__cplusplus)
}
#endif

#endif
