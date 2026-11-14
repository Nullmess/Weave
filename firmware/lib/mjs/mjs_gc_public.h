/*
 * Copyright (c) 2014 Cesanta Software Limited
 * All rights reserved
 */

#ifndef MJS_GC_PUBLIC_H_
#define MJS_GC_PUBLIC_H_

#include "mjs_core_public.h"

#if defined(__cplusplus)
extern "C" {
#endif

/* full=true : restituer heap inutilisée au système */

void mjs_gc(struct mjs* mjs, int full);

#if defined(__cplusplus)
}
#endif

#endif
