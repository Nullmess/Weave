#pragma once
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DirWalk DirWalk;

typedef enum {
    DirWalkOK, /**< OK */
    DirWalkError,
    DirWalkLast, /**< Last element */
} DirWalkResult;

typedef bool (*DirWalkFilterCb)(const char* name, FileInfo* fileinfo, void* ctx);

DirWalk* dir_walk_alloc(Storage* storage);

void dir_walk_free(DirWalk* dir_walk);

/* Récursif par défaut */

void dir_walk_set_recursive(DirWalk* dir_walk, bool recursive);

/* Filter callback true si données valides */

void dir_walk_set_filter_cb(DirWalk* dir_walk, DirWalkFilterCb cb, void* context);

bool dir_walk_open(DirWalk* dir_walk, const char* path);

FS_Error dir_walk_get_error(DirWalk* dir_walk);

DirWalkResult dir_walk_read(DirWalk* dir_walk, FuriString* return_path, FileInfo* fileinfo);

/**
 * Close directory
 * @param dir_walk 
 */
void dir_walk_close(DirWalk* dir_walk);

#ifdef __cplusplus
}
#endif
