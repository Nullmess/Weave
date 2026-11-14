/*
  
  u8x8_cad.c
  
  "command arg data" interface to the graphics controller

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


  The following sequence must be used for any data, which is set to the display:
  
  
  uint8_t u8x8_cad_StartTransfer(u8x8_t *u8x8)

  any of the following calls
    uint8_t u8x8_cad_SendCmd(u8x8_t *u8x8, uint8_t cmd)
    uint8_t u8x8_cad_SendArg(u8x8_t *u8x8, uint8_t arg)
    uint8_t u8x8_cad_SendData(u8x8_t *u8x8, uint8_t cnt, uint8_t *data)
  
  uint8_t u8x8_cad_EndTransfer(u8x8_t *u8x8)



*/
/*
uint8_t u8x8_cad_template(u8x8_t *u8x8, uint8_t msg, uint16_t arg_int, void *arg_ptr)
{
  uint8_t i;
  
  switch(msg)
  {
    case U8X8_MSG_CAD_SEND_CMD:
      u8x8_mcd_byte_SetDC(mcd->next, 1);
      u8x8_mcd_byte_Send(mcd->next, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_ARG:
      u8x8_mcd_byte_SetDC(mcd->next, 1);
      u8x8_mcd_byte_Send(mcd->next, arg_int);
      break;
    case U8X8_MSG_CAD_SEND_DATA:
      u8x8_mcd_byte_SetDC(mcd->next, 0);
      for( i = 0; i < 8; i++ )
	u8x8_mcd_byte_Send(mcd->next, ((uint8_t *)arg_ptr)[i]);
      break;
    case U8X8_MSG_CAD_RESET:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_END_TRANSFER:
      return mcd->next->cb(mcd->next, msg, arg_int, arg_ptr);
    default:
      break;
  }
  return 1;
}

*/

#include "u8x8.h"

uint8_t u8x8_cad_SendCmd(u8x8_t* u8x8, uint8_t cmd) {
    return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_CMD, cmd, NULL);
}

uint8_t u8x8_cad_SendArg(u8x8_t* u8x8, uint8_t arg) {
    return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_ARG, arg, NULL);
}

uint8_t u8x8_cad_SendMultipleArg(u8x8_t* u8x8, uint8_t cnt, uint8_t arg) {
    while(cnt > 0) {
        u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_ARG, arg, NULL);
        cnt--;
    }
    return 1;
}

uint8_t u8x8_cad_SendData(u8x8_t* u8x8, uint8_t cnt, uint8_t* data) {
    return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, cnt, data);
}

uint8_t u8x8_cad_StartTransfer(u8x8_t* u8x8) {
    return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_START_TRANSFER, 0, NULL);
}

uint8_t u8x8_cad_EndTransfer(u8x8_t* u8x8) {
    return u8x8->cad_cb(u8x8, U8X8_MSG_CAD_END_TRANSFER, 0, NULL);
}

void u8x8_cad_vsendf(u8x8_t* u8x8, const char* fmt, va_list va) {
    uint8_t d;
    u8x8_cad_StartTransfer(u8x8);
    while(*fmt != '\0') {
        d = (uint8_t)va_arg(va, int);
        switch(*fmt) {
        case 'a':
            u8x8_cad_SendArg(u8x8, d);
            break;
        case 'c':
            u8x8_cad_SendCmd(u8x8, d);
            break;
        case 'd':
            u8x8_cad_SendData(u8x8, 1, &d);
            break;
        }
        fmt++;
    }
    u8x8_cad_EndTransfer(u8x8);
}

void u8x8_SendF(u8x8_t* u8x8, const char* fmt, ...) {
    va_list va;
    va_start(va, fmt);
    u8x8_cad_vsendf(u8x8, fmt, va);
    va_end(va);
}

/* 21 commande ; 22 argument ; 23 data */
/* 24 CS on ; 25 CS off ; 254 délai ms ; 255 fin */

void u8x8_cad_SendSequence(u8x8_t* u8x8, uint8_t const* data) {
    uint8_t cmd;
    uint8_t v;

    for(;;) {
        cmd = *data;
        data++;
        switch(cmd) {
        case U8X8_MSG_CAD_SEND_CMD:
        case U8X8_MSG_CAD_SEND_ARG:
            v = *data;
            u8x8->cad_cb(u8x8, cmd, v, NULL);
            data++;
            break;
        case U8X8_MSG_CAD_SEND_DATA:
            v = *data;
            u8x8_cad_SendData(u8x8, 1, &v);
            data++;
            break;
        case U8X8_MSG_CAD_START_TRANSFER:
        case U8X8_MSG_CAD_END_TRANSFER:
            u8x8->cad_cb(u8x8, cmd, 0, NULL);
            break;
        case 0x0fe:
            v = *data;
            u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_MILLI, v);
            data++;
            break;
        default:
            return;
        }
    }
}

