#pragma once

#include <stdbool.h>
#include <storage/storage.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UPDATE_OPERATION_ROOT_DIR_PACKAGE_MAGIC 0
#define UPDATE_OPERATION_MAX_MANIFEST_PATH_LEN  255u
#define UPDATE_OPERATION_MIN_MANIFEST_VERSION   2

/* Nom UPDATE_MANIFEST_DEFAULT_NAME requis ; output dir peut être vide */

bool update_operation_get_package_dir_name(const char* full_path, FuriString* out_manifest_dir);

/* Modification enum : synchroniser assets/protobuf/system.proto */
typedef enum {
    UpdatePrepareResultOK,
    UpdatePrepareResultManifestPathInvalid,
    UpdatePrepareResultManifestFolderNotFound,
    UpdatePrepareResultManifestInvalid,
    UpdatePrepareResultStageMissing,
    UpdatePrepareResultStageIntegrityError,
    UpdatePrepareResultManifestPointerCreateError,
    UpdatePrepareResultManifestPointerCheckError,
    UpdatePrepareResultTargetMismatch,
    UpdatePrepareResultOutdatedManifestVersion,
    UpdatePrepareResultIntFull,
    UpdatePrepareResultUnspecifiedError,
} UpdatePrepareResult;

const char* update_operation_describe_preparation_result(const UpdatePrepareResult value);

/* Valide prochain stage et prépare registres pour update après restart */

UpdatePrepareResult update_operation_prepare(const char* manifest_file_path);

/* String out_path initialisée requise */

bool update_operation_get_current_package_manifest_path(Storage* storage, FuriString* out_path);

/* Étape update en attente après reset */

bool update_operation_is_armed(void);

/* 
 * Cancels pending update operation
 */
void update_operation_disarm(void);

#ifdef __cplusplus
}
#endif
