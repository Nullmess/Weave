/*

  u8x8_setup.c

  Universal 8bit Graphics Library (https://github.com/olikraus/u8g2/)

  Copyright (c) 2016, olikraus@gmail.com
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this list 
    of conditions and the following disclaimer.
    
  * Redistributions in binary form must reproduce the above copyright notice, this 
    list of conditions and the following disclaimer in the documentation and/or other 
    materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND 
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR 
  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
  NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER 
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
  ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  

*/

#include "u8x8.h"

uint8_t u8x8_dummy_cb(
    U8X8_UNUSED u8x8_t* u8x8,
    U8X8_UNUSED uint8_t msg,
    U8X8_UNUSED uint8_t arg_int,
    U8X8_UNUSED void* arg_ptr) {
    /* Callback dummy échoue pour tous les messages */
    return 0;
}

static const u8x8_display_info_t u8x8_null_display_info = {
      0,
      1,

      0,
      0,
      0,
      0,
      0,

    0, /* Demi-cycle 100 ns ; AVR <70 : 8 MHz, sinon 4 MHz */

    4000000UL, /* Arduino >=1.6.0 : SPI en Hz, 1e9/sck_pulse_width_ns */
      0, /* Actif haut, front montant */
      4,
      0,
      0,
      1, /* 8x8 */
      1,
      0,
      0,
      8,
      8};

uint8_t u8x8_d_null_cb(
    u8x8_t* u8x8,
    uint8_t msg,
    U8X8_UNUSED uint8_t arg_int,
    U8X8_UNUSED void* arg_ptr) {
    switch(msg) {
    case U8X8_MSG_DISPLAY_SETUP_MEMORY:
        u8x8_d_helper_display_setup_memory(u8x8, &u8x8_null_display_info);
        break;
    case U8X8_MSG_DISPLAY_INIT:
        u8x8_d_helper_display_init(u8x8);
        break;
    }
    /* Callback null réussit pour tous les messages */
    return 1;
}

void u8x8_SetupDefaults(u8x8_t* u8x8) {
    u8x8->display_info = NULL;
    u8x8->display_cb = u8x8_dummy_cb;
    u8x8->cad_cb = u8x8_dummy_cb;
    u8x8->byte_cb = u8x8_dummy_cb;
    u8x8->gpio_and_delay_cb = u8x8_dummy_cb;
    u8x8->is_font_inverse_mode = 0;

    u8x8->utf8_state = 0; /* Également reset par u8x8_utf8_init */
    u8x8->bus_clock = 0; /* Issue #769 */
    u8x8->i2c_address = 255;
    u8x8->debounce_default_pin_state = 255; /* Boutons supposés actifs bas */

#ifdef U8X8_USE_PINS
    {
        uint8_t i;
        for(i = 0; i < U8X8_PIN_CNT; i++)
            u8x8->pins[i] = U8X8_PIN_NONE;
    }
#endif
}

/* Setup assigne callbacks sans communiquer avec écran */
/* u8x8_dummy_cb si aucun callback requis */
/* Appeler InitDisplay pour envoyer la séquence startup */

void u8x8_Setup(
    u8x8_t* u8x8,
    u8x8_msg_cb display_cb,
    u8x8_msg_cb cad_cb,
    u8x8_msg_cb byte_cb,
    u8x8_msg_cb gpio_and_delay_cb) {
    /* Defaults puis pins U8X8_PIN_NONE */
    u8x8_SetupDefaults(u8x8);

    u8x8->display_cb = display_cb;
    u8x8->cad_cb = cad_cb;
    u8x8->byte_cb = byte_cb;
    u8x8->gpio_and_delay_cb = gpio_and_delay_cb;

    u8x8_SetupMemory(u8x8);
}
