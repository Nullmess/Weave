#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Premier entier extrait ; arguments restants conservés */

bool args_read_int_and_trim(FuriString* args, int* value);

/* Premier float extrait ; arguments restants conservés */

bool args_read_float_and_trim(FuriString* args, float* value);

bool args_read_string_and_trim(FuriString* args, FuriString* word);

/* Non quoté : fallback args_read_string_and_trim ; quotes retirées */

bool args_read_probably_quoted_string_and_trim(FuriString* args, FuriString* word);

/* Hex ASCII vers bytes ; false si valeurs insuffisantes/invalides */

bool args_read_hex_bytes(FuriString* args, uint8_t* bytes, size_t bytes_count);

/* Durée convertie en ms ; unités ms/s/m/h ; default_unit NULL=ms */

bool args_read_duration(FuriString* args, uint32_t* value, const char* default_unit);

/************************************ HELPERS ***************************************/

size_t args_get_first_word_length(FuriString* args);

size_t args_length(FuriString* args);

/* ASCII hex vers octet ; bool indique réussite */

bool args_char_to_hex(char hi_nibble, char low_nibble, uint8_t* byte);

#ifdef __cplusplus
}
#endif
