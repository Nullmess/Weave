/* Stream buffer : un writer et un reader uniques */
#pragma once

#include "base.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriStreamBuffer FuriStreamBuffer;

/* Un writer et un reader uniques */
/* Trigger : bytes requis pour réveiller le reader */
FuriStreamBuffer* furi_stream_buffer_alloc(size_t size, size_t trigger_level);

void furi_stream_buffer_free(FuriStreamBuffer* stream_buffer);

/* Trigger : seuil de bytes pour réveiller le reader */
/* Retour true si seuil <= capacité */
bool furi_stream_set_trigger_level(FuriStreamBuffer* stream_buffer, size_t trigger_level);

/* Trigger : seuil de bytes pour réveiller le reader */
size_t furi_stream_get_trigger_level(FuriStreamBuffer* stream_buffer);

/* Copie des données ; retour nombre de bytes écrits */
/* Timeout 0 immédiat, FuriWaitForever infini ; ignoré en ISR */
/* ISR : réveil du reader en attente */
size_t furi_stream_buffer_send(
    FuriStreamBuffer* stream_buffer,
    const void* data,
    size_t length,
    uint32_t timeout);

/* Retour nombre de bytes lus */
/* Timeout 0 immédiat, FuriWaitForever infini ; ignoré en ISR */
/* ISR : réveil du writer en attente */
size_t furi_stream_buffer_receive(
    FuriStreamBuffer* stream_buffer,
    void* data,
    size_t length,
    uint32_t timeout);

/* Nombre de bytes disponibles en lecture */
size_t furi_stream_buffer_bytes_available(FuriStreamBuffer* stream_buffer);

/* Capacité libre en bytes */
size_t furi_stream_buffer_spaces_available(FuriStreamBuffer* stream_buffer);

bool furi_stream_buffer_is_full(FuriStreamBuffer* stream_buffer);

bool furi_stream_buffer_is_empty(FuriStreamBuffer* stream_buffer);

/* Reset refusé si reader/writer en attente */
/* FuriStatusOk si reset ; FuriStatusError sinon */
FuriStatus furi_stream_buffer_reset(FuriStreamBuffer* stream_buffer);

#ifdef __cplusplus
}
#endif
