#include "wrappers.h"

/* Désassemblage : locale NULL accepté */
/* libc_nano strtof/strtod utilisent mêmes fonctions */
/* Locale explicite économisée : 364 octets data */

float __wrap_strtof(const char* in, char** tail) {
    return strtof_l(in, tail, NULL);
}

double __wrap_strtod(const char* in, char** tail) {
    return strtod_l(in, tail, NULL);
}
