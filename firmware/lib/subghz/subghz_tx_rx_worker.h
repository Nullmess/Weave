#pragma once

#include <furi_hal.h>
#include <devices/devices.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*SubGhzTxRxWorkerCallbackHaveRead)(void* context);

typedef struct SubGhzTxRxWorker SubGhzTxRxWorker;

typedef enum {
    SubGhzTxRxWorkerStatusIDLE,
    SubGhzTxRxWorkerStatusTx,
    SubGhzTxRxWorkerStatusRx,
} SubGhzTxRxWorkerStatus;

/* Retour true si données acceptées */
bool subghz_tx_rx_worker_write(SubGhzTxRxWorker* instance, uint8_t* data, size_t size);

size_t subghz_tx_rx_worker_available(SubGhzTxRxWorker* instance);

/* Lecture bornée par size ; retour taille lue */
size_t subghz_tx_rx_worker_read(SubGhzTxRxWorker* instance, uint8_t* data, size_t size);

/* Callback sur remplissage du buffer vide */
void subghz_tx_rx_worker_set_callback_have_read(
    SubGhzTxRxWorker* instance,
    SubGhzTxRxWorkerCallbackHaveRead callback,
    void* context);

SubGhzTxRxWorker* subghz_tx_rx_worker_alloc(void);

void subghz_tx_rx_worker_free(SubGhzTxRxWorker* instance);

/* Retour true si succès */
bool subghz_tx_rx_worker_start(
    SubGhzTxRxWorker* instance,
    const SubGhzDevice* device,
    uint32_t frequency);

void subghz_tx_rx_worker_stop(SubGhzTxRxWorker* instance);

bool subghz_tx_rx_worker_is_running(SubGhzTxRxWorker* instance);

#ifdef __cplusplus
}
#endif
