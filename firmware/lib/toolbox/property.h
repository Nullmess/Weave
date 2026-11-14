#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <core/string.h>

/* last indique dernière paire key/value */

typedef void (*PropertyValueCallback)(const char* key, const char* value, bool last, void* context);

typedef struct {
    FuriString* key; /* Buffer clé à initialiser avant usage */
    FuriString* value; /* Buffer valeur à initialiser avant usage */
    PropertyValueCallback out; /**< output callback function */
    char sep; /**< separator character between key parts */
    bool last;
    void* context; /* Contexte utilisateur transmis au callback out */
} PropertyValueContext;

/* fmt NULL évite format ; key parts puis value dans varargs */

void property_value_out(PropertyValueContext* ctx, const char* fmt, unsigned int nparts, ...);

#ifdef __cplusplus
}
#endif
