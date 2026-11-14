#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Index premier élément le plus proche ; 0 si aucun */

size_t value_index_int32(const int32_t value, const int32_t values[], size_t values_count);

/* Index premier élément le plus proche ; 0 si aucun */

size_t value_index_uint32(const uint32_t value, const uint32_t values[], size_t values_count);

/* Index premier élément le plus proche ; 0 si aucun */

size_t value_index_float(const float value, const float values[], size_t values_count);

/* Index premier élément égal ; 0 si aucun */

size_t value_index_bool(const bool value, const bool values[], size_t values_count);

#ifdef __cplusplus
}
#endif
