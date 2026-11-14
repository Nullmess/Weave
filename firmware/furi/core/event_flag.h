#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriEventFlag FuriEventFlag;

FuriEventFlag* furi_event_flag_alloc(void);

void furi_event_flag_free(FuriEventFlag* instance);

/* Les flags retournés peuvent être effacés par un autre waiter */
/* Consulter xEventGroupSetBits ; retour flags ou erreur FuriStatus */
uint32_t furi_event_flag_set(FuriEventFlag* instance, uint32_t flags);

/* Retour flags ou erreur FuriStatus */
uint32_t furi_event_flag_clear(FuriEventFlag* instance, uint32_t flags);

uint32_t furi_event_flag_get(FuriEventFlag* instance);

/* Retour flags ou erreur FuriStatus */
uint32_t furi_event_flag_wait(
    FuriEventFlag* instance,
    uint32_t flags,
    uint32_t options,
    uint32_t timeout);

#ifdef __cplusplus
}
#endif
