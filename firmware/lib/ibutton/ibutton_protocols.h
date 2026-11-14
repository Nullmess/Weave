

#pragma once

#include <stdint.h>
#include <stddef.h>

#include "protocols/protocol_common.h"

#include "ibutton_key.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct iButtonProtocols iButtonProtocols;

iButtonProtocols* ibutton_protocols_alloc(void);

void ibutton_protocols_free(iButtonProtocols* protocols);

uint32_t ibutton_protocols_get_protocol_count(void);

/* Taille maximale en octets */

size_t ibutton_protocols_get_max_data_size(iButtonProtocols* protocols);

/* iButtonProtocolIdInvalid si nom absent */

iButtonProtocolId ibutton_protocols_get_id_by_name(iButtonProtocols* protocols, const char* name);

/* String statique */

const char* ibutton_protocols_get_manufacturer(iButtonProtocols* protocols, iButtonProtocolId id);

/* String statique */

const char* ibutton_protocols_get_name(iButtonProtocols* protocols, iButtonProtocolId id);

uint32_t ibutton_protocols_get_features(iButtonProtocols* protocols, iButtonProtocolId id);

/* Clé destination préalablement allouée requise */

bool ibutton_protocols_read(iButtonProtocols* protocols, iButtonKey* key);

bool ibutton_protocols_write_id(iButtonProtocols* protocols, iButtonKey* key);

/* Clé destination du même type */

bool ibutton_protocols_write_copy(iButtonProtocols* protocols, iButtonKey* key);

void ibutton_protocols_emulate_start(iButtonProtocols* protocols, iButtonKey* key);

void ibutton_protocols_emulate_stop(iButtonProtocols* protocols, iButtonKey* key);

/* Chemin absolu complet */

bool ibutton_protocols_save(
    iButtonProtocols* protocols,
    const iButtonKey* key,
    const char* file_name);

/* Clé destination allouée ; chemin absolu complet */

bool ibutton_protocols_load(iButtonProtocols* protocols, iButtonKey* key, const char* file_name);

/* FuriString résultat initialisée requise */

void ibutton_protocols_render_uid(
    iButtonProtocols* protocols,
    const iButtonKey* key,
    FuriString* result);

/* FuriString résultat initialisée requise */

void ibutton_protocols_render_data(
    iButtonProtocols* protocols,
    const iButtonKey* key,
    FuriString* result);

/* FuriString résultat initialisée requise */

void ibutton_protocols_render_brief_data(
    iButtonProtocols* protocols,
    const iButtonKey* key,
    FuriString* result);

/* FuriString résultat initialisée requise */

void ibutton_protocols_render_error(
    iButtonProtocols* protocols,
    const iButtonKey* key,
    FuriString* result);

bool ibutton_protocols_is_valid(iButtonProtocols* protocols, const iButtonKey* key);

/* Données éditables en place */

void ibutton_protocols_get_editable_data(
    iButtonProtocols* protocols,
    const iButtonKey* key,
    iButtonEditableData* editable);

/* Ajustements internes obligatoires après édition clé */

void ibutton_protocols_apply_edits(iButtonProtocols* protocols, const iButtonKey* key);

#ifdef __cplusplus
}
#endif
