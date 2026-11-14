/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_UTIL_PUBLIC_H_
#define MJS_UTIL_PUBLIC_H_

#include "mjs_core_public.h"
#include <stdio.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef void (*MjsPrintCallback)(void* ctx, const char* format, ...);

const char* mjs_typeof(mjs_val_t v);

void mjs_fprintf(mjs_val_t v, struct mjs* mjs, FILE* fp);
void mjs_sprintf(mjs_val_t v, struct mjs* mjs, char* buf, size_t buflen);

void mjs_disasm_all(struct mjs* mjs, MjsPrintCallback print_cb, void* print_ctx);
void mjs_dump(struct mjs* mjs, int do_disasm, MjsPrintCallback print_cb, void* print_ctx);

const char* mjs_get_bcode_filename_by_offset(struct mjs* mjs, int offset);

int mjs_get_lineno_by_offset(struct mjs* mjs, int offset);

/* Frame 0 courant, 1 premier retour ; -1 si hors limites */

int mjs_get_offset_by_call_frame_num(struct mjs* mjs, int cf_num);

/* MJS_OK si succès ; p/sizep reçoivent string */
/* need_free non NULL requis ; free si non nul */
/* Objets : MJS_TYPE_ERROR, sans toString/valueOf */

mjs_err_t mjs_to_string(struct mjs* mjs, mjs_val_t* v, char** p, size_t* sizep, int* need_free);

mjs_val_t mjs_to_boolean_v(struct mjs* mjs, mjs_val_t v);

int mjs_is_truthy(struct mjs* mjs, mjs_val_t v);

#if defined(__cplusplus)
}
#endif

#endif
