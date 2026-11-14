
#pragma once

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "check.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Test : link contre furi memmgr */
#define FURI_MEMMGR_GUARD 1

/* Taille libre en bytes */
size_t memmgr_get_free_heap(void);

/* Taille totale en bytes */
size_t memmgr_get_total_heap(void);

/* Minimum de heap libre en bytes */
size_t memmgr_get_minimum_free_heap(void);

/* Allocation alignée : libération uniquement via aligned_free */
void* aligned_malloc(size_t size, size_t alignment);

/* Pointeur obtenu uniquement via aligned_malloc */
void aligned_free(void* p);

/* Pool séparé : allocation non libérable */
void* memmgr_alloc_from_pool(size_t size);

size_t memmgr_pool_get_free(void);

size_t memmgr_pool_get_max_block(void);

#ifdef __cplusplus
}
#endif
