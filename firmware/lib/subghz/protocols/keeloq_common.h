#pragma once

#include "base.h"

#include <furi.h>

/*
 * Keeloq
 * https://ru.wikipedia.org/wiki/KeeLoq
 * https://phreakerclub.com/forum/showthread.php?t=1094
 *
 */
#define KEELOQ_NLF 0x3A5C742E

/*
 * KeeLoq learning types
 * https://phreakerclub.com/forum/showthread.php?t=67
 */
#define KEELOQ_LEARNING_UNKNOWN             0u
#define KEELOQ_LEARNING_SIMPLE              1u
#define KEELOQ_LEARNING_NORMAL              2u
#define KEELOQ_LEARNING_SECURE              3u
#define KEELOQ_LEARNING_MAGIC_XOR_TYPE_1    4u
#define KEELOQ_LEARNING_MAGIC_SERIAL_TYPE_1 5u
#define KEELOQ_LEARNING_MAGIC_SERIAL_TYPE_2 6u
#define KEELOQ_LEARNING_MAGIC_SERIAL_TYPE_3 7u

/* Encrypt Simple Learning : B 4 bits, serial&0x3FF 10 bits, compteur 16 bits */

uint32_t subghz_protocol_keeloq_common_encrypt(const uint32_t data, const uint64_t key);

/* Decrypt Simple Learning : B 4 bits, serial&0x3FF 10 bits, compteur 16 bits */

uint32_t subghz_protocol_keeloq_common_decrypt(const uint32_t data, const uint64_t key);

/* Normal Learning : serial28, key64 ; résultat key64 */

uint64_t subghz_protocol_keeloq_common_normal_learning(uint32_t data, const uint64_t key);

/* Secure Learning : serial28, seed32, key64 ; résultat key64 */

uint64_t
    subghz_protocol_keeloq_common_secure_learning(uint32_t data, uint32_t seed, const uint64_t key);

/* Magic XOR Learning : serial28, XOR64 ; résultat key64 */

uint64_t subghz_protocol_keeloq_common_magic_xor_type1_learning(uint32_t data, uint64_t xor);

/* Magic serial Learning : serial28, man64 ; résultat key64 */

uint64_t subghz_protocol_keeloq_common_magic_serial_type1_learning(uint32_t data, uint64_t man);

/* Magic serial type2 : bouton+serial32, man64 ; résultat key64 */

uint64_t subghz_protocol_keeloq_common_magic_serial_type2_learning(uint32_t data, uint64_t man);

/* Magic serial type3 : bouton+serial32, man64 ; résultat key64 */

uint64_t subghz_protocol_keeloq_common_magic_serial_type3_learning(uint32_t data, uint64_t man);
