#pragma once

#include "base.h"
#include "common_defines.h"
#include "thread.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    FuriThread* thread; /* Pointeur valide tant que le thread tourne */
    const char* app_id; /* Appid valide tant que le thread tourne */
    const char* name; /* Nom valide tant que le thread tourne */
    FuriThreadPriority priority;
    uint32_t stack_address;
    size_t heap; /* Heap tracé en bytes ; 0 si traçage désactivé */
    uint32_t stack_size;
    uint32_t stack_min_free; /* Minimum de stack libre atteint */
    const char*
        state;
    float cpu; /* Charge CPU en pourcentage, interrupts incluses */

    uint32_t counter_previous;
    uint32_t counter_current;
    uint32_t tick;
} FuriThreadListItem;

typedef struct FuriThreadList FuriThreadList;

FuriThreadList* furi_thread_list_alloc(void);

void furi_thread_list_free(FuriThreadList* instance);

/* Nombre d’items */
size_t furi_thread_list_size(FuriThreadList* instance);

FuriThreadListItem* furi_thread_list_get_at(FuriThreadList* instance, size_t position);

FuriThreadListItem* furi_thread_list_get_or_insert(FuriThreadList* instance, FuriThread* thread);

/* Temps passé en ISR, en pourcentage */
float furi_thread_list_get_isr_time(FuriThreadList* instance);

#ifdef __cplusplus
}
#endif
