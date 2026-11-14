#pragma once

#include <stdint.h>
#include "filesystem_api_defines.h"
#include "storage_sd_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_INT_PATH_PREFIX        "/int"
#define STORAGE_EXT_PATH_PREFIX        "/ext"
#define STORAGE_ANY_PATH_PREFIX        "/any"
#define STORAGE_APP_DATA_PATH_PREFIX   "/data"
#define STORAGE_APP_ASSETS_PATH_PREFIX "/assets"

#define INT_PATH(path)        STORAGE_INT_PATH_PREFIX "/" path
#define EXT_PATH(path)        STORAGE_EXT_PATH_PREFIX "/" path
#define ANY_PATH(path)        STORAGE_ANY_PATH_PREFIX "/" path
#define APP_DATA_PATH(path)   STORAGE_APP_DATA_PATH_PREFIX "/" path
#define APP_ASSETS_PATH(path) STORAGE_APP_ASSETS_PATH_PREFIX "/" path

#define RECORD_STORAGE "storage"

typedef struct Storage Storage;

File* storage_file_alloc(Storage* storage);

/* Fichier ouvert automatiquement fermé lors de la libération */
void storage_file_free(File* file);

typedef enum {
    StorageEventTypeCardMount,
    StorageEventTypeCardUnmount,
    StorageEventTypeCardMountError,
    StorageEventTypeFileClose,
    StorageEventTypeDirClose,
} StorageEventType;

typedef struct {
    StorageEventType type;
} StorageEvent;

/* PubSub émet des messages StorageEvent */
FuriPubSub* storage_get_pubsub(Storage* storage);

/* Close obligatoire même après échec d’open */
bool storage_file_open(
    File* file,
    const char* path,
    FS_AccessMode access_mode,
    FS_OpenMode open_mode);

bool storage_file_close(File* file);

bool storage_file_is_open(File* file);

bool storage_file_is_dir(File* file);

/* Lecture bornée par buffer ; retour bytes réellement lus */
size_t storage_file_read(File* file, void* buff, size_t bytes_to_read);

/* Écriture bornée par buffer ; retour bytes réellement écrits */
size_t storage_file_write(File* file, const void* buff, size_t bytes_to_write);

/* Offset depuis début si from_start, sinon depuis position courante */
bool storage_file_seek(File* file, uint32_t offset, bool from_start);

uint64_t storage_file_tell(File* file);

/* Taille tronquée à la position courante */
bool storage_file_truncate(File* file);

/* Taille fichier en bytes */
uint64_t storage_file_size(File* file);

bool storage_file_sync(File* file);

bool storage_file_eof(File* file);

bool storage_file_exists(Storage* storage, const char* path);

/* Fichiers ouverts requis ; copie depuis leurs positions courantes */
bool storage_file_copy_to_file(File* source, File* destination, size_t size);

/* Dir_close obligatoire même après échec d’open */
bool storage_dir_open(File* file, const char* path);

bool storage_dir_close(File* file);

/* Fin répertoire : false et FSE_NOT_EXIST */
/* Fileinfo/name peuvent être NULL ; name_length en bytes */
bool storage_dir_read(File* file, FileInfo* fileinfo, char* name, uint16_t name_length);

bool storage_dir_rewind(File* file);

bool storage_dir_exists(Storage* storage, const char* path);

/* Timestamp dernier accès UNIX ; FSE_OK si succès */
FS_Error storage_common_timestamp(Storage* storage, const char* path, uint32_t* timestamp);

/* Fileinfo peut être NULL ; FSE_OK si succès */
FS_Error storage_common_stat(Storage* storage, const char* path, FileInfo* fileinfo);

/* Objet fermé requis ; répertoire vide obligatoire */
FS_Error storage_common_remove(Storage* storage, const char* path);

/* Objet fermé requis ; overwrite de la destination */
/* Fichier vers lui-même : succès sans effet */
/* Répertoire vers lui-même ou son sous-répertoire : échec */
FS_Error storage_common_rename(Storage* storage, const char* old_path, const char* new_path);

/* Fichier fermé requis */
FS_Error storage_common_copy(Storage* storage, const char* old_path, const char* new_path);

/* Merge répertoires avec renommage des conflits */
FS_Error storage_common_merge(Storage* storage, const char* old_path, const char* new_path);

FS_Error storage_common_mkdir(Storage* storage, const char* path);

/* Espaces total/libre en bytes */
FS_Error storage_common_fs_info(
    Storage* storage,
    const char* fs_path,
    uint64_t* total_space,
    uint64_t* free_space);

/* Alias résolus ; répertoires spéciaux absents créés */
void storage_common_resolve_path_and_ensure_app_directory(Storage* storage, FuriString* path);

/* Migration avec renommage conflits ; source supprimée si succès */
FS_Error storage_common_migrate(Storage* storage, const char* source, const char* dest);

bool storage_common_exists(Storage* storage, const char* path);

/* Alias résolus ; /int sensible à la casse, /ext insensible */
bool storage_common_equivalent_path(Storage* storage, const char* path1, const char* path2);

/* True si child sous-path ou équivalent à parent */
bool storage_common_is_subdir(Storage* storage, const char* parent, const char* child);

/* String statique terminée par NUL */
const char* storage_error_get_desc(FS_Error error_id);

/* Fichier non NULL et non fermé requis pour lire erreur */
FS_Error storage_file_get_error(File* file);

/* Fichier non NULL et non fermé requis pour lire erreur interne */
int32_t storage_file_get_internal_error(File* file);

/* Fichier non NULL/non fermé ; retour string statique terminée par NUL */
const char* storage_file_get_error_desc(File* file);

FS_Error storage_sd_format(Storage* storage);

/* FSE_NOT_READY si non montée ; FSE_DENIED si fichiers ouverts */
FS_Error storage_sd_unmount(Storage* storage);

FS_Error storage_sd_mount(Storage* storage);

FS_Error storage_sd_info(Storage* storage, SDInfo* info);

FS_Error storage_sd_status(Storage* storage);

typedef void (*StorageNameConverter)(FuriString*);

/* Backup interne vers archive TAR */
FS_Error storage_int_backup(Storage* storage, const char* dstname);

/* Restore archive TAR ; converter peut être NULL */
FS_Error
    storage_int_restore(Storage* storage, const char* dstname, StorageNameConverter converter);

/* Objet fermé, répertoire vide ; true si succès ou déjà absent */
bool storage_simply_remove(Storage* storage, const char* path);

/* Suppression récursive ; true si succès ou déjà absent */
bool storage_simply_remove_recursive(Storage* storage, const char* path);

/* Retour true si créé ou déjà existant */
bool storage_simply_mkdir(Storage* storage, const char* path);

/* Nom libre numéroté ; nom original si max_len dépassé */
void storage_get_next_filename(
    Storage* storage,
    const char* dirname,
    const char* filename,
    const char* fileextension,
    FuriString* nextfilename,
    uint8_t max_len);

#ifdef __cplusplus
}
#endif
