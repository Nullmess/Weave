/**
 * @file expansion.h
 * @brief Expansion module support library.
 */
#pragma once

#include <furi_hal_serial_types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECORD_EXPANSION "expansion"

typedef struct Expansion Expansion;

/* Charge settings utilisateur ; aucun effet si support désactivé */

void expansion_enable(Expansion* instance);

/* Disable expansion avant serial_control_acquire obligatoire */
/* Enable expansion juste après serial_control_release obligatoire */

void expansion_disable(Expansion* instance);

/* Un seul port ; nouvelle sélection remplace ancienne */
/* Ignore settings utilisateur, peut laisser état incohérent */

void expansion_set_listen_serial(Expansion* instance, FuriHalSerialId serial_id);

#ifdef __cplusplus
}
#endif
