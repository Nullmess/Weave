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

#ifndef CS_COMMON_CS_FILE_H_
#define CS_COMMON_CS_FILE_H_

#include "platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Contenu alloué, NUL final ; free requis par appelant */
/* size hors NUL ; NULL si erreur */

char *cs_read_file(const char *path, size_t *size);

#ifdef CS_MMAP
/* Plateformes mmap uniquement ; taille renvoyée via size */

char *cs_mmap_file(const char *path, size_t *size);
#endif

#ifdef __cplusplus
}
#endif

#endif
