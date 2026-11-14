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

/*
 * Mbufs are mutable/growing memory buffers, like C++ strings.
 * Mbuf can append data to the end of a buffer or insert data into arbitrary
 * position in the middle of a buffer. The buffer grows automatically when
 * needed.
 */

#ifndef CS_COMMON_MBUF_H_
#define CS_COMMON_MBUF_H_

#include <stdlib.h>
#include "platform.h"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef MBUF_SIZE_MULTIPLIER
#define MBUF_SIZE_MULTIPLIER 1.5
#endif

#ifndef MBUF_SIZE_MAX_HEADROOM
#ifdef BUFSIZ
#define MBUF_SIZE_MAX_HEADROOM BUFSIZ
#else
#define MBUF_SIZE_MAX_HEADROOM 1024
#endif
#endif

struct mbuf {
  char *buf;
  size_t len;  /* Données entre offsets 0 et len */
  size_t size; /* Capacité allouée toujours >=len */
};

void mbuf_init(struct mbuf *, size_t initial_capacity);

void mbuf_free(struct mbuf *);

/* Octets ajoutés ; 0 si mémoire insuffisante */

size_t mbuf_append(struct mbuf *, const void *data, size_t data_size);

/* Data heap libérée même si échec ; 0 si mémoire insuffisante */

size_t mbuf_append_and_free(struct mbuf *, void *data, size_t data_size);

/* Insertion décale données et agrandit buffer si nécessaire */

size_t mbuf_insert(struct mbuf *, size_t, const void *, size_t);

void mbuf_remove(struct mbuf *, size_t data_size);

/* Resize ignoré si new_size<len */

void mbuf_resize(struct mbuf *, size_t new_size);

/* Transfert état entre mbuf */
void mbuf_move(struct mbuf *from, struct mbuf *to);

void mbuf_clear(struct mbuf *);

void mbuf_trim(struct mbuf *);

#if defined(__cplusplus)
}
#endif

#endif
