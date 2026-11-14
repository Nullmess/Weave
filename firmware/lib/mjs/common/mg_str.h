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

#ifndef CS_COMMON_MG_STR_H_
#define CS_COMMON_MG_STR_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct mg_str {
  const char *p; /* Pointer de bloc mémoire */
  size_t len;
};

/* NULL admis : {NULL, 0} */

struct mg_str mg_mk_str(const char *s);

struct mg_str mg_mk_str_n(const char *s, size_t len);

#define MG_MK_STR(str_literal) \
  { str_literal, sizeof(str_literal) - 1 }
#define MG_MK_STR_N(str_literal, len) \
  { str_literal, len }
#define MG_NULL_STR \
  { NULL, 0 }

int mg_vcmp(const struct mg_str *str2, const char *str1);

int mg_vcasecmp(const struct mg_str *str2, const char *str1);

/* Copie allouée sur heap */
struct mg_str mg_strdup(const struct mg_str s);

/* Copie heap avec NUL, exclu de len */

struct mg_str mg_strdup_nul(const struct mg_str s);

const char *mg_strchr(const struct mg_str s, int c);

/* Retour identique à strcmp */

int mg_strcmp(const struct mg_str str1, const struct mg_str str2);

int mg_strncmp(const struct mg_str str1, const struct mg_str str2, size_t n);

/* Comparaison sans casse ; retour identique à strcmp */

int mg_strcasecmp(const struct mg_str str1, const struct mg_str str2);

/* String supposée allouée sur heap */

void mg_strfree(struct mg_str *s);

const char *mg_strstr(const struct mg_str haystack, const struct mg_str needle);

struct mg_str mg_strstrip(struct mg_str s);

int mg_str_starts_with(struct mg_str s, struct mg_str prefix);

#ifdef __cplusplus
}
#endif

#endif
