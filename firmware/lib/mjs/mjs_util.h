/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_UTIL_H_
#define MJS_UTIL_H_

#include "common/frozen/frozen.h"
#include "mjs_core.h"
#include "mjs_util_public.h"

#if defined(__cplusplus)
extern "C" {
#endif

struct mjs_bcode_part;

#if MJS_ENABLE_DEBUG
MJS_PRIVATE const char* opcodetostr(uint8_t opcode);
MJS_PRIVATE size_t mjs_disasm_single(const uint8_t* code, size_t i);
#endif

MJS_PRIVATE const char* mjs_stringify_type(enum mjs_type t);

/* arg_num négatif : vérifier this ; sinon argument */
/* Échec : erreur et retour 0 ; succès : 1 */
/* arg_name pour message ; parg non NULL reçoit valeur */

MJS_PRIVATE int mjs_check_arg(
    struct mjs* mjs,
    int arg_num,
    const char* arg_name,
    enum mjs_type expected_type,
    mjs_val_t* parg);

/* Index borné à [0,size] ; négatif devient size+index */

MJS_PRIVATE int mjs_normalize_idx(int idx, int size);

MJS_PRIVATE const char* mjs_get_bcode_filename(struct mjs* mjs, struct mjs_bcode_part* bp);

void mjs_jprintf(mjs_val_t v, struct mjs* mjs, struct json_out* out);

#if defined(__cplusplus)
}
#endif

#endif
