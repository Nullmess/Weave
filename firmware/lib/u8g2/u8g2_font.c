/*

  u8g2_font.c

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

/* Taille données font, sans struct */

#define U8G2_FONT_DATA_STRUCT_SIZE 23

/* Offsets font : 0 glyph_cnt ; 1 bbx_mode */
/* 2/3 bits RLE ; 4/5 width/height ; 6/7 x/y ; 8 delta_x */
/* 9/10 max width/height ; 11/12 x/y offsets */
/* 13/14 ascent A/descent g ; 15/16 ascent (/descent ) */
/* 17/18 début A ; 19/20 début a ; 21/22 début Unicode */
/* Positions 16 bits : octet fort puis faible */
/* Modes : 0 proportionnel ; 1 hauteur commune ; 2 mono ; 3 multiple 8 */
/* Mode t compact : hauteurs variables, transparent uniquement */
/* Mode h : hauteur maximale selon suffixe r/f/n */
/* Mode h : HEIGHT_MODE_ALL, transparent ou solid */

/* Boîte minimale contenant la string */
void u8g2_font_GetStrSize(const void* font, const char* s, u8g2_uint_t* width, u8g2_uint_t* height);
void u8g2_font_GetStrSizeP(
    const void* font,
    const char* s,
    u8g2_uint_t* width,
    u8g2_uint_t* height);

/* Placement texte depuis coin bas gauche connu */
void u8g2_font_AdjustXYToDraw(const void* font, const char* s, u8g2_uint_t* x, u8g2_uint_t* y);
void u8g2_font_AdjustXYToDrawP(const void* font, const char* s, u8g2_uint_t* x, u8g2_uint_t* y);

/* Boîte minimale depuis origine baseline */
void u8g2_font_GetStrMinBox(
    u8g2_t* u8g2,
    const void* font,
    const char* s,
    u8g2_uint_t* x,
    u8g2_uint_t* y,
    u8g2_uint_t* width,
    u8g2_uint_t* height);

/* Sans NOINLINE : code AVR plus compact */

static uint8_t u8g2_font_get_byte(const uint8_t* font, uint8_t offset) {
    font += offset;
    return u8x8_pgm_read(font);
}

static uint16_t u8g2_font_get_word(const uint8_t* font, uint8_t offset) U8G2_NOINLINE;
static uint16_t u8g2_font_get_word(const uint8_t* font, uint8_t offset) {
    uint16_t pos;
    font += offset;
    pos = u8x8_pgm_read(font);
    font++;
    pos <<= 8;
    pos += u8x8_pgm_read(font);
    return pos;
}

void u8g2_read_font_info(u8g2_font_info_t* font_info, const uint8_t* font) {
    /* Offset 0 */
    font_info->glyph_cnt = u8g2_font_get_byte(font, 0);
    font_info->bbx_mode = u8g2_font_get_byte(font, 1);
    font_info->bits_per_0 = u8g2_font_get_byte(font, 2);
    font_info->bits_per_1 = u8g2_font_get_byte(font, 3);

    /* Offset 4 */
    font_info->bits_per_char_width = u8g2_font_get_byte(font, 4);
    font_info->bits_per_char_height = u8g2_font_get_byte(font, 5);
    font_info->bits_per_char_x = u8g2_font_get_byte(font, 6);
    font_info->bits_per_char_y = u8g2_font_get_byte(font, 7);
    font_info->bits_per_delta_x = u8g2_font_get_byte(font, 8);

    /* Offset 9 */
    font_info->max_char_width = u8g2_font_get_byte(font, 9);
    font_info->max_char_height = u8g2_font_get_byte(font, 10);
    font_info->x_offset = u8g2_font_get_byte(font, 11);
    font_info->y_offset = u8g2_font_get_byte(font, 12);

    /* Offset 13 */
    font_info->ascent_A = u8g2_font_get_byte(font, 13);
    font_info->descent_g = u8g2_font_get_byte(font, 14);
    font_info->ascent_para = u8g2_font_get_byte(font, 15);
    font_info->descent_para = u8g2_font_get_byte(font, 16);

    /* Offset 17 */
    font_info->start_pos_upper_A = u8g2_font_get_word(font, 17);
    font_info->start_pos_lower_a = u8g2_font_get_word(font, 19);

    /* Offset 21 */
#ifdef U8G2_WITH_UNICODE
    font_info->start_pos_unicode = u8g2_font_get_word(font, 21);
#endif
}

