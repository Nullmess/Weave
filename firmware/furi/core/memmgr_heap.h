
#pragma once

#include <stdint.h>
#include <core/thread.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMMGR_HEAP_UNKNOWN 0xFFFFFFFF

void memmgr_heap_enable_thread_trace(FuriThreadId thread_id);

void memmgr_heap_disable_thread_trace(FuriThreadId thread_id);

/* Mémoire actuellement allouée au thread en bytes */
size_t memmgr_heap_get_thread_memory(FuriThreadId thread_id);

/* Plus grand bloc libre contigu du heap */
size_t memmgr_heap_get_max_free_block(void);

void memmgr_heap_printf_free_blocks(void);

#ifdef __cplusplus
}
#endif
