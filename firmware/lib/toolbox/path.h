#pragma once
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

/* String filename destination initialisée requise */

void path_extract_filename_no_ext(const char* path, FuriString* filename);

/* Destination initialisée ; trim_ext enlève extension */

void path_extract_filename(FuriString* path, FuriString* filename, bool trim_ext);

/* ext_len_max borne extension */

void path_extract_extension(FuriString* path, char* ext, size_t ext_len_max);

/* String destination initialisée requise */

void path_extract_basename(const char* path, FuriString* basename);

/* String destination initialisée requise */

void path_extract_dirname(const char* path, FuriString* dirname);

/* Ajout composant avec séparateur de chemin */

void path_append(FuriString* path, const char* suffix);

/* Destination initialisée ; séparateur ajouté */

void path_concat(const char* path, const char* suffix, FuriString* out_path);

bool path_contains_only_ascii(const char* path);

#ifdef __cplusplus
}
#endif
