/* Jusqu’à 32 signals indexés, enregistrés avant définition séquence */

#pragma once

#include <furi_hal_gpio.h>

#include <digital_signal/digital_signal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DigitalSequence DigitalSequence;

/* size : nombre maximal index signals */

DigitalSequence* digital_sequence_alloc(uint32_t size, const GpioPin* gpio);

void digital_sequence_free(DigitalSequence* sequence);

/* Index <32 ; DigitalSequence ne possède pas signals */
/* Appelant crée/libère signals, valides au moins jusqu’à destruction séquence */

void digital_sequence_register_signal(
    DigitalSequence* sequence,
    uint8_t signal_index,
    const DigitalSignal* signal);

/* Signal enregistré préalable ; index <32 */

void digital_sequence_add_signal(DigitalSequence* sequence, uint8_t signal_index);

/* Au moins un signal enregistré et un index requis */
/* GPIO initialisé ; appelant reconfigure avant autre usage */

void digital_sequence_transmit(DigitalSequence* sequence);

/* Clear conserve signals enregistrés */

void digital_sequence_clear(DigitalSequence* sequence);

#ifdef __cplusplus
}
#endif
