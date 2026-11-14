#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TarArchive TarArchive;

typedef struct Storage Storage;

/** Tar archive open mode 
 */
typedef enum {
    TarOpenModeRead = 'r',
    TarOpenModeWrite = 'w',
    /* Tar compressé heatshrink en lecture seule */
    TarOpenModeReadHeatshrink = 'h',
} TarOpenMode;

/* Mode archive déduit extension fichier */

TarOpenMode tar_archive_get_mode_for_path(const char* path);

TarArchive* tar_archive_alloc(Storage* storage);

bool tar_archive_open(TarArchive* archive, const char* path, TarOpenMode mode);

/** Tar archive destructor
 *
 * @param archive Tar archive object
 */
void tar_archive_free(TarArchive* archive);

typedef void (*TarArchiveNameConverter)(FuriString*);

/* Archive déjà ouverte requise */

/* Archive ouverte en lecture requise */

bool tar_archive_unpack_to(
    TarArchive* archive,
    const char* destination,
    TarArchiveNameConverter converter);

/* Archive ouverte en écriture requise */

bool tar_archive_add_file(
    TarArchive* archive,
    const char* fs_file_path,
    const char* archive_fname,
    const int32_t file_size);

/* Archive ouverte en écriture requise */

bool tar_archive_add_dir(TarArchive* archive, const char* fs_full_path, const char* path_prefix);

/* Nombre entrées ; -1 si erreur */

int32_t tar_archive_get_entries_count(TarArchive* archive);

/* Archive ouverte en lecture requise */

bool tar_archive_get_read_progress(TarArchive* archive, int32_t* processed, int32_t* total);

/* Archive ouverte en lecture requise */

bool tar_archive_unpack_file(
    TarArchive* archive,
    const char* archive_fname,
    const char* destination);

/* Callback entrée : true traite, false ignore */

typedef bool (*tar_unpack_file_cb)(const char* name, bool is_directory, void* context);

/* Callback par entrée pendant extraction */

void tar_archive_set_file_callback(TarArchive* archive, tar_unpack_file_cb callback, void* context);

/* Low-level API */

/* Archive ouverte en écriture requise */

bool tar_archive_dir_add_element(TarArchive* archive, const char* dirpath);

/* Archive ouverte en écriture requise */

bool tar_archive_file_add_header(TarArchive* archive, const char* path, const int32_t data_len);

/* Archive ouverte en écriture requise */

bool tar_archive_file_add_data_block(
    TarArchive* archive,
    const uint8_t* data_block,
    const int32_t block_len);

/* Archive ouverte en écriture requise */

bool tar_archive_file_finalize(TarArchive* archive);

/* Archive ouverte en écriture requise */

bool tar_archive_store_data(
    TarArchive* archive,
    const char* path,
    const uint8_t* data,
    const int32_t data_len);

/* Archive ouverte en écriture requise */

bool tar_archive_finalize(TarArchive* archive);

#ifdef __cplusplus
}
#endif