uint8_t u8x8_cad_empty(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
        return 0;
    }
    return 1;
}

/* DC=1 commandes/arguments ; DC=0 data */

uint8_t u8x8_cad_110(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_SetDC(u8x8, 1);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SetDC(u8x8, 1);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:
        u8x8_byte_SetDC(u8x8, 0);

        /* Fallthrough */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
        return 0;
    }
    return 1;
}

/* T6963 : DC=1 commandes/arguments ; DC=0 data */

uint8_t u8x8_cad_100(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_SetDC(u8x8, 1);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SetDC(u8x8, 0);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:
        u8x8_byte_SetDC(u8x8, 0);

        /* Fallthrough */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
        return 0;
    }
    return 1;
}

/* DC=0 commandes/arguments ; DC=1 data */

uint8_t u8x8_cad_001(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_SetDC(u8x8, 0);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SetDC(u8x8, 0);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:
        u8x8_byte_SetDC(u8x8, 1);

        /* Fallthrough */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
        return 0;
    }
    return 1;
}

/* DC=0 commandes ; DC=1 arguments/data */

uint8_t u8x8_cad_011(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_SetDC(u8x8, 0);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SetDC(u8x8, 1);
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:
        u8x8_byte_SetDC(u8x8, 1);

        /* Fallthrough */
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
        return 0;
    }
    return 1;
}

/* u8x8_byte_SetDC inutilisé */
uint8_t u8x8_cad_st7920_spi(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    uint8_t* data;
    uint8_t b;
    uint8_t i;
    static uint8_t buf[16];
    uint8_t* ptr;

    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_SendByte(u8x8, 0x0f8);
        u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
        u8x8_byte_SendByte(u8x8, arg_int & 0x0f0);
        u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
        u8x8_byte_SendByte(u8x8, arg_int << 4);
        u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SendByte(u8x8, 0x0f8);
        u8x8_byte_SendByte(u8x8, arg_int & 0x0f0);
        u8x8_byte_SendByte(u8x8, arg_int << 4);
        break;
    case U8X8_MSG_CAD_SEND_DATA:

        u8x8_byte_SendByte(u8x8, 0x0fa);
        u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);

        /* TODO: grouper les octets du transfert */

        data = (uint8_t*)arg_ptr;

        /* Boucle accélérée d’environ 20% */
        while(arg_int >= 8) {
            i = 8;
            ptr = buf;
            do {
                b = *data++;
                *ptr++ = b & 0x0f0;
                b <<= 4;
                *ptr++ = b;
                i--;
            } while(i > 0);
            arg_int -= 8;
            u8x8_byte_SendBytes(u8x8, 16, buf);
        }

        while(arg_int > 0) {
            b = *data;
            u8x8_byte_SendByte(u8x8, b & 0x0f0);
            u8x8_byte_SendByte(u8x8, b << 4);
            data++;
            arg_int--;
        }
        u8x8_gpio_Delay(u8x8, U8X8_MSG_DELAY_NANO, 1);
        break;
    case U8X8_MSG_CAD_INIT:
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    default:
        return 0;
    }
    return 1;
}

/* Également utilisé par ST7588 */
/* u8x8_byte_SetDC inutilisé */
/* BYTE_START_TRANSFER démarre I2C ; BYTE_END_TRANSFER termine */
/* Après start : octet complet indique mode commande/data */

static void u8x8_i2c_data_transfer(u8x8_t* u8x8, uint8_t arg_int, void* arg_ptr) U8X8_NOINLINE;
static void u8x8_i2c_data_transfer(u8x8_t* u8x8, uint8_t arg_int, void* arg_ptr) {
    u8x8_byte_StartTransfer(u8x8);
    u8x8_byte_SendByte(u8x8, 0x040);
    u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, arg_ptr);
    u8x8_byte_EndTransfer(u8x8);
}

