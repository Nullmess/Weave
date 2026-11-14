/*
 * Copyright (c) 2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_CORE_PUBLIC_H_
#define MJS_CORE_PUBLIC_H_

#if !defined(_MSC_VER) || _MSC_VER >= 1700
#include <stdint.h>
#else
typedef unsigned __int64 uint64_t;
typedef int int32_t;
typedef unsigned char uint8_t;
#endif
#include <stdio.h>
#include <stddef.h>
#include "mjs_features.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef MJS_ENABLE_DEBUG
#define MJS_ENABLE_DEBUG 0
#endif

/* IEEE754 : 64 bits, signe 1, exposant 11, mantisse 52 */
/* Exposant tout 1 : mantisse zéro infinity ; MSB mantisse 1 quiet NaN */
/* Packing mJS : marqueur 0xFFF, tag 4 bits non nul, valeur 48 bits */
/* Pointeurs 64 bits limités à 48 bits, sign extension requise */

typedef uint64_t mjs_val_t;

/* Tag signe+4 bits ; combinaison (1,0) inutilisée pour éviter infinity */

#define MAKE_TAG(s, t) ((uint64_t)(s) << 63 | (uint64_t)0x7ff0 << 48 | (uint64_t)(t) << 48)

#define MJS_TAG_OBJECT MAKE_TAG(1, 1)
#define MJS_TAG_FOREIGN MAKE_TAG(1, 2)
#define MJS_TAG_UNDEFINED MAKE_TAG(1, 3)
#define MJS_TAG_BOOLEAN MAKE_TAG(1, 4)
#define MJS_TAG_NAN MAKE_TAG(1, 5)
#define MJS_TAG_STRING_I MAKE_TAG(1, 6) /* String inline : longueur <5 */
#define MJS_TAG_STRING_5 MAKE_TAG(1, 7) /* String inline : longueur 5 */
#define MJS_TAG_STRING_O MAKE_TAG(1, 8) /* String possédée */
#define MJS_TAG_STRING_F MAKE_TAG(1, 9) /* String externe */
#define MJS_TAG_STRING_C MAKE_TAG(1, 10)
#define MJS_TAG_STRING_D MAKE_TAG(1, 11)
#define MJS_TAG_ARRAY MAKE_TAG(1, 12)
#define MJS_TAG_FUNCTION MAKE_TAG(1, 13)
#define MJS_TAG_FUNCTION_FFI MAKE_TAG(1, 14)
#define MJS_TAG_NULL MAKE_TAG(1, 15)

#define MJS_TAG_ARRAY_BUF MAKE_TAG(0, 1)
#define MJS_TAG_ARRAY_BUF_VIEW MAKE_TAG(0, 2)

#define MJS_TAG_MASK MAKE_TAG(1, 15)

/* Bloc if-0 conservé pour indexation etags de struct mjs */
#if 0

struct mjs {

};
#endif

struct mjs;

enum mjs_type {

    MJS_TYPE_UNDEFINED,
    MJS_TYPE_NULL,
    MJS_TYPE_BOOLEAN,
    MJS_TYPE_NUMBER,
    MJS_TYPE_STRING,
    MJS_TYPE_FOREIGN,
    MJS_TYPE_ARRAY_BUF,
    MJS_TYPE_ARRAY_BUF_VIEW,

    MJS_TYPE_OBJECT_GENERIC,
    MJS_TYPE_OBJECT_ARRAY,
    MJS_TYPE_OBJECT_FUNCTION,
    /* TODO(dfrank): ajouter éléments pour prototypes si supportés */

    MJS_TYPES_CNT
};

typedef enum mjs_err {
    MJS_OK,
    MJS_SYNTAX_ERROR,
    MJS_REFERENCE_ERROR,
    MJS_TYPE_ERROR,
    MJS_OUT_OF_MEMORY,
    MJS_INTERNAL_ERROR,
    MJS_NOT_IMPLEMENTED_ERROR,
    MJS_FILE_READ_ERROR,
    MJS_BAD_ARGS_ERROR,

    MJS_NEED_EXIT,

    MJS_ERRS_CNT
} mjs_err_t;

typedef void (*mjs_flags_poller_t)(struct mjs* mjs);

struct mjs;

struct mjs* mjs_create(void* context);

void mjs_destroy(struct mjs* mjs);

mjs_val_t mjs_get_global(struct mjs* mjs);

/* mjs_own requis pour valeurs survivant exec/call ou callback C */
/* Initialiser mjs_val_t avant own ; sinon comportement indéfini */
/* Registration protège contre GC ; disown avant fin de vie/libération */

void mjs_own(struct mjs* mjs, mjs_val_t* v);

/* Renvoie 1 si valeur trouvée ; sinon 0 */

int mjs_disown(struct mjs* mjs, mjs_val_t* v);

mjs_err_t mjs_set_errorf(struct mjs* mjs, mjs_err_t err, const char* fmt, ...);

void mjs_exit(struct mjs* mjs);

void mjs_set_exec_flags_poller(struct mjs* mjs, mjs_flags_poller_t poller);

void* mjs_get_context(struct mjs* mjs);

/* Préfixe nouveau message avec deux-points ; code erreur initial conservé */

mjs_err_t mjs_prepend_errorf(struct mjs* mjs, mjs_err_t err, const char* fmt, ...);

/* msg=NULL : MJS error ; print_stack_trace non nul ajoute trace */

void mjs_print_error(struct mjs* mjs, FILE* fp, const char* msg, int print_stack_trace);

/* String erreur susceptible d’être remplacée par set_errorf */

const char* mjs_strerror(struct mjs* mjs, enum mjs_err err);

const char* mjs_get_stack_trace(struct mjs* mjs);

/* Génération jsc désactivée par défaut ; nécessite GENERATE_JSC et MMAP */

void mjs_set_generate_jsc(struct mjs* mjs, int generate_jsc);

/* Depuis cfunction : nombre arguments JS courants */

int mjs_nargs(struct mjs* mjs);

/* Depuis cfunction : argument JS d’index n */

mjs_val_t mjs_arg(struct mjs* mjs, int n);

void mjs_return(struct mjs* mjs, mjs_val_t v);

#if defined(__cplusplus)
}
#endif

#endif
