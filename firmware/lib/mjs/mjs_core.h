/*
 * Copyright (c) 2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_CORE_H
#define MJS_CORE_H

#include "mjs_ffi.h"
#include "mjs_gc.h"
#include "mjs_internal.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define JUMP_INSTRUCTION_SIZE 2

enum mjs_call_stack_frame_item {
    CALL_STACK_FRAME_ITEM_RETVAL_STACK_IDX, /* TOS */
    CALL_STACK_FRAME_ITEM_LOOP_ADDR_IDX,
    CALL_STACK_FRAME_ITEM_SCOPE_IDX,
    CALL_STACK_FRAME_ITEM_RETURN_ADDR,
    CALL_STACK_FRAME_ITEM_THIS,

    CALL_STACK_FRAME_ITEMS_CNT
};

struct mjs_vals {
    /* Valeur this courante */
    mjs_val_t this_obj;
    mjs_val_t dataview_proto;

    /* Objet du dernier OP_GET pour invocation méthode */

    mjs_val_t last_getprop_obj;
};

struct mjs_bcode_part {
    /* Index bytecode global */
    size_t start_idx;

    struct {
        const char* p; /* Pointer de bloc mémoire */
        size_t len;
    } data;

    /* Évaluation réussie MJS_OK évite réexécution ; parsing invalide jamais commité */

    mjs_err_t exec_res : 4;

    /* Actif : bytecode ne doit pas être libéré */
    unsigned in_rom : 1;
};

struct mjs {
    struct mbuf bcode_gen;
    struct mbuf bcode_parts;
    size_t bcode_len;
    struct mbuf stack;
    struct mbuf call_stack;
    struct mbuf arg_stack;
    struct mbuf scopes;
    struct mbuf loop_addresses;
    struct mbuf owned_strings; /* Séquence (varint len, char data[]) */
    struct mbuf foreign_strings; /* Séquence (varint len, char* data) */
    struct mbuf owned_values;
    struct mbuf json_visited_stack;
    struct mbuf array_buffers;
    struct mjs_vals vals;
    char* error_msg;
    char* stack_trace;
    enum mjs_err error;
    mjs_ffi_resolver_t* dlsym;
    void* dlsym_handle;
    ffi_cb_args_t* ffi_cb_args;
    size_t cur_bcode_offset;
    mjs_flags_poller_t exec_flags_poller;
    void* context;

    struct gc_arena object_arena;
    struct gc_arena property_arena;
    struct gc_arena ffi_sig_arena;

    unsigned inhibit_gc : 1;
    unsigned need_gc : 1;
    unsigned generate_jsc : 1;
};

typedef uint32_t mjs_header_item_t;
enum mjs_header_items {
    MJS_HDR_ITEM_TOTAL_SIZE, /* Taille bytecode hors octet OP_BCODE_HEADER */

    MJS_HDR_ITEM_BCODE_OFFSET, /* Offset bytecode hors octet OP_BCODE_HEADER */

    MJS_HDR_ITEM_MAP_OFFSET, /* Offset mapping offset→line_no */

    MJS_HDR_ITEMS_CNT
};

MJS_PRIVATE size_t mjs_get_func_addr(mjs_val_t v);

MJS_PRIVATE int mjs_getretvalpos(struct mjs* mjs);

MJS_PRIVATE enum mjs_type mjs_get_type(mjs_val_t v);

/* Trace depuis offset donné puis offsets call_stack */

MJS_PRIVATE void mjs_gen_stack_trace(struct mjs* mjs, size_t offset);

MJS_PRIVATE mjs_val_t vtop(struct mbuf* m);
MJS_PRIVATE size_t mjs_stack_size(const struct mbuf* m);
MJS_PRIVATE mjs_val_t* vptr(struct mbuf* m, int idx);
MJS_PRIVATE void push_mjs_val(struct mbuf* m, mjs_val_t v);
MJS_PRIVATE mjs_val_t mjs_pop_val(struct mbuf* m);
MJS_PRIVATE mjs_val_t mjs_pop(struct mjs* mjs);
MJS_PRIVATE void mjs_push(struct mjs* mjs, mjs_val_t v);
MJS_PRIVATE void mjs_die(struct mjs* mjs);

#if defined(__cplusplus)
}
#endif

#endif
