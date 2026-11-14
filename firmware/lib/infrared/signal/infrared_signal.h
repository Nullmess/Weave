/* Signals parsed décodés ; raw décrit par timings */

#pragma once

#include "infrared_error_code.h"
#include <flipper_format/flipper_format.h>
#include <infrared.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InfraredSignal InfraredSignal;

/* Temps µs ; fréquence Hz ; duty_cycle entre 0 et 1 */

typedef struct {
    size_t timings_size;
    uint32_t* timings;
    uint32_t frequency;
    float duty_cycle;
} InfraredRawSignal;

InfraredSignal* infrared_signal_alloc(void);

void infrared_signal_free(InfraredSignal* signal);

bool infrared_signal_is_raw(const InfraredSignal* signal);

bool infrared_signal_is_valid(const InfraredSignal* signal);

/* Contenu précédent supprimé avant copie */

void infrared_signal_set_signal(InfraredSignal* signal, const InfraredSignal* other);

/* Copie raw, remplace contenu ; fréquence Hz, duty_cycle 0..1 */

void infrared_signal_set_raw_signal(
    InfraredSignal* signal,
    const uint32_t* timings,
    size_t timings_size,
    uint32_t frequency,
    float duty_cycle);

/* Signal raw obligatoire ; sinon comportement indéfini */

const InfraredRawSignal* infrared_signal_get_raw_signal(const InfraredSignal* signal);

/* Copie parsed, remplace contenu précédent */

void infrared_signal_set_message(InfraredSignal* signal, const InfraredMessage* message);

/* Signal parsed obligatoire ; sinon comportement indéfini */

const InfraredMessage* infrared_signal_get_message(const InfraredSignal* signal);

/* Fichier ouvert et name alloué ; lecture depuis curseur courant */

InfraredErrorCode
    infrared_signal_read(InfraredSignal* signal, FlipperFormat* ff, FuriString* name);

/* Fichier ouvert et name alloué ; lecture depuis curseur courant */

InfraredErrorCode infrared_signal_read_name(FlipperFormat* ff, FuriString* name);

/* Fichier ouvert et signal alloué ; lecture depuis curseur courant */

InfraredErrorCode infrared_signal_read_body(InfraredSignal* signal, FlipperFormat* ff);

/* Fichier ouvert requis ; recherche nom puis lecture */

InfraredErrorCode infrared_signal_search_by_name_and_read(
    InfraredSignal* signal,
    FlipperFormat* ff,
    const char* name);

/* Fichier ouvert requis ; recherche index puis lecture */

InfraredErrorCode infrared_signal_search_by_index_and_read(
    InfraredSignal* signal,
    FlipperFormat* ff,
    size_t index);

/* Fichier ouvert avec header approprié préalable requis */

InfraredErrorCode
    infrared_signal_save(const InfraredSignal* signal, FlipperFormat* ff, const char* name);

/* Une transmission par appel via HAL intégré */

void infrared_signal_transmit(const InfraredSignal* signal);

#ifdef __cplusplus
}
#endif
