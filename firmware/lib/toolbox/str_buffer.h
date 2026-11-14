/* Copies strings owned, append-only ; libération groupée */

#pragma once

#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Struct intégrable directement sans alloc/free propre */

typedef struct {
    char** owned_strings;
    size_t n_owned_strings;
} StrBuffer;

/* Copie owned reste valide après fin source */

const char* str_buffer_make_owned_clone(StrBuffer* buffer, const char* str);

void str_buffer_clear_all_clones(StrBuffer* buffer);

#ifdef __cplusplus
}
#endif
