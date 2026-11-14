#pragma once

#include "../infrared_i.h"

/* Sony SIRC : sbprojects.net/knowledge/ir/sirc.php, picprojects.org.uk */
/* Préambule 2400/600 ; PWM 600/1200, période 45000 */
/* Command 7 bits LSB ; address 5/8/13 bits selon SIRC12/15/20 */
/* Repeat indiscernable ; remotes Sony transmettent au moins trois messages */
/* SIRC20 : huit bits étendus finaux considérés address */

void* infrared_decoder_sirc_alloc(void);
void infrared_decoder_sirc_reset(void* decoder);
InfraredMessage* infrared_decoder_sirc_check_ready(void* decoder);
void infrared_decoder_sirc_free(void* decoder);
InfraredMessage* infrared_decoder_sirc_decode(void* decoder, bool level, uint32_t duration);

void* infrared_encoder_sirc_alloc(void);
void infrared_encoder_sirc_reset(void* encoder_ptr, const InfraredMessage* message);
void infrared_encoder_sirc_free(void* decoder);
InfraredStatus infrared_encoder_sirc_encode(void* encoder_ptr, uint32_t* duration, bool* polarity);

const InfraredProtocolVariant* infrared_protocol_sirc_get_variant(InfraredProtocol protocol);
