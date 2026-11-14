/* Noms en RAM, potentiellement identiques ; signaux chargés à la demande */

#pragma once

#include <lib/infrared/signal/infrared_signal.h>

typedef struct InfraredRemote InfraredRemote;

InfraredRemote* infrared_remote_alloc(void);

void infrared_remote_free(InfraredRemote* remote);

/* Reset efface liste noms et chemin fichier */

void infrared_remote_reset(InfraredRemote* remote);

/* Nom dérivé du chemin, valide jusqu’au reset/load/create */

const char* infrared_remote_get_name(const InfraredRemote* remote);

/* Chemin valide jusqu’au reset/load/create */

const char* infrared_remote_get_path(const InfraredRemote* remote);

size_t infrared_remote_get_signal_count(const InfraredRemote* remote);

/* Index strictement inférieur au nombre de signaux */

const char* infrared_remote_get_signal_name(const InfraredRemote* remote, size_t index);

bool infrared_remote_get_signal_index(
    const InfraredRemote* remote,
    const char* name,
    size_t* index);

/* Chargement à chaque signal ; destination allouée, index valide requis */

InfraredErrorCode
    infrared_remote_load_signal(const InfraredRemote* remote, InfraredSignal* signal, size_t index);

/* load/create préalable requis pour initialiser chemin */

InfraredErrorCode infrared_remote_append_signal(
    InfraredRemote* remote,
    const InfraredSignal* signal,
    const char* name);

/* Index >=nombre : append ; erreur OR index signal fautif */

InfraredErrorCode infrared_remote_insert_signal(
    InfraredRemote* remote,
    const InfraredSignal* signal,
    const char* name,
    size_t index);

/* Index valide requis ; nom seul modifié */

InfraredErrorCode
    infrared_remote_rename_signal(InfraredRemote* remote, size_t index, const char* new_name);

/* Indexes source/destination valides ; erreur OR index signal fautif */

InfraredErrorCode
    infrared_remote_move_signal(InfraredRemote* remote, size_t index, size_t new_index);

/* Index valide requis ; erreur OR index signal fautif */

InfraredErrorCode infrared_remote_delete_signal(InfraredRemote* remote, size_t index);

/* Reset puis fichier vide avec header, chemin complet */

InfraredErrorCode infrared_remote_create(InfraredRemote* remote, const char* path);

/* Reset puis noms chargés ; fichier existant valide requis */

InfraredErrorCode infrared_remote_load(InfraredRemote* remote, const char* path);

/* Chemin complet ; aucun signal modifié */

InfraredErrorCode infrared_remote_rename(InfraredRemote* remote, const char* new_path);

/* Suppression irréversible ; instance invalide jusqu’à create/load */

InfraredErrorCode infrared_remote_remove(InfraredRemote* remote);
