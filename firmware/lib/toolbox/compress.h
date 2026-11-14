/**
 * @file compress.h
 * LZSS based compression HAL API
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Compress Icon control structure */
typedef struct CompressIcon CompressIcon;

/* decode_buf_size suffisant pour toutes icônes requis */

CompressIcon* compress_icon_alloc(size_t decode_buf_size);

void compress_icon_free(CompressIcon* instance);

/* Pointer valide jusqu’au prochain decode/free */
/* Sans compression : pointer dans icon_data */

void compress_icon_decode(CompressIcon* instance, const uint8_t* icon_data, uint8_t** output);

/** Compress control structure */
typedef struct Compress Compress;

/** Supported compression types */
typedef enum {
    CompressTypeHeatshrink = 0,
} CompressType;

/* Configuration compression heatshrink */
typedef struct {
    uint16_t window_sz2;
    uint16_t lookahead_sz2;
    uint16_t input_buffer_sz;
} CompressConfigHeatshrink;

/* Heatshrink par défaut pour assets image */
extern const CompressConfigHeatshrink compress_config_heatshrink_default;

Compress* compress_alloc(CompressType type, const void* config);

void compress_free(Compress* compress);

/* Header ajouté ; data originale si incompressible */

bool compress_encode(
    Compress* compress,
    uint8_t* data_in,
    size_t data_in_size,
    uint8_t* data_out,
    size_t data_out_size,
    size_t* data_res_size);

/* Header compress_encode attendu */

bool compress_decode(
    Compress* compress,
    uint8_t* data_in,
    size_t data_in_size,
    uint8_t* data_out,
    size_t data_out_size,
    size_t* data_res_size);

/* Callback I/O : octets traités ; 0 fin, négatif erreur */

typedef int32_t (*CompressIoCallback)(void* context, uint8_t* buffer, size_t size);

/* Stream compressé sans header attendu */

bool compress_decode_streamed(
    Compress* compress,
    CompressIoCallback read_cb,
    void* read_context,
    CompressIoCallback write_cb,
    void* write_context);

/** CompressStreamDecoder control structure */
typedef struct CompressStreamDecoder CompressStreamDecoder;

CompressStreamDecoder* compress_stream_decoder_alloc(
    CompressType type,
    const void* config,
    CompressIoCallback read_cb,
    void* read_context);

void compress_stream_decoder_free(CompressStreamDecoder* instance);

bool compress_stream_decoder_read(
    CompressStreamDecoder* instance,
    uint8_t* data_out,
    size_t data_out_size);

/* Seek arrière non supporté */

bool compress_stream_decoder_seek(CompressStreamDecoder* instance, size_t position);

size_t compress_stream_decoder_tell(CompressStreamDecoder* instance);

/* Reset decoder ; appelant repositionne callback read séparément */

bool compress_stream_decoder_rewind(CompressStreamDecoder* instance);

#ifdef __cplusplus
}
#endif
