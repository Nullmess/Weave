#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void name_generator_make_auto(char* name, size_t max_name_size, const char* prefix);

void name_generator_make_random(char* name, size_t max_name_size);

void name_generator_make_detailed(char* name, size_t max_name_size, const char* prefix);

#ifdef __cplusplus
}
#endif
