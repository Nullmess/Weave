#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    KeysDictModeOpenExisting,
    KeysDictModeOpenAlways,
} KeysDictMode;

typedef struct KeysDict KeysDict;

bool keys_dict_check_presence(const char* path);

/* key_size en octets */

KeysDict* keys_dict_alloc(const char* path, KeysDictMode mode, size_t key_size);

/** Close list
 *
 * @param instance  - KeysDict list instance
*/
void keys_dict_free(KeysDict* instance);

size_t keys_dict_get_total_keys(KeysDict* instance);

bool keys_dict_rewind(KeysDict* instance);

/* key_size en octets */

bool keys_dict_is_key_present(KeysDict* instance, const uint8_t* key, size_t key_size);

/* Fin liste : false ; appeler rewind */

bool keys_dict_get_next_key(KeysDict* instance, uint8_t* key, size_t key_size);

/* key_size en octets */

bool keys_dict_add_key(KeysDict* instance, const uint8_t* key, size_t key_size);

/* key_size en octets */

bool keys_dict_delete_key(KeysDict* instance, const uint8_t* key, size_t key_size);

#ifdef __cplusplus
}
#endif
