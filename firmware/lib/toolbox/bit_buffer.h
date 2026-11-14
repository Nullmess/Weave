#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BitBuffer BitBuffer;

/* Capacité maximale en bytes */
BitBuffer* bit_buffer_alloc(size_t capacity_bytes);

void bit_buffer_free(BitBuffer* buf);

void bit_buffer_reset(BitBuffer* buf);

/* Capacité destination >=taille source ; données remplacées */
void bit_buffer_copy(BitBuffer* buf, const BitBuffer* other);

/* Copie depuis start_index ; capacité destination suffisante requise */
void bit_buffer_copy_right(BitBuffer* buf, const BitBuffer* other, size_t start_index);

/* Copie jusqu’à end_index ; capacité destination suffisante requise */
void bit_buffer_copy_left(BitBuffer* buf, const BitBuffer* other, size_t end_index);

/* Données remplacées ; capacité destination >=size_bytes */
void bit_buffer_copy_bytes(BitBuffer* buf, const uint8_t* data, size_t size_bytes);

/* Données remplacées ; capacité suffisante pour size_bits requise */
void bit_buffer_copy_bits(BitBuffer* buf, const uint8_t* data, size_t size_bits);

/* Capacité destination suffisante pour size_bits requise */
/* Parité bit-packed : DDDDDDDD PDDDDDDD DPDDDDDD DDP... */
void bit_buffer_copy_bytes_with_parity(BitBuffer* buf, const uint8_t* data, size_t size_bits);

/* Destination allouée ; capacité >=taille source ; limite en bytes */
void bit_buffer_write_bytes(const BitBuffer* buf, void* dest, size_t size_bytes);

/* Destination allouée ; capacité >=données+parité */
/* Parité bit-packed après chaque byte ; retour bits_written en bits */
void bit_buffer_write_bytes_with_parity(
    const BitBuffer* buf,
    void* dest,
    size_t size_bytes,
    size_t* bits_written);

/* Destination allouée ; capacité >=size_bytes ; copie depuis start_index */
void bit_buffer_write_bytes_mid(
    const BitBuffer* buf,
    void* dest,
    size_t start_index,
    size_t size_bytes);

/* True si taille bits non divisible par 8 */
bool bit_buffer_has_partial_byte(const BitBuffer* buf);

bool bit_buffer_starts_with_byte(const BitBuffer* buf, uint8_t byte);

/* Capacité maximale en bytes */
size_t bit_buffer_get_capacity_bytes(const BitBuffer* buf);

/* Taille en bits, parfois non divisible par 8 */
size_t bit_buffer_get_size(const BitBuffer* buf);

/* Taille en bytes, byte partiel inclus */
size_t bit_buffer_get_size_bytes(const BitBuffer* buf);

/* Index strictement inférieur à la taille en bytes */
uint8_t bit_buffer_get_byte(const BitBuffer* buf, size_t index);

/* Index valide en bits ; byte reconstruit sur deux bytes si décalé */
uint8_t bit_buffer_get_byte_from_bit(const BitBuffer* buf, size_t index_bits);

const uint8_t* bit_buffer_get_data(const BitBuffer* buf);

const uint8_t* bit_buffer_get_parity(const BitBuffer* buf);

/* Index strictement inférieur à la taille en bytes */
void bit_buffer_set_byte(BitBuffer* buf, size_t index, uint8_t byte);

/* Index strictement inférieur à la taille en bytes */
void bit_buffer_set_byte_with_parity(BitBuffer* buff, size_t index, uint8_t byte, bool parity);

/* Resize risqué ; taille en bits */
void bit_buffer_set_size(BitBuffer* buf, size_t new_size);

/* Resize risqué ; taille en bytes */
void bit_buffer_set_size_bytes(BitBuffer* buf, size_t new_size_bytes);

/* Capacité destination >=taille finale après ajout */
void bit_buffer_append(BitBuffer* buf, const BitBuffer* other);

/* Copie depuis start_index ; capacité finale suffisante requise */
void bit_buffer_append_right(BitBuffer* buf, const BitBuffer* other, size_t start_index);

/* Capacité destination >=taille actuelle +1 byte */
void bit_buffer_append_byte(BitBuffer* buf, uint8_t byte);

/* Capacité destination >=taille actuelle +size_bytes */
void bit_buffer_append_bytes(BitBuffer* buf, const uint8_t* data, size_t size_bytes);

/* Capacité suffisante pour le bit supplémentaire requise */
void bit_buffer_append_bit(BitBuffer* buf, bool bit);

#ifdef __cplusplus
}
#endif
