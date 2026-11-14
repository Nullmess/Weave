#pragma once
#include <furi.h>
#include "filesystem_api_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/** File type */
typedef enum {
    FileTypeClosed, /**< Closed file */
    FileTypeOpenDir, /**< Open dir */
    FileTypeOpenFile, /**< Open file */
} FileType;

struct File {
    uint32_t file_id;
    FileType type;
    FS_Error error_id; /* Erreur API standard depuis FS_Error */
    int32_t internal_error_id; /**< Internal API error value */
    void* storage;
};

/* Lecture bornée à taille buffer ; retours : octets réels */
/* Seek depuis début ou curseur selon from_start */
/* Truncate à position courante ; sync flush cache */

typedef struct {
    bool (*const open)(
        void* context,
        File* file,
        const char* path,
        FS_AccessMode access_mode,
        FS_OpenMode open_mode);
    bool (*const close)(void* context, File* file);
    uint16_t (*read)(void* context, File* file, void* buff, uint16_t bytes_to_read);
    uint16_t (*write)(void* context, File* file, const void* buff, uint16_t bytes_to_write);
    bool (*const seek)(void* context, File* file, uint32_t offset, bool from_start);
    uint64_t (*tell)(void* context, File* file);
    bool (*const truncate)(void* context, File* file);
    uint64_t (*size)(void* context, File* file);
    bool (*const sync)(void* context, File* file);
    bool (*const eof)(void* context, File* file);
} FS_File_Api;

/* fileinfo/name NULL admis */
/* Fin répertoire : false avec FSE_NOT_EXIST */

typedef struct {
    bool (*const open)(void* context, File* file, const char* path);
    bool (*const close)(void* context, File* file);
    bool (*const read)(
        void* context,
        File* file,
        FileInfo* fileinfo,
        char* name,
        uint16_t name_length);
    bool (*const rewind)(void* context, File* file);
} FS_Dir_Api;

/* Remove : dossier vide, objet fermé, sans FSF_READ_ONLY */
/* Equivalent paths selon filesystem ; truncate compare longueur path1 */

typedef struct {
    FS_Error (*const stat)(void* context, const char* path, FileInfo* fileinfo);
    FS_Error (*const remove)(void* context, const char* path);
    FS_Error (*const mkdir)(void* context, const char* path);
    FS_Error (*const fs_info)(
        void* context,
        const char* fs_path,
        uint64_t* total_space,
        uint64_t* free_space);
    bool (*const equivalent_path)(const char* path1, const char* path2);
} FS_Common_Api;

/** Full filesystem api structure */
typedef struct {
    const FS_File_Api file;
    const FS_Dir_Api dir;
    const FS_Common_Api common;
} FS_Api;

#ifdef __cplusplus
}
#endif
