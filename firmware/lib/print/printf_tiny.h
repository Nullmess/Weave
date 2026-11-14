///////////////////////////////////////////////////////////////////////////////
// \author (c) Marco Paland (info@paland.com)
//             2014-2019, PALANDesign Hannover, Germany
//
// \license The MIT License (MIT)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// \brief Tiny printf, sprintf and snprintf implementation, optimized for speed on
//        embedded systems with a very limited resources.
//        Use this instead of bloated standard/newlib printf.
//        These routines are thread safe and reentrant.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _PRINTF_H_
#define _PRINTF_H_

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Implémentation _putchar utilisateur requise pour printf */

void _putchar(char character);

/* API printf remplacée par macros/functions suffixées underscore ; _putchar requis */

int printf_(const char* format, ...) _ATTRIBUTE((__format__(__printf__, 1, 2)));

/* Buffer suffisamment grand obligatoire ; préférer snprintf/vsnprintf */

int sprintf_(char* buffer, const char* format, ...) _ATTRIBUTE((__format__(__printf__, 2, 3)));

/* count inclut NUL ; retour taille requise hors NUL */
/* Retour >=count : troncature ; complet si 0<=ret<count */

int snprintf_(char* buffer, size_t count, const char* format, ...)
    _ATTRIBUTE((__format__(__printf__, 3, 4)));
int vsnprintf_(char* buffer, size_t count, const char* format, va_list va);

/* Nombre caractères écrits hors NUL */

int vprintf_(const char* format, va_list va);

/* Callback out(character,arg) ; retour caractères envoyés hors NUL */

int fctprintf(void (*out)(char character, void* arg), void* arg, const char* format, ...)
    _ATTRIBUTE((__format__(__printf__, 3, 4)));

#ifdef __cplusplus
}
#endif

#endif
