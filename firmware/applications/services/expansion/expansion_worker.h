/* Worker expansion privé ; démarrage détection, arrêt déconnexion/erreur */

#pragma once

#include <furi_hal_serial_types.h>

/**
 * @brief Expansion worker opaque type declaration.
 */
typedef struct ExpansionWorker ExpansionWorker;

typedef void (*ExpansionWorkerCallback)(void* context);

ExpansionWorker* expansion_worker_alloc(FuriHalSerialId serial_id);

void expansion_worker_free(ExpansionWorker* instance);

/* Disconnect callback exclut arrêt explicite via expansion_worker_stop */
void expansion_worker_set_callback(
    ExpansionWorker* instance,
    ExpansionWorkerCallback callback,
    void* context);

void expansion_worker_start(ExpansionWorker* instance);

/* Stop explicite ne déclenche pas callback */

void expansion_worker_stop(ExpansionWorker* instance);