size_t u8g2_GetFontSize(const uint8_t* font_arg) {
    uint16_t e;
    const uint8_t* font = font_arg;
    font += U8G2_FONT_DATA_STRUCT_SIZE;

    for(;;) {
        if(u8x8_pgm_read(font + 1) == 0) break;
        font += u8x8_pgm_read(font + 1);
    }

    font += 2;

    font += u8g2_font_get_word(font, 0);

    for(;;) {
        e = u8x8_pgm_read(font);
        e <<= 8;
        e |= u8x8_pgm_read(font + 1);
        if(e == 0) break;
        font += u8x8_pgm_read(font + 2);
    }

    return (font - font_arg) + 2;
}

uint8_t u8g2_GetFontBBXWidth(u8g2_t* u8g2) {
    return u8g2->font_info.max_char_width; /* Nouvelle structure informations font */
}

uint8_t u8g2_GetFontBBXHeight(u8g2_t* u8g2) {
    return u8g2->font_info.max_char_height; /* Nouvelle structure informations font */
}

int8_t u8g2_GetFontBBXOffX(u8g2_t* u8g2) U8G2_NOINLINE;
int8_t u8g2_GetFontBBXOffX(u8g2_t* u8g2) {
    return u8g2->font_info.x_offset; /* Nouvelle structure informations font */
}

int8_t u8g2_GetFontBBXOffY(u8g2_t* u8g2) U8G2_NOINLINE;
int8_t u8g2_GetFontBBXOffY(u8g2_t* u8g2) {
    return u8g2->font_info.y_offset; /* Nouvelle structure informations font */
}

uint8_t u8g2_GetFontCapitalAHeight(u8g2_t* u8g2) U8G2_NOINLINE;
uint8_t u8g2_GetFontCapitalAHeight(u8g2_t* u8g2) {
    return u8g2->font_info.ascent_A; /* Nouvelle structure informations font */
}

uint8_t u8g2_font_decode_get_unsigned_bits(u8g2_font_decode_t* f, uint8_t cnt) {
    uint8_t val;
    uint8_t bit_pos = f->decode_bit_pos;
    uint8_t bit_pos_plus_cnt;

    val = u8x8_pgm_read(f->decode_ptr);

    val >>= bit_pos;
    bit_pos_plus_cnt = bit_pos;
    bit_pos_plus_cnt += cnt;
    if(bit_pos_plus_cnt >= 8) {
        uint8_t s = 8;
        s -= bit_pos;
        f->decode_ptr++;

        val |= u8x8_pgm_read(f->decode_ptr) << (s);

        bit_pos_plus_cnt -= 8;
    }
    val &= (1U << cnt) - 1;

    f->decode_bit_pos = bit_pos_plus_cnt;
    return val;
}

/* Décodage signé centré sur zéro : bits-2^(cnt-1) */

int8_t u8g2_font_decode_get_signed_bits(u8g2_font_decode_t* f, uint8_t cnt) {
    int8_t v, d;
    v = (int8_t)u8g2_font_decode_get_unsigned_bits(f, cnt);
    d = 1;
    cnt--;
    d <<= cnt;
    v -= d;
    return v;

}

