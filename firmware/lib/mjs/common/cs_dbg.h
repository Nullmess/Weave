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

#ifndef CS_COMMON_CS_DBG_H_
#define CS_COMMON_CS_DBG_H_

#include "platform.h"

#if CS_ENABLE_STDIO
#include <stdio.h>
#endif

#ifndef CS_ENABLE_DEBUG
#define CS_ENABLE_DEBUG 0
#endif

#ifndef CS_LOG_PREFIX_LEN
#define CS_LOG_PREFIX_LEN 24
#endif

#ifndef CS_LOG_ENABLE_TS_DIFF
#define CS_LOG_ENABLE_TS_DIFF 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* LL_INFO par défaut ; modifier via cs_log_set_level */

enum cs_log_level {
    LL_NONE = -1,
    LL_ERROR = 0,
    LL_WARN = 1,
    LL_INFO = 2,
    LL_DEBUG = 3,
    LL_VERBOSE_DEBUG = 4,

    _LL_MIN = -2,
    _LL_MAX = 5,
};

/* Messages au-dessus du niveau maximal ignorés */

void cs_log_set_level(enum cs_log_level level);

/* Filtre prefix=level séparé par virgules ; premier match prioritaire */
/* Préfixe tel qu’affiché, numéro ligne inclus ; match partiel accepté */
/* Sans match : niveau par défaut */

void cs_log_set_file_level(const char* file_level);

/* Préfixe imprimé et retour 1 si accepté ; sinon 0 */

int cs_log_print_prefix(enum cs_log_level level, const char* fname, int line);

extern enum cs_log_level cs_log_level;

#if CS_ENABLE_STDIO

/* NULL : logs vers stderr */

void cs_log_set_file(FILE* file);

/* Ajoute LF puis flush stream */

void cs_log_printf(const char* fmt, ...) PRINTF_LIKE(1, 2);

#if CS_ENABLE_STDIO

#define LOG(l, x)                                        \
    do {                                                 \
        if(cs_log_print_prefix(l, __FILE__, __LINE__)) { \
            cs_log_printf x;                             \
        }                                                \
    } while(0)

#else

#define LOG(l, x) ((void)l)

#endif

#ifndef CS_NDEBUG

#define DBG(x) LOG(LL_VERBOSE_DEBUG, x)

#else

#define DBG(x)

#endif

#else

#define LOG(l, x)
#define DBG(x)

#endif

#ifdef __cplusplus
}
#endif

#endif
