#pragma once

#include <infrared.h>
#include <furi_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_TIMINGS_AMOUNT 1024U

typedef struct InfraredWorker InfraredWorker;

typedef struct InfraredWorkerSignal InfraredWorkerSignal;

typedef enum {
    InfraredWorkerGetSignalResponseNew, /* Nouveau signal callback : reset encodeur */
    InfraredWorkerGetSignalResponseSame, /* Signal callback identique : aucun reset encodeur */
    InfraredWorkerGetSignalResponseStop,
} InfraredWorkerGetSignalResponse;

/* Callback fournit signal via make_decoded_signal/make_raw_signal */

typedef InfraredWorkerGetSignalResponse (
    *InfraredWorkerGetSignalCallback)(void* context, InfraredWorker* instance);

typedef void (*InfraredWorkerMessageSentCallback)(void* context);

/* Callback InfraredWorker thread après réception signal */
typedef void (
    *InfraredWorkerReceivedSignalCallback)(void* context, InfraredWorkerSignal* received_signal);

InfraredWorker* infrared_worker_alloc(void);

void infrared_worker_free(InfraredWorker* instance);

/* Lance thread et initialise furi_hal */

void infrared_worker_rx_start(InfraredWorker* instance);

/* Arrête thread et désinitialise furi_hal */

void infrared_worker_rx_stop(InfraredWorker* instance);

void infrared_worker_rx_set_received_signal_callback(
    InfraredWorker* instance,
    InfraredWorkerReceivedSignalCallback callback,
    void* context);

void infrared_worker_rx_enable_blink_on_receiving(InfraredWorker* instance, bool enable);

void infrared_worker_rx_enable_signal_decoding(InfraredWorker* instance, bool enable);

bool infrared_worker_signal_is_decoded(const InfraredWorkerSignal* signal);

/* Configurer GetSignalCallback avant start, qui préremplit buffer */

void infrared_worker_tx_start(InfraredWorker* instance);

/* Attendre fin signal courant avant arrêt TX */

void infrared_worker_tx_stop(InfraredWorker* instance);

void infrared_worker_tx_set_get_signal_callback(
    InfraredWorker* instance,
    InfraredWorkerGetSignalCallback callback,
    void* context);

void infrared_worker_tx_set_signal_sent_callback(
    InfraredWorker* instance,
    InfraredWorkerMessageSentCallback callback,
    void* context);

/* Signal constant préconfiguré via set_decoded_signal/set_raw_signal */
/* Callback steady à enregistrer, jamais appeler directement */

InfraredWorkerGetSignalResponse
    infrared_worker_tx_get_signal_steady_callback(void* context, InfraredWorker* instance);

/* Signal raw requis avant accès timings */

void infrared_worker_get_raw_signal(
    const InfraredWorkerSignal* signal,
    const uint32_t** timings,
    size_t* timings_cnt);

/* Signal décodé requis avant accès message */

const InfraredMessage* infrared_worker_get_decoded_signal(const InfraredWorkerSignal* signal);

void infrared_worker_set_decoded_signal(InfraredWorker* instance, const InfraredMessage* message);

/* Fréquence en Hz ; duty_cycle entre 0 et 1 */

void infrared_worker_set_raw_signal(
    InfraredWorker* instance,
    const uint32_t* timings,
    size_t timings_cnt,
    uint32_t frequency,
    float duty_cycle);

#ifdef __cplusplus
}
#endif
