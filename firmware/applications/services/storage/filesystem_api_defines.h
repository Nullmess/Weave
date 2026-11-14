#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Access mode flags */
typedef enum {
    FSAM_READ = (1 << 0), /* Accès lecture */
    FSAM_WRITE = (1 << 1), /* Accès écriture */
    FSAM_READ_WRITE = FSAM_READ | FSAM_WRITE, /* Accès lecture/écriture */
} FS_AccessMode;

/** Open mode flags */
typedef enum {
    FSOM_OPEN_EXISTING = 1, /**< Open file, fail if file doesn't exist */
    FSOM_OPEN_ALWAYS = 2, /* Créer fichier si absent */
    FSOM_OPEN_APPEND = 4, /* Créer si absent, curseur initial à EOF */
    FSOM_CREATE_NEW = 8, /* Création échoue si fichier existant */
    FSOM_CREATE_ALWAYS = 16, /* Fichier existant tronqué à zéro */
} FS_OpenMode;

/** API errors enumeration */
typedef enum {
    FSE_OK,
    FSE_NOT_READY, /* FS indisponible */
    FSE_EXIST, /**< File/Dir already exist */
    FSE_NOT_EXIST, /* Fichier/dossier absent */
    FSE_INVALID_PARAMETER, /**< Invalid API parameter */
    FSE_DENIED, /**< Access denied */
    FSE_INVALID_NAME, /**< Invalid name/path */
    FSE_INTERNAL, /**< Internal error */
    FSE_NOT_IMPLEMENTED, /* Fonction non implémentée */
    FSE_ALREADY_OPEN, /**< File/Dir already opened */
} FS_Error;

/** FileInfo flags */
typedef enum {
    FSF_DIRECTORY = (1 << 0), /**< Directory */
} FS_Flags;

typedef struct File File;

typedef struct {
    uint8_t flags; /* Flags issus de FS_Flags */
    uint64_t size; /**< file size */
} FileInfo;

const char* filesystem_api_error_get_desc(FS_Error error_id);

bool file_info_is_dir(const FileInfo* file_info);

#ifdef __cplusplus
}
#endif
