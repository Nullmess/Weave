/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef CS_COMMON_STR_UTIL_H_
#define CS_COMMON_STR_UTIL_H_

#include <stdarg.h>
#include <stdlib.h>

#include "mg_str.h"
#include "platform.h"

#ifndef CS_ENABLE_STRDUP
#define CS_ENABLE_STRDUP 0
#endif

#ifndef CS_ENABLE_TO64
#define CS_ENABLE_TO64 0
#endif

/* Stringification argument littéral */

#if !defined(_MSC_VER) || _MSC_VER >= 1900
#define CS_STRINGIFY_LIT(...) #__VA_ARGS__
#else
#define CS_STRINGIFY_LIT(x) #x
#endif

/* Stringification après expansion macro */

#define CS_STRINGIFY_MACRO(x) CS_STRINGIFY_LIT(x)

#ifdef __cplusplus
extern "C" {
#endif

size_t c_strnlen(const char* s, size_t maxlen);

int c_snprintf(char* buf, size_t buf_size, const char* format, ...) PRINTF_LIKE(3, 4);

int c_vsnprintf(char* buf, size_t buf_size, const char* format, va_list ap);

/* Recherche limitée aux slen premiers caractères */

const char* c_strnstr(const char* s, const char* find, size_t slen);

/* Buffer hex : 2*taille_entrée+1 octets, NUL inclus */

void cs_to_hex(char* to, const unsigned char* p, size_t len);

void cs_from_hex(char* to, const char* p, size_t len);

#if CS_ENABLE_STRDUP
/* strdup disponible uniquement si CS_ENABLE_STRDUP=1 */

char* strdup(const char* src);
#endif

#if CS_ENABLE_TO64
#include <stdint.h>

int64_t cs_to64(const char* s);
#endif

int mg_ncasecmp(const char* s1, const char* s2, size_t len);

int mg_casecmp(const char* s1, const char* s2);

/* Buffer insuffisant : allocation heap ; free si pointeur changé */

int mg_asprintf(char** buf, size_t size, const char* fmt, ...) PRINTF_LIKE(3, 4);

int mg_avprintf(char** buf, size_t size, const char* fmt, va_list ap);

/* Liste séparée par virgules, sans copie/allocation */
/* x=y : val=x, eq_val=y ; sinon eq_val vide */
/* Renvoie entrée suivante ou NULL en fin */

const char* mg_next_comma_list_entry(const char* list, struct mg_str* val, struct mg_str* eq_val);

/* Tester .p, pas .len ; dernière entrée peut avoir len=0 */

struct mg_str
    mg_next_comma_list_entry_n(struct mg_str list, struct mg_str* val, struct mg_str* eq_val);

/* Glob sans casse : * exclut slash ; ** inclut slash */
/* ? : un caractère hors slash ; |/virgule : alternatives */
/* Renvoie le nombre d’octets matched */

size_t mg_match_prefix(const char* pattern, int pattern_len, const char* str);

size_t mg_match_prefix_n(const struct mg_str pattern, const struct mg_str str);

#ifdef __cplusplus
}
#endif

#endif
