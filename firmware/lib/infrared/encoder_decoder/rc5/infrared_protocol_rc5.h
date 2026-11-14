#pragma once

#include "../infrared_i.h"

/* RC5/RC5X Manchester : quantums 888/1776, débute Space */
/* Start=1 ; si : start RC5 ou extension RC5X */
/* Toggle change à chaque press ; address 5 bits, command 6/7 */
/* Source : mikrocontroller.net/articles/IRMP_-_english#RC5_.2B_RC5X */

void* infrared_decoder_rc5_alloc(void);
void infrared_decoder_rc5_reset(void* decoder);
void infrared_decoder_rc5_free(void* decoder);
InfraredMessage* infrared_decoder_rc5_check_ready(void* ctx);
InfraredMessage* infrared_decoder_rc5_decode(void* decoder, bool level, uint32_t duration);

void* infrared_encoder_rc5_alloc(void);
void infrared_encoder_rc5_reset(void* encoder_ptr, const InfraredMessage* message);
void infrared_encoder_rc5_free(void* decoder);
InfraredStatus infrared_encoder_rc5_encode(void* encoder_ptr, uint32_t* duration, bool* polarity);

const InfraredProtocolVariant* infrared_protocol_rc5_get_variant(InfraredProtocol protocol);