/* Version classique : start/stop pour chaque commande/argument */
uint8_t u8x8_cad_ssd13xx_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    uint8_t* p;
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
    case U8X8_MSG_CAD_SEND_ARG:

        u8x8_byte_StartTransfer(u8x8);

        u8x8_byte_SendByte(u8x8, 0x000);
        u8x8_byte_SendByte(u8x8, arg_int);
        u8x8_byte_EndTransfer(u8x8);
        break;
    case U8X8_MSG_CAD_SEND_DATA:

        /* FeatherWing OLED 32u4 : transferts longs impossibles */
        /* Transferts fractionnés */
        /* Buffer Wire Arduino probablement limité à 32 octets */

        /* Limite CAD 24 octets, plus octet DC */

        p = arg_ptr;
        while(arg_int > 24) {
            u8x8_i2c_data_transfer(u8x8, 24, p);
            arg_int -= 24;
            p += 24;
        }
        u8x8_i2c_data_transfer(u8x8, arg_int, p);
        break;
    case U8X8_MSG_CAD_INIT:
        /* Adresse I2C par défaut disponible avant start transfer */
        if(u8x8->i2c_address == 255) u8x8->i2c_address = 0x078;
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        /* Commandes CAD transfer ignorées */
        break;
    default:
        return 0;
    }
    return 1;
}

/* Version rapide : moins de start/stop ; issue #735 */
uint8_t u8x8_cad_ssd13xx_fast_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    static uint8_t in_transfer = 0;
    uint8_t* p;
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:

        /* Arguments d’une commande limités à 31 octets */
        /* Gain ~4% face à version classique */
        if(in_transfer != 0) u8x8_byte_EndTransfer(u8x8);

        u8x8_byte_StartTransfer(u8x8);
        u8x8_byte_SendByte(u8x8, 0x000);
        u8x8_byte_SendByte(u8x8, arg_int);
        in_transfer = 1;

        /* Séquence init complète requise dans buffer Wire */
        /* Limite Wire : 32 octets */
        /* Gain ~6% face à version classique */

        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:
        if(in_transfer != 0) u8x8_byte_EndTransfer(u8x8);

        /* FeatherWing OLED 32u4 : transferts longs impossibles */
        /* Transferts fractionnés */
        /* Buffer Wire Arduino probablement limité à 32 octets */

        /* Limite CAD 24 octets, plus octet DC */

        p = arg_ptr;
        while(arg_int > 24) {
            u8x8_i2c_data_transfer(u8x8, 24, p);
            arg_int -= 24;
            p += 24;
        }
        u8x8_i2c_data_transfer(u8x8, arg_int, p);
        in_transfer = 0;
        break;
    case U8X8_MSG_CAD_INIT:
        /* Adresse I2C par défaut disponible avant start transfer */
        if(u8x8->i2c_address == 255) u8x8->i2c_address = 0x078;
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
        in_transfer = 0;
        break;
    case U8X8_MSG_CAD_END_TRANSFER:
        if(in_transfer != 0) u8x8_byte_EndTransfer(u8x8);
        in_transfer = 0;
        break;
    default:
        return 0;
    }
    return 1;
}

/* ST75256 : copie SSD13xx avec arg=1 */
/* CAD001 SSD13xx adapté en CAD011 */
uint8_t u8x8_cad_st75256_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    uint8_t* p;
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        u8x8_byte_StartTransfer(u8x8);
        u8x8_byte_SendByte(u8x8, 0x000);
        u8x8_byte_SendByte(u8x8, arg_int);
        u8x8_byte_EndTransfer(u8x8);
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_StartTransfer(u8x8);
        u8x8_byte_SendByte(u8x8, 0x040);
        u8x8_byte_SendByte(u8x8, arg_int);
        u8x8_byte_EndTransfer(u8x8);
        break;
    case U8X8_MSG_CAD_SEND_DATA:

        p = arg_ptr;
        while(arg_int > 24) {
            u8x8_i2c_data_transfer(u8x8, 24, p);
            arg_int -= 24;
            p += 24;
        }
        u8x8_i2c_data_transfer(u8x8, arg_int, p);
        break;
    case U8X8_MSG_CAD_INIT:
        /* Adresse I2C par défaut disponible avant start transfer */
        if(u8x8->i2c_address == 255) u8x8->i2c_address = 0x078; /* ST75256 : souvent 0x07e */
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
    case U8X8_MSG_CAD_END_TRANSFER:
        /* Commandes CAD transfer ignorées */
        break;
    default:
        return 0;
    }
    return 1;
}

