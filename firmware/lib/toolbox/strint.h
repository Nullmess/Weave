

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Erreur conversion string vers entier */
typedef enum {
    StrintParseNoError, //!< Conversion performed successfully
    StrintParseSignError, /* Signes multiples ou signe négatif pour unsigned : erreur */
    StrintParseAbsentError, /* Aucun chiffre valide après espaces/signe/préfixe */
    StrintParseOverflowError, /* Résultat hors limites du type demandé */
} StrintParseError;

/** See `strint_to_uint32` */
StrintParseError strint_to_uint64(const char* str, char** end, uint64_t* out, uint8_t base);

/** See `strint_to_uint32` */
StrintParseError strint_to_int64(const char* str, char** end, int64_t* out, uint8_t base);

/* Espaces initiaux admis ; chiffres selon base 0..9/A..Z */
/* base=0 : 0x hex, 0b binaire, 0 octal, sinon décimal */
/* end pointe après préfixes/chiffres ; caractères suivants tolérés */
/* Erreur : end/out inchangés */

StrintParseError strint_to_uint32(const char* str, char** end, uint32_t* out, uint8_t base);

/** See `strint_to_uint32` */
StrintParseError strint_to_int32(const char* str, char** end, int32_t* out, uint8_t base);

/** See `strint_to_uint32` */
StrintParseError strint_to_uint16(const char* str, char** end, uint16_t* out, uint8_t base);

/** See `strint_to_uint32` */
StrintParseError strint_to_int16(const char* str, char** end, int16_t* out, uint8_t base);

#ifdef __cplusplus
}
#endif
