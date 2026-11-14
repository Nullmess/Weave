#pragma once

#include "../infrared_i.h"

/* NEC : https://radioparty.ru/manuals/encyclopedia/213-ircontrol?start=1 */
/* Préambule 9000/4500 ; 32 bits + stop ; période 110000 */
/* Répétition : préambule 9000/2250 + stop */

void* infrared_decoder_nec_alloc(void);
void infrared_decoder_nec_reset(void* decoder);
void infrared_decoder_nec_free(void* decoder);
InfraredMessage* infrared_decoder_nec_check_ready(void* decoder);
InfraredMessage* infrared_decoder_nec_decode(void* decoder, bool level, uint32_t duration);

void* infrared_encoder_nec_alloc(void);
InfraredStatus infrared_encoder_nec_encode(void* encoder_ptr, uint32_t* duration, bool* level);
void infrared_encoder_nec_reset(void* encoder_ptr, const InfraredMessage* message);
void infrared_encoder_nec_free(void* encoder_ptr);

const InfraredProtocolVariant* infrared_protocol_nec_get_variant(InfraredProtocol protocol);
