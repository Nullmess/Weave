#pragma once

#include "../infrared_i.h"

/* Kaseikyo : https://github.com/Arduino-IRremote/Arduino-IRremote/blob/master/src/ir_Kaseikyo.hpp */
/* Préambule 3360/1665 ; 48 bits ; période 130000 */
/* Répétition : préambule 3456/1728 */

void* infrared_decoder_kaseikyo_alloc(void);
void infrared_decoder_kaseikyo_reset(void* decoder);
void infrared_decoder_kaseikyo_free(void* decoder);
InfraredMessage* infrared_decoder_kaseikyo_check_ready(void* decoder);
InfraredMessage* infrared_decoder_kaseikyo_decode(void* decoder, bool level, uint32_t duration);

void* infrared_encoder_kaseikyo_alloc(void);
InfraredStatus
    infrared_encoder_kaseikyo_encode(void* encoder_ptr, uint32_t* duration, bool* level);
void infrared_encoder_kaseikyo_reset(void* encoder_ptr, const InfraredMessage* message);
void infrared_encoder_kaseikyo_free(void* encoder_ptr);

const InfraredProtocolVariant* infrared_protocol_kaseikyo_get_variant(InfraredProtocol protocol);
