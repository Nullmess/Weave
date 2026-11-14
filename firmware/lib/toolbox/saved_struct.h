/**
 * @file saved_struct.h
 * @brief SavedStruct - data serialization/de-serialization
 * 
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool saved_struct_load(const char* path, void* data, size_t size, uint8_t magic, uint8_t version);

bool saved_struct_save(
    const char* path,
    const void* data,
    size_t size,
    uint8_t magic,
    uint8_t version);

/* magic/version/payload_size NULL admis si résultat inutile */

bool saved_struct_get_metadata(
    const char* path,
    uint8_t* magic,
    uint8_t* version,
    size_t* payload_size);

#ifdef __cplusplus
}
#endif
