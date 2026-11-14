/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_STRING_PUBLIC_H_
#define MJS_STRING_PUBLIC_H_

#include "mjs_core_public.h"

#define MJS_STRING_LITERAL_MAX_LEN 128

#if defined(__cplusplus)
extern "C" {
#endif

/* str UTF-8 ; len=~0 : strlen */
/* copy non nul : GC possède copie ; source libérable */
/* copy nul : appelant conserve source tant qu’utilisée */

mjs_val_t mjs_mk_string(struct mjs* mjs, const char* str, size_t len, int copy);

int mjs_is_string(mjs_val_t v);

/* len peut être NULL ; NULL si valeur non-string */
/* NUL internes possibles ; terminaison non garantie */
/* Allocation JS peut déclencher GC et déplacer données */
/* Strings courtes inline : données invalides après fin de vie mjs_val_t */

const char* mjs_get_string(struct mjs* mjs, mjs_val_t* v, size_t* len);

/* Compatible C : un seul NUL, uniquement final */
/* Strings owned toujours terminées ; NULL si incompatibles */

const char* mjs_get_cstring(struct mjs* mjs, mjs_val_t* v);

/* Retour strcmp ; égalité exige longueurs identiques, pas strncmp */

int mjs_strcmp(struct mjs* mjs, mjs_val_t* a, const char* b, size_t len);

#if defined(__cplusplus)
}
#endif

#endif