#ifdef U8G2_WITH_FONT_ROTATION
u8g2_uint_t u8g2_add_vector_y(u8g2_uint_t dy, int8_t x, int8_t y, uint8_t dir) {
    switch(dir) {
    case 0:
        dy += y;
        break;
    case 1:
        dy += x;
        break;
    case 2:
        dy -= y;
        break;
    default:
        dy -= x;
        break;
    }
    return dy;
}

u8g2_uint_t u8g2_add_vector_x(u8g2_uint_t dx, int8_t x, int8_t y, uint8_t dir) {
    switch(dir) {
    case 0:
        dx += x;
        break;
    case 1:
        dx -= y;
        break;
    case 2:
        dx -= x;
        break;
    default:
        dx += y;
        break;
    }
    return dx;
}

#endif

/* Dessin RLE : longueur quelconque, wrap aux bords du glyphe */

void u8g2_font_decode_len(u8g2_t* u8g2, uint8_t len, uint8_t is_foreground) {
    uint8_t cnt;
    uint8_t rem; /* Pixels restants avant bord droit */
    uint8_t current;
    /* current égal à cnt ou rem */

    /* Coordonnées locales du glyphe */
    uint8_t lx, ly;

    /* Position cible écran */
    u8g2_uint_t x, y;

    u8g2_font_decode_t* decode = &(u8g2->font_decode);

    cnt = len;

    lx = decode->x;
    ly = decode->y;

    for(;;) {

        rem = decode->glyph_width;
        rem -= lx;

        /* Longueur bornée par bord droit */

        current = rem;
        if(cnt < rem) current = cnt;

        /* Rotation autour du point cible du glyphe */

        x = decode->target_x;
        y = decode->target_y;

#ifdef U8G2_WITH_FONT_ROTATION

        x = u8g2_add_vector_x(x, lx, ly, decode->dir);
        y = u8g2_add_vector_y(y, lx, ly, decode->dir);

#else
        x += lx;
        y += ly;
#endif

        if(is_foreground) {
            u8g2->draw_color = decode->fg_color; /* draw_color restauré ensuite */
            u8g2_DrawHVLine(
                u8g2,
                x,
                y,
                current,
#ifdef U8G2_WITH_FONT_ROTATION
                  decode->dir
#else
                0
#endif
            );
        } else if(decode->is_transparent == 0) {
            u8g2->draw_color = decode->bg_color; /* draw_color restauré ensuite */
            u8g2_DrawHVLine(
                u8g2,
                x,
                y,
                current,
#ifdef U8G2_WITH_FONT_ROTATION
                  decode->dir
#else
                0
#endif
            );
        }

        if(cnt < rem) break;
        cnt -= rem;
        lx = 0;
        ly++;
    }
    lx += cnt;

    decode->x = lx;
    decode->y = ly;
}

static void u8g2_font_setup_decode(u8g2_t* u8g2, const uint8_t* glyph_data) {
    u8g2_font_decode_t* decode = &(u8g2->font_decode);
    decode->decode_ptr = glyph_data;
    decode->decode_bit_pos = 0;

    /* Déjà effectué pendant recherche du glyphe */

    decode->glyph_width =
        u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_width);
    decode->glyph_height =
        u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_char_height);

    decode->fg_color = u8g2->draw_color;
    decode->bg_color = (decode->fg_color == 0 ? 1 : 0);
}

/* Décode le glyphe compressé ; renvoie son avance delta_x */

