#pragma once

#include <storage/storage.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ResourceManifestEntryTypeUnknown = 0,
    ResourceManifestEntryTypeVersion,
    ResourceManifestEntryTypeTimestamp,
    ResourceManifestEntryTypeDirectory,
    ResourceManifestEntryTypeFile,
} ResourceManifestEntryType;

typedef struct {
    ResourceManifestEntryType type;
    FuriString* name;
    uint32_t size;
    uint8_t hash[16];
} ResourceManifestEntry;

typedef struct ResourceManifestReader ResourceManifestReader;

ResourceManifestReader* resource_manifest_reader_alloc(Storage* storage);

/**
 * @brief Release resource manifest reader
 * @param resource_manifest allocated object
 */
void resource_manifest_reader_free(ResourceManifestReader* resource_manifest);

/* Initialiser itération ; true si fichier ouvert */

bool resource_manifest_reader_open(ResourceManifestReader* resource_manifest, const char* filename);

bool resource_manifest_rewind(ResourceManifestReader* resource_manifest);

/* NULL si EOF */

ResourceManifestEntry* resource_manifest_reader_next(ResourceManifestReader* resource_manifest);

/* Relecture précédente exige position fin manifest après reader_next */

ResourceManifestEntry*
    resource_manifest_reader_previous(ResourceManifestReader* resource_manifest);

#ifdef __cplusplus
} // extern "C"
#endif
