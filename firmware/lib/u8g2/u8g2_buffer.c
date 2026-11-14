/* 

  u8g2_buffer.c 

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

#include "u8g2.h"
#include <string.h>

void u8g2_ClearBuffer(u8g2_t* u8g2) {
    size_t cnt;
    cnt = u8g2_GetU8x8(u8g2)->display_info->tile_width;
    cnt *= u8g2->tile_buf_height;
    cnt *= 8;
    memset(u8g2->tile_buf_ptr, 0, cnt);
}

static void u8g2_send_tile_row(u8g2_t* u8g2, uint8_t src_tile_row, uint8_t dest_tile_row) {
    uint8_t* ptr;
    uint16_t offset;
    uint8_t w;

    w = u8g2_GetU8x8(u8g2)->display_info->tile_width;
    offset = src_tile_row;
    ptr = u8g2->tile_buf_ptr;
    offset *= w;
    offset *= 8;
    ptr += offset;
    u8x8_DrawTile(u8g2_GetU8x8(u8g2), 0, dest_tile_row, w, ptr);
}

/* SSD1606 : RefreshDisplay requis après écriture RAM */

static void u8g2_send_buffer(u8g2_t* u8g2) U8X8_NOINLINE;
static void u8g2_send_buffer(u8g2_t* u8g2) {
    uint8_t src_row;
    uint8_t src_max;
    uint8_t dest_row;
    uint8_t dest_max;

    src_row = 0;
    src_max = u8g2->tile_buf_height;
    dest_row = u8g2->tile_curr_row;
    dest_max = u8g2_GetU8x8(u8g2)->display_info->tile_height;

    do {
        u8g2_send_tile_row(u8g2, src_row, dest_row);
        src_row++;
        dest_row++;
    } while(src_row < src_max && dest_row < dest_max);
}

/* Ajoute DISPLAY_REFRESH pour SSD1606 */
void u8g2_SendBuffer(u8g2_t* u8g2) {
    u8g2_send_buffer(u8g2);
    u8x8_RefreshDisplay(u8g2_GetU8x8(u8g2));
}

void u8g2_SetBufferCurrTileRow(u8g2_t* u8g2, uint8_t row) {
    u8g2->tile_curr_row = row;
    u8g2->cb->update_dimension(u8g2);
    u8g2->cb->update_page_win(u8g2);
}

void u8g2_FirstPage(u8g2_t* u8g2) {
    if(u8g2->is_auto_page_clear) {
        u8g2_ClearBuffer(u8g2);
    }
    u8g2_SetBufferCurrTileRow(u8g2, 0);
}

uint8_t u8g2_NextPage(u8g2_t* u8g2) {
    uint8_t row;
    u8g2_send_buffer(u8g2);
    row = u8g2->tile_curr_row;
    row += u8g2->tile_buf_height;
    if(row >= u8g2_GetU8x8(u8g2)->display_info->tile_height) {
        u8x8_RefreshDisplay(u8g2_GetU8x8(u8g2));
        return 0;
    }
    if(u8g2->is_auto_page_clear) {
        u8g2_ClearBuffer(u8g2);
    }
    u8g2_SetBufferCurrTileRow(u8g2, row);
    return 1;
}

/* Full buffer uniquement ; aucune validation des arguments */
/* Coordonnées tiles : pixels divisés par 8 */
/* Rotation/mirror ignorés ; support U8x8 requis */
/* Sans refresh e-paper */

void u8g2_UpdateDisplayArea(u8g2_t* u8g2, uint8_t tx, uint8_t ty, uint8_t tw, uint8_t th) {
    uint16_t page_size;
    uint8_t* ptr;

    if(u8g2->tile_buf_height != u8g2_GetU8x8(u8g2)->display_info->tile_height)
        return;

    page_size = u8g2->pixel_buf_width;

    ptr = u8g2_GetBufferPtr(u8g2);
    ptr += tx * 8;
    ptr += page_size * ty;

    while(th > 0) {
        u8x8_DrawTile(u8g2_GetU8x8(u8g2), tx, ty, tw, ptr);
        ptr += page_size;
        ty++;
        th--;
    }
}

/* Sans refresh e-paper */
void u8g2_UpdateDisplay(u8g2_t* u8g2) {
    u8g2_send_buffer(u8g2);
}

/* Organisation mémoire vertical_top */
void u8g2_WriteBufferPBM(u8g2_t* u8g2, void (*out)(const char* s)) {
    u8x8_capture_write_pbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
    u8x8_capture_write_pbm_buffer(
        u8g2_GetBufferPtr(u8g2),
        u8g2_GetBufferTileWidth(u8g2),
        u8g2_GetBufferTileHeight(u8g2),
        u8x8_capture_get_pixel_1,
        out);
}

void u8g2_WriteBufferXBM(u8g2_t* u8g2, void (*out)(const char* s)) {
    u8x8_capture_write_xbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
    u8x8_capture_write_xbm_buffer(
        u8g2_GetBufferPtr(u8g2),
        u8g2_GetBufferTileWidth(u8g2),
        u8g2_GetBufferTileHeight(u8g2),
        u8x8_capture_get_pixel_1,
        out);
}

/* Organisation mémoire horizontal_right */
/* SH1122, LD7032, ST7920, ST7986, LC7981, T6963, SED1330, RA8835, MAX7219, LS0 */
void u8g2_WriteBufferPBM2(u8g2_t* u8g2, void (*out)(const char* s)) {
    u8x8_capture_write_pbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
    u8x8_capture_write_pbm_buffer(
        u8g2_GetBufferPtr(u8g2),
        u8g2_GetBufferTileWidth(u8g2),
        u8g2_GetBufferTileHeight(u8g2),
        u8x8_capture_get_pixel_2,
        out);
}

void u8g2_WriteBufferXBM2(u8g2_t* u8g2, void (*out)(const char* s)) {
    u8x8_capture_write_xbm_pre(u8g2_GetBufferTileWidth(u8g2), u8g2_GetBufferTileHeight(u8g2), out);
    u8x8_capture_write_xbm_buffer(
        u8g2_GetBufferPtr(u8g2),
        u8g2_GetBufferTileWidth(u8g2),
        u8g2_GetBufferTileHeight(u8g2),
        u8x8_capture_get_pixel_2,
        out);
}
