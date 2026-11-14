/* Bruteforce par catégorie pour Universal Remote */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "infrared_error_code.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct InfraredBruteForce InfraredBruteForce;

InfraredBruteForce* infrared_brute_force_alloc(void);

void infrared_brute_force_free(InfraredBruteForce* brute_force);

/* Chemin database complet */

void infrared_brute_force_set_db_filename(InfraredBruteForce* brute_force, const char* db_filename);

/* Rebâtir dictionnaire après chaque changement database */

InfraredErrorCode infrared_brute_force_calculate_messages(InfraredBruteForce* brute_force);

/* record_count=0 si échec ; true si catégorie trouvée et fichier ouvert */

bool infrared_brute_force_start(
    InfraredBruteForce* brute_force,
    uint32_t index,
    uint32_t* record_count);

bool infrared_brute_force_is_started(const InfraredBruteForce* brute_force);

void infrared_brute_force_stop(InfraredBruteForce* brute_force);

/* signal_index dans catégorie selon record_count de start */

bool infrared_brute_force_send(InfraredBruteForce* brute_force, uint32_t signal_index);

void infrared_brute_force_add_record(
    InfraredBruteForce* brute_force,
    uint32_t index,
    const char* name);

void infrared_brute_force_reset(InfraredBruteForce* brute_force);

#ifdef __cplusplus
}
#endif
