#pragma once

#include "../infrared_i.h"

/* RC6 : https://www.mikrocontroller.net/articles/IRMP_-_english#RC6_.2B_RC6A */
/* Préambule 2666/889 ; bits 444/888 ; silence 2666 */
/* Start toujours 1 ; mode 000 ; toggle double durée */
/* Adresse puis commande : 8 bits MSB chacune */

void* infrared_decoder_rc6_alloc(void);
void infrared_decoder_rc6_reset(void* decoder);
void infrared_decoder_rc6_free(void* decoder);
InfraredMessage* infrared_decoder_rc6_check_ready(void* ctx);
InfraredMessage* infrared_decoder_rc6_decode(void* decoder, bool level, uint32_t duration);

void* infrared_encoder_rc6_alloc(void);
void infrared_encoder_rc6_reset(void* encoder_ptr, const InfraredMessage* message);
void infrared_encoder_rc6_free(void* decoder);
InfraredStatus infrared_encoder_rc6_encode(void* encoder_ptr, uint32_t* duration, bool* polarity);

const InfraredProtocolVariant* infrared_protocol_rc6_get_variant(InfraredProtocol protocol);
