/*

  u8g2_intersection.c 
  
  Intersection calculation, code taken from u8g_clip.c

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

#ifdef __GNUC__
#define U8G2_ALWAYS_INLINE __inline__ __attribute__((always_inline))
#else
#define U8G2_ALWAYS_INLINE
#endif

#if defined(U8G2_WITH_INTERSECTION) || defined(U8G2_WITH_CLIP_WINDOW_SUPPORT)

#ifdef OLD_VERSION_WITH_SYMETRIC_BOUNDARIES

/* Intersection suppose a1<=a2 */

/* Intersection avec bornes supérieures incluses */

static uint8_t u8g2_is_intersection_decision_tree(
    u8g2_uint_t a0,
    u8g2_uint_t a1,
    u8g2_uint_t v0,
    u8g2_uint_t v1) {
    if(v0 <= a1) {
        if(v1 >= a0) {
            return 1;
        } else {
            if(v0 > v1) {
                return 1;
            } else {
                return 0;
            }
        }
    } else {
        if(v1 >= a0) {
            if(v0 > v1) {
                return 1;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
}

#endif

/* Bornes a1/v1 exclues ; v0==v1 non supporté, renvoie 1 */

uint8_t u8g2_is_intersection_decision_tree(
    u8g2_uint_t a0,
    u8g2_uint_t a1,
    u8g2_uint_t v0,
    u8g2_uint_t v1) {
    if(v0 < a1) // v0 <= a1
    {
        if(v1 > a0) // v1 >= a0
        {
            return 1;
        } else {
            if(v0 > v1) // v0 > v1
            {
                return 1;
            } else {
                return 0;
            }
        }
    } else {
        if(v1 > a0) // v1 >= a0
        {
            if(v0 > v1) // v0 > v1
            {
                return 1;
            } else {
                return 0;
            }
        } else {
            return 0;
        }
    }
}

/* Bornes supérieures exclues */
uint8_t u8g2_IsIntersection(
    u8g2_t* u8g2,
    u8g2_uint_t x0,
    u8g2_uint_t y0,
    u8g2_uint_t x1,
    u8g2_uint_t y1) {
    if(u8g2_is_intersection_decision_tree(u8g2->user_y0, u8g2->user_y1, y0, y1) == 0) return 0;

    return u8g2_is_intersection_decision_tree(u8g2->user_x0, u8g2->user_x1, x0, x1);
}

#endif
