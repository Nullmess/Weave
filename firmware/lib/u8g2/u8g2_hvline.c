/*

  u8g2_hvline.c

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


  Calltree
  
    void u8g2_DrawHVLine(u8g2_t *u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir)
    u8g2->cb->draw_l90
    u8g2_draw_hv_line_2dir
    u8g2->ll_hvline(u8g2, x, y, len, dir);
    

*/

#include "u8g2.h"
#include <assert.h>

/* Intersection [a,a+len[ et [c,d[ ; len>0, c<=d requis */

static uint8_t
    u8g2_clip_intersection2(u8g2_uint_t* ap, u8g2_uint_t* len, u8g2_uint_t c, u8g2_uint_t d) {
    u8g2_uint_t a = *ap;
    u8g2_uint_t b;
    b = a;
    b += *len;

    /* Intersection [a,b[ et [c,d[ ; c<=d requis, a>b géré */

    /* Traitement a>b protège contre arguments invalides */

    /* Sans cette condition, a>b peut corrompre la mémoire */
    if(a > b) {
        /* Retour 0 immédiat gérerait mal a négatif */
        if(a < d) {
            b = d;
            b--;
        } else {
            a = c;
        }
    }

    /* À partir d’ici : a<=b */

    if(a >= d) return 0;
    if(b <= c) return 0;
    if(a < c) a = c;
    if(b > d) b = d;

    *ap = a;
    b -= a;
    *len = b;
    return 1;
}

/* x/y locaux au buffer ; len en pixels, non nul */
/* dir : 0 horizontal ; 1 vertical ; clipping avant low-level */

void u8g2_draw_hv_line_2dir(
    u8g2_t* u8g2,
    u8g2_uint_t x,
    u8g2_uint_t y,
    u8g2_uint_t len,
    uint8_t dir) {
    /* Clipping avant rotation écran */

    y -= u8g2->pixel_curr_row;

    u8g2->ll_hvline(u8g2, x, y, len, dir);
}

/* API HVLine : dir 0..3, callback applique rotation */

void u8g2_DrawHVLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len, uint8_t dir) {

    /* Callback peut appliquer rotation HVLine */

#ifdef U8G2_WITH_CLIP_WINDOW_SUPPORT
    if(u8g2->is_page_clip_window_intersection != 0)
#endif
        if(len != 0) {

            if(len > 1) {
                if(dir == 2) {
                    x -= len;
                    x++;
                } else if(dir == 3) {
                    y -= len;
                    y++;
                }
            }
            dir &= 1;

            if(dir == 0) {
                if(y < u8g2->user_y0) return;
                if(y >= u8g2->user_y1) return;
                if(u8g2_clip_intersection2(&x, &len, u8g2->user_x0, u8g2->user_x1) == 0) return;
            } else {
                if(x < u8g2->user_x0) return;
                if(x >= u8g2->user_x1) return;
                if(u8g2_clip_intersection2(&y, &len, u8g2->user_y0, u8g2->user_y1) == 0) return;
            }

            u8g2->cb->draw_l90(u8g2, x, y, len, dir);
        }
}

void u8g2_DrawHLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len) {

    u8g2_DrawHVLine(u8g2, x, y, len, 0);
}

void u8g2_DrawVLine(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, u8g2_uint_t len) {

    u8g2_DrawHVLine(u8g2, x, y, len, 1);
}

void u8g2_DrawPixel(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y) {
#ifdef U8G2_WITH_INTERSECTION
    if(y < u8g2->user_y0) return;
    if(y >= u8g2->user_y1) return;
    if(x < u8g2->user_x0) return;
    if(x >= u8g2->user_x1) return;
#endif
    u8g2_DrawHVLine(u8g2, x, y, 1, 0);
}

/* 0 efface ; 1 active ; 2 XOR */
/* OLED : lumière ; LCD : segment absorbant ; e-paper : noir */

void u8g2_SetDrawColor(u8g2_t* u8g2, uint8_t color) {
    u8g2->draw_color = color;
    if(color >= 3)
        u8g2->draw_color = 1;
}
