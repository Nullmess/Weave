
#pragma once

#include <stdbool.h>
#include "core_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Usage interne uniquement */
void furi_record_init(void);

/* Thread-safe ; création/destruction depuis le même thread */
bool furi_record_exists(const char* name);

/* Thread-safe ; data non NULL ; création/destruction même thread */
void furi_record_create(const char* name, void* data);

/* Retour false si holders actifs ou thread non propriétaire */
/* Thread-safe ; création/destruction depuis le même thread */
bool furi_record_destroy(const char* name);

/* Thread-safe ; ouverture/fermeture depuis le même thread */
/* Suspend le caller jusqu’à disponibilité du record */
FURI_RETURNS_NONNULL void* furi_record_open(const char* name);

/* Thread-safe ; ouverture/fermeture depuis le même thread */
void furi_record_close(const char* name);

#ifdef __cplusplus
}
#endif