int8_t u8g2_font_decode_glyph(u8g2_t* u8g2, const uint8_t* glyph_data) {
    uint8_t a, b;
    int8_t x, y;
    int8_t d;
    int8_t h;
    u8g2_font_decode_t* decode = &(u8g2->font_decode);

    u8g2_font_setup_decode(u8g2, glyph_data);
    h = u8g2->font_decode.glyph_height;

    x = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_x);
    y = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_char_y);
    d = u8g2_font_decode_get_signed_bits(decode, u8g2->font_info.bits_per_delta_x);

    if(decode->glyph_width > 0) {
#ifdef U8G2_WITH_FONT_ROTATION
        decode->target_x = u8g2_add_vector_x(decode->target_x, x, -(h + y), decode->dir);
        decode->target_y = u8g2_add_vector_y(decode->target_y, x, -(h + y), decode->dir);

#else
        decode->target_x += x;
        decode->target_y -= h + y;
#endif

#ifdef U8G2_WITH_INTERSECTION
        {
            u8g2_uint_t x0, x1, y0, y1;
            x0 = decode->target_x;
            y0 = decode->target_y;
            x1 = x0;
            y1 = y0;

#ifdef U8G2_WITH_FONT_ROTATION
            switch(decode->dir) {
            case 0:
                x1 += decode->glyph_width;
                y1 += h;
                break;
            case 1:
                x0 -= h;
                x0++; /* Décalage bas pour bornes d’intersection asymétriques */
                x1++;
                y1 += decode->glyph_width;
                break;
            case 2:
                x0 -= decode->glyph_width;
                x0++; /* Décalage bas pour bornes d’intersection asymétriques */
                x1++;
                y0 -= h;
                y0++; /* Décalage bas pour bornes d’intersection asymétriques */
                y1++;
                break;
            case 3:
                x1 += h;
                y0 -= decode->glyph_width;
                y0++; /* Décalage bas pour bornes d’intersection asymétriques */
                y1++;
                break;
            }
#else
            x1 += decode->glyph_width;
            y1 += h;
#endif

            if(u8g2_IsIntersection(u8g2, x0, y0, x1, y1) == 0) return d;
        }
#endif

        decode->x = 0;
        decode->y = 0;

        for(;;) {
            a = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_0);
            b = u8g2_font_decode_get_unsigned_bits(decode, u8g2->font_info.bits_per_1);
            do {
                u8g2_font_decode_len(u8g2, a, 0);
                u8g2_font_decode_len(u8g2, b, 1);
            } while(u8g2_font_decode_get_unsigned_bits(decode, 1) != 0);

            if(decode->y >= h) break;
        }

        /* Décodage modifie draw_color ; restauration nécessaire */
        u8g2->draw_color = decode->fg_color;
    }
    return d;
}

/* Données ASCII/Unicode ; NULL si glyphe absent */

const uint8_t* u8g2_font_get_glyph_data(u8g2_t* u8g2, uint16_t encoding) {
    const uint8_t* font = u8g2->font;
    font += U8G2_FONT_DATA_STRUCT_SIZE;

    if(encoding <= 255) {
        if(encoding >= 'a') {
            font += u8g2->font_info.start_pos_lower_a;
        } else if(encoding >= 'A') {
            font += u8g2->font_info.start_pos_upper_A;
        }

        for(;;) {
            if(u8x8_pgm_read(font + 1) == 0) break;
            if(u8x8_pgm_read(font) == encoding) {
                return font + 2;
            }
            font += u8x8_pgm_read(font + 1);
        }
    }
#ifdef U8G2_WITH_UNICODE
    else {
        uint16_t e;
        const uint8_t* unicode_lookup_table;

        /* Remplacé par la nouvelle table index */

        font += u8g2->font_info.start_pos_unicode;
        unicode_lookup_table = font;

        /* Issue #596 : index Unicode pour début du glyphe */
        do {
            font += u8g2_font_get_word(unicode_lookup_table, 0);
            e = u8g2_font_get_word(unicode_lookup_table, 2);
            unicode_lookup_table += 4;
        } while(e < encoding);

        for(;;) {
            e = u8x8_pgm_read(font);
            e <<= 8;
            e |= u8x8_pgm_read(font + 1);

            /* Remplacé par la nouvelle table index */

            if(e == 0) break;

            if(e == encoding) {
                /* Remplacé par la nouvelle table index */

                return font + 3;
            }
            font += u8x8_pgm_read(font + 2);
        }
    }
#endif

    return NULL;
}

