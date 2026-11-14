#pragma once

#include <core/base.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Retour true en IRQ ou kernel actif avec IRQ masquées */
/* Kernel arrêté : PRIMASK ignoré ; vérifier sécurité des appels */
bool furi_kernel_is_irq_or_masked(void);

bool furi_kernel_is_running(void);

/* Interdit en ISR ; retour 0 libre, 1 verrouillé */
int32_t furi_kernel_lock(void);

/* Interdit en ISR ; retour 0 libre, 1 verrouillé */
int32_t furi_kernel_unlock(void);

/* Interdit en ISR ; retour nouvel état ou erreur */
int32_t furi_kernel_restore_lock(int32_t lock);

/* Fréquence systick en Hz */
uint32_t furi_kernel_get_tick_frequency(void);

/* Interdit en ISR ; délai quantifié par le scheduler */
void furi_delay_tick(uint32_t ticks);

/* Interdit en ISR ; attente jusqu’au tick demandé */
FuriStatus furi_delay_until_tick(uint32_t tick);

/* Uptime en ms, avec overflow possible */
uint32_t furi_get_tick(void);

uint32_t furi_ms_to_ticks(uint32_t milliseconds);

/* Interdit en ISR ; 0 provoque yield */
/* Délai quantifié ; kernel arrêté : fallback furi_delay_us */
void furi_delay_ms(uint32_t milliseconds);

/* DWT : délai bloquant en µs, sans quantification scheduler */
void furi_delay_us(uint32_t microseconds);

#ifdef __cplusplus
}
#endif