/* Issue #865 : https://github.com/olikraus/u8g2/issues/865 */
/* Workaround : retirer boucle while ou augmenter sa limite */
uint8_t u8x8_cad_ld7032_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    static uint8_t in_transfer = 0;
    uint8_t* p;
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
        if(in_transfer != 0) u8x8_byte_EndTransfer(u8x8);
        u8x8_byte_StartTransfer(u8x8);
        u8x8_byte_SendByte(u8x8, arg_int);
        in_transfer = 1;
        break;
    case U8X8_MSG_CAD_SEND_ARG:
        u8x8_byte_SendByte(u8x8, arg_int);
        break;
    case U8X8_MSG_CAD_SEND_DATA:

        /* FeatherWing OLED 32u4 : transferts longs impossibles */
        /* Transferts fractionnés */
        /* Buffer Wire Arduino probablement limité à 32 octets */

        /* Limite CAD 24 octets, plus octet DC */

        p = arg_ptr;
        while(arg_int > 24) {
            u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, 24, p);
            arg_int -= 24;
            p += 24;
            u8x8_byte_EndTransfer(u8x8);
            u8x8_byte_StartTransfer(u8x8);
            u8x8_byte_SendByte(u8x8, 0x08);
        }
        u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, p);
        break;
    case U8X8_MSG_CAD_INIT:
        /* Adresse I2C par défaut disponible avant start transfer */
        if(u8x8->i2c_address == 255) u8x8->i2c_address = 0x060;
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
        in_transfer = 0;
        break;
    case U8X8_MSG_CAD_END_TRANSFER:
        if(in_transfer != 0) u8x8_byte_EndTransfer(u8x8);
        break;
    default:
        return 0;
    }
    return 1;
}

/* u8x8_byte_SetDC inutilisé */
/* Bit DC encodé dans l’adresse */
uint8_t u8x8_cad_uc16xx_i2c(u8x8_t* u8x8, uint8_t msg, uint8_t arg_int, void* arg_ptr) {
    static uint8_t in_transfer = 0;
    static uint8_t is_data = 0;
    uint8_t* p;
    switch(msg) {
    case U8X8_MSG_CAD_SEND_CMD:
    case U8X8_MSG_CAD_SEND_ARG:
        if(in_transfer != 0) {
            if(is_data != 0) {
                /* Mode transfer actif pour data */
                u8x8_byte_EndTransfer(u8x8);
                /* Effacement des deux bits inférieurs de l’adresse */
                u8x8_SetI2CAddress(u8x8, u8x8_GetI2CAddress(u8x8) & 0x0fc);
                u8x8_byte_StartTransfer(u8x8);
            }
        } else {
            /* Effacement des deux bits inférieurs de l’adresse */
            u8x8_SetI2CAddress(u8x8, u8x8_GetI2CAddress(u8x8) & 0x0fc);
            u8x8_byte_StartTransfer(u8x8);
        }
        u8x8_byte_SendByte(u8x8, arg_int);
        in_transfer = 1;
        break;
    case U8X8_MSG_CAD_SEND_DATA:
        if(in_transfer != 0) {
            if(is_data == 0) {
                /* Mode transfer actif pour data */
                u8x8_byte_EndTransfer(u8x8);
                /* Effacement des deux bits inférieurs de l’adresse */
                u8x8_SetI2CAddress(u8x8, (u8x8_GetI2CAddress(u8x8) & 0x0fc) | 2);
                u8x8_byte_StartTransfer(u8x8);
            }
        } else {
            /* Effacement des deux bits inférieurs de l’adresse */
            u8x8_SetI2CAddress(u8x8, (u8x8_GetI2CAddress(u8x8) & 0x0fc) | 2);
            u8x8_byte_StartTransfer(u8x8);
        }
        in_transfer = 1;

        p = arg_ptr;
        while(arg_int > 24) {
            u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, 24, p);
            arg_int -= 24;
            p += 24;
            u8x8_byte_EndTransfer(u8x8);
            u8x8_byte_StartTransfer(u8x8);
        }
        u8x8->byte_cb(u8x8, U8X8_MSG_CAD_SEND_DATA, arg_int, p);

        break;
    case U8X8_MSG_CAD_INIT:
        /* Adresse I2C par défaut disponible avant start transfer */
        if(u8x8->i2c_address == 255) u8x8->i2c_address = 0x070;
        return u8x8->byte_cb(u8x8, msg, arg_int, arg_ptr);
    case U8X8_MSG_CAD_START_TRANSFER:
        in_transfer = 0;
        /* Start différé jusqu’au choix commande/data */
        break;
    case U8X8_MSG_CAD_END_TRANSFER:
        if(in_transfer != 0) u8x8_byte_EndTransfer(u8x8);
        in_transfer = 0;
        break;
    default:
        return 0;
    }
    return 1;
}
