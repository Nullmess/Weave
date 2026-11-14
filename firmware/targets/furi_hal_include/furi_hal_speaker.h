#pragma once

#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

void furi_hal_speaker_init(void);

void furi_hal_speaker_deinit(void);

/* Acquisition ownership obligatoire avant utilisation */
FURI_WARN_UNUSED bool furi_hal_speaker_acquire(uint32_t timeout);

/* Release ownership obligatoire après utilisation */
void furi_hal_speaker_release(void);

/* Retour true systématique depuis ISR */
bool furi_hal_speaker_is_mine(void);

/* Aucune vérification ownership en ISR */
void furi_hal_speaker_start(float frequency, float volume);

/* Aucune vérification ownership en ISR */
void furi_hal_speaker_set_volume(float volume);

/* Aucune vérification ownership en ISR */
void furi_hal_speaker_stop(void);

#ifdef __cplusplus
}
#endif