static u8g2_uint_t
    u8g2_font_draw_glyph(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding) {
    u8g2_uint_t dx = 0;
    u8g2->font_decode.target_x = x;
    u8g2->font_decode.target_y = y;
    /* is_transparent déjà initialisé */

    const uint8_t* glyph_data = u8g2_font_get_glyph_data(u8g2, encoding);
    if(glyph_data != NULL) {
        dx = u8g2_font_decode_glyph(u8g2, glyph_data);
    }
    return dx;
}

uint8_t u8g2_IsGlyph(u8g2_t* u8g2, uint16_t requested_encoding) {

    if(u8g2_font_get_glyph_data(u8g2, requested_encoding) != NULL) return 1;
    return 0;
}

/* Modifie aussi font_decode et glyph_x_offset */
int8_t u8g2_GetGlyphWidth(u8g2_t* u8g2, uint16_t requested_encoding) {
    const uint8_t* glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
    if(glyph_data == NULL) return 0;

    u8g2_font_setup_decode(u8g2, glyph_data);
    u8g2->glyph_x_offset =
        u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
    u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);

    /* Largeur dans font_decode.glyph_width */

    return u8g2_font_decode_get_signed_bits(
        &(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}

void u8g2_SetFontMode(u8g2_t* u8g2, uint8_t is_transparent) {
    u8g2->font_decode.is_transparent = is_transparent;
}

u8g2_uint_t u8g2_DrawGlyph(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, uint16_t encoding) {
#ifdef U8G2_WITH_FONT_ROTATION
    switch(u8g2->font_decode.dir) {
    case 0:
        y += u8g2->font_calc_vref(u8g2);
        break;
    case 1:
        x -= u8g2->font_calc_vref(u8g2);
        break;
    case 2:
        y -= u8g2->font_calc_vref(u8g2);
        break;
    case 3:
        x += u8g2->font_calc_vref(u8g2);
        break;
    }
#else
    y += u8g2->font_calc_vref(u8g2);
#endif
    return u8g2_font_draw_glyph(u8g2, x, y, encoding);
}

static u8g2_uint_t
    u8g2_draw_string(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str) U8G2_NOINLINE;
static u8g2_uint_t u8g2_draw_string(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str) {
    uint16_t e;
    u8g2_uint_t delta, sum;
    u8x8_utf8_init(u8g2_GetU8x8(u8g2));
    sum = 0;
    for(;;) {
        e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
        if(e == 0x0ffff) break;
        str++;
        if(e != 0x0fffe) {
            delta = u8g2_DrawGlyph(u8g2, x, y, e);

#ifdef U8G2_WITH_FONT_ROTATION
            switch(u8g2->font_decode.dir) {
            case 0:
                x += delta;
                break;
            case 1:
                y += delta;
                break;
            case 2:
                x -= delta;
                break;
            case 3:
                y -= delta;
                break;
            }

#else
            x += delta;
#endif

            sum += delta;
        }
    }
    return sum;
}

u8g2_uint_t u8g2_DrawStr(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str) {
    u8g2->u8x8.next_cb = u8x8_ascii_next;
    return u8g2_draw_string(u8g2, x, y, str);
}

/* UTF-8 : 1..6 octets selon le format historique */
/* 0..7F : 0xxxxxxx ; 80..7FF : 110xxxxx + continuation */
/* 800..FFFF : 1110xxxx ; 10000..1FFFFF : 11110xxx */
/* Séquences historiques 5/6 octets : 111110xx/1111110x */
/* Continuation : 10xxxxxx */

u8g2_uint_t u8g2_DrawUTF8(u8g2_t* u8g2, u8g2_uint_t x, u8g2_uint_t y, const char* str) {
    u8g2->u8x8.next_cb = u8x8_utf8_next;
    return u8g2_draw_string(u8g2, x, y, str);
}

u8g2_uint_t u8g2_DrawExtendedUTF8(
    u8g2_t* u8g2,
    u8g2_uint_t x,
    u8g2_uint_t y,
    uint8_t to_left,
    u8g2_kerning_t* kerning,
    const char* str) {
    u8g2->u8x8.next_cb = u8x8_utf8_next;
    uint16_t e_prev = 0x0ffff;
    uint16_t e;
    u8g2_uint_t delta, sum, k;
    u8x8_utf8_init(u8g2_GetU8x8(u8g2));
    sum = 0;
    for(;;) {
        e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
        if(e == 0x0ffff) break;
        str++;
        if(e != 0x0fffe) {
            delta = u8g2_GetGlyphWidth(u8g2, e);

            if(to_left) {
                k = u8g2_GetKerning(u8g2, kerning, e, e_prev);
                delta -= k;
                x -= delta;
            } else {
                k = u8g2_GetKerning(u8g2, kerning, e_prev, e);
                delta -= k;
            }
            e_prev = e;

            u8g2_DrawGlyph(u8g2, x, y, e);
            if(to_left) {
            } else {
                x += delta;
                x -= k;
            }

            sum += delta;
        }
    }
    return sum;
}

u8g2_uint_t u8g2_DrawExtUTF8(
    u8g2_t* u8g2,
    u8g2_uint_t x,
    u8g2_uint_t y,
    uint8_t to_left,
    const uint16_t* kerning_table,
    const char* str) {
    u8g2->u8x8.next_cb = u8x8_utf8_next;
    uint16_t e_prev = 0x0ffff;
    uint16_t e;
    u8g2_uint_t delta, sum, k;
    u8x8_utf8_init(u8g2_GetU8x8(u8g2));
    sum = 0;
    for(;;) {
        e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
        if(e == 0x0ffff) break;
        str++;
        if(e != 0x0fffe) {
            delta = u8g2_GetGlyphWidth(u8g2, e);

            if(to_left) {
                k = u8g2_GetKerningByTable(u8g2, kerning_table, e, e_prev);
                delta -= k;
                x -= delta;
            } else {
                k = u8g2_GetKerningByTable(u8g2, kerning_table, e_prev, e);
                delta -= k;
            }
            e_prev = e;

            if(to_left) {
            } else {
                x += delta;
            }
            u8g2_DrawGlyph(u8g2, x, y, e);
            if(to_left) {
            } else {

            }

            sum += delta;
        }
    }
    return sum;
}

void u8g2_UpdateRefHeight(u8g2_t* u8g2) {
    if(u8g2->font == NULL) return;
    u8g2->font_ref_ascent = u8g2->font_info.ascent_A;
    u8g2->font_ref_descent = u8g2->font_info.descent_g;
    if(u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_TEXT) {
    } else if(u8g2->font_height_mode == U8G2_FONT_HEIGHT_MODE_XTEXT) {
        if(u8g2->font_ref_ascent < u8g2->font_info.ascent_para)
            u8g2->font_ref_ascent = u8g2->font_info.ascent_para;
        if(u8g2->font_ref_descent > u8g2->font_info.descent_para)
            u8g2->font_ref_descent = u8g2->font_info.descent_para;
    } else {
        if(u8g2->font_ref_ascent < u8g2->font_info.max_char_height + u8g2->font_info.y_offset)
            u8g2->font_ref_ascent = u8g2->font_info.max_char_height + u8g2->font_info.y_offset;
        if(u8g2->font_ref_descent > u8g2->font_info.y_offset)
            u8g2->font_ref_descent = u8g2->font_info.y_offset;
    }
}

void u8g2_SetFontRefHeightText(u8g2_t* u8g2) {
    u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_TEXT;
    u8g2_UpdateRefHeight(u8g2);
}

void u8g2_SetFontRefHeightExtendedText(u8g2_t* u8g2) {
    u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_XTEXT;
    u8g2_UpdateRefHeight(u8g2);
}

void u8g2_SetFontRefHeightAll(u8g2_t* u8g2) {
    u8g2->font_height_mode = U8G2_FONT_HEIGHT_MODE_ALL;
    u8g2_UpdateRefHeight(u8g2);
}

u8g2_uint_t u8g2_font_calc_vref_font(U8X8_UNUSED u8g2_t* u8g2) {
    return 0;
}

void u8g2_SetFontPosBaseline(u8g2_t* u8g2) {
    u8g2->font_calc_vref = u8g2_font_calc_vref_font;
}

u8g2_uint_t u8g2_font_calc_vref_bottom(u8g2_t* u8g2) {
    return (u8g2_uint_t)(u8g2->font_ref_descent);
}

void u8g2_SetFontPosBottom(u8g2_t* u8g2) {
    u8g2->font_calc_vref = u8g2_font_calc_vref_bottom;
}

u8g2_uint_t u8g2_font_calc_vref_top(u8g2_t* u8g2) {
    u8g2_uint_t tmp;
    /* Référence un pixel au-dessus du glyphe */
    tmp = (u8g2_uint_t)(u8g2->font_ref_ascent);
    tmp++;
    return tmp;
}

void u8g2_SetFontPosTop(u8g2_t* u8g2) {
    u8g2->font_calc_vref = u8g2_font_calc_vref_top;
}

u8g2_uint_t u8g2_font_calc_vref_center(u8g2_t* u8g2) {
    int8_t tmp;
    tmp = u8g2->font_ref_ascent;
    tmp -= u8g2->font_ref_descent;
    tmp /= 2;
    tmp += u8g2->font_ref_descent;
    return tmp;
}

void u8g2_SetFontPosCenter(u8g2_t* u8g2) {
    u8g2->font_calc_vref = u8g2_font_calc_vref_center;
}

void u8g2_SetFont(u8g2_t* u8g2, const uint8_t* font) {
    if(u8g2->font != font) {

        u8g2->font = font;
        u8g2_read_font_info(&(u8g2->font_info), font);
        u8g2_UpdateRefHeight(u8g2);

    }
}

static uint8_t u8g2_is_all_valid(u8g2_t* u8g2, const char* str) U8G2_NOINLINE;
static uint8_t u8g2_is_all_valid(u8g2_t* u8g2, const char* str) {
    uint16_t e;
    u8x8_utf8_init(u8g2_GetU8x8(u8g2));
    for(;;) {
        e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
        if(e == 0x0ffff) break;
        str++;
        if(e != 0x0fffe) {
            if(u8g2_font_get_glyph_data(u8g2, e) == NULL) return 0;
        }
    }
    return 1;
}

uint8_t u8g2_IsAllValidUTF8(u8g2_t* u8g2, const char* str) {
    u8g2->u8x8.next_cb = u8x8_utf8_next;
    return u8g2_is_all_valid(u8g2, str);
}

/* Largeur string encore approximative : inclut offset initial */
static u8g2_long_t u8g2_string_width(u8g2_t* u8g2, const char* str) U8G2_NOINLINE;
static u8g2_long_t u8g2_string_width(u8g2_t* u8g2, const char* str) {
    uint16_t e;
    u8g2_uint_t dx;
    u8g2_long_t w;

    u8g2->font_decode.glyph_width = 0;
    u8x8_utf8_init(u8g2_GetU8x8(u8g2));

    w = 0;
    dx = 0;

    for(;;) {
        e = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
        if(e == 0x0ffff) break;
        str++;
        if(e != 0x0fffe) {
            dx = u8g2_GetGlyphWidth(u8g2, e);
            w += dx;
        }
    }

    /* Issue #16 : ne pas ajuster largeur nulle */
    if(u8g2->font_decode.glyph_width != 0) {
        w -= dx;
        w += u8g2->font_decode
                 .glyph_width; /* Largeur pixel, mise à jour par GetGlyphWidth */
        /* Issue #46 : inclure offset x */
        w += u8g2->glyph_x_offset; /* Mis à jour par u8g2_GetGlyphWidth */
    }

    return w;
}

static void u8g2_GetGlyphHorizontalProperties(
    u8g2_t* u8g2,
    uint16_t requested_encoding,
    uint8_t* w,
    int8_t* ox,
    int8_t* dx) {
    const uint8_t* glyph_data = u8g2_font_get_glyph_data(u8g2, requested_encoding);
    if(glyph_data == NULL) return;

    u8g2_font_setup_decode(u8g2, glyph_data);
    *w = u8g2->font_decode.glyph_width;
    *ox = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_x);
    u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_char_y);
    *dx = u8g2_font_decode_get_signed_bits(&(u8g2->font_decode), u8g2->font_info.bits_per_delta_x);
}

int8_t u8g2_GetStrX(u8g2_t* u8g2, const char* s) {
    uint8_t w;
    int8_t ox, dx;
    u8g2_GetGlyphHorizontalProperties(u8g2, *s, &w, &ox, &dx);
    return ox;
}

static u8g2_uint_t u8g2_calculate_exact_string_width(u8g2_t* u8g2, const char* str) {
    u8g2_uint_t w;
    uint16_t enc;
    uint8_t gw;
    int8_t ox, dx;

    w = 0;

    do {
        enc = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
        str++;
    } while(enc == 0x0fffe);

    if(enc == 0x0ffff) return w;

    /* Glyphe absent : paramètres cached nuls */
    u8g2_GetGlyphHorizontalProperties(u8g2, enc, &gw, &ox, &dx);

    /* strlen(s) == 1:       width = width(s[0]) */
    /* strlen(s) == 2:       width = - offx(s[0]) + deltax(s[0]) + offx(s[1]) + width(s[1]) */
    /* strlen(s) == 3:       width = - offx(s[0]) + deltax(s[0]) + deltax(s[1]) + offx(s[2]) + width(s[2]) */

    /* Strings multiples : offset x initial négatif */
    /* String unique : compensation après boucle */
    w = -ox;
    for(;;) {

        do {
            enc = u8g2->u8x8.next_cb(u8g2_GetU8x8(u8g2), (uint8_t)*str);
            str++;
        } while(enc == 0x0fffe);
        if(enc == 0x0ffff) break;

        u8g2_GetGlyphHorizontalProperties(u8g2, enc, &gw, &ox, &dx);

        w += dx;
    }

    /* Dernier espace : utiliser avance dx */
    if(enc != ' ') {
        /* String unique : annuler offset x initial */
        w += gw;
        w += ox;
    } else {
        w += dx;
    }

    return w;
}

u8g2_long_t u8g2_GetStrWidth(u8g2_t* u8g2, const char* s) {
    u8g2->u8x8.next_cb = u8x8_ascii_next;
    return u8g2_string_width(u8g2, s);
}

u8g2_uint_t u8g2_GetExactStrWidth(u8g2_t* u8g2, const char* s) {
    u8g2->u8x8.next_cb = u8x8_ascii_next;
    return u8g2_calculate_exact_string_width(u8g2, s);
}

/* UTF-8 : 1..6 octets selon le format historique */
/* 0..7F : 0xxxxxxx ; 80..7FF : 110xxxxx + continuation */
/* 800..FFFF : 1110xxxx ; 10000..1FFFFF : 11110xxx */
/* Séquences historiques 5/6 octets : 111110xx/1111110x */
/* Continuation : 10xxxxxx */

u8g2_uint_t u8g2_GetUTF8Width(u8g2_t* u8g2, const char* str) {
    u8g2->u8x8.next_cb = u8x8_utf8_next;
    return u8g2_string_width(u8g2, str);
}

void u8g2_SetFontDirection(u8g2_t* u8g2, uint8_t dir) {
#ifdef U8G2_WITH_FONT_ROTATION
    u8g2->font_decode.dir = dir;
#endif
}
