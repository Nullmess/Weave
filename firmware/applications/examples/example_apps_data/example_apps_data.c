#include <furi.h>
#include <storage/storage.h>

#define TAG "ExampleAppsData"

int32_t example_apps_data_main(void* p) {

    UNUSED(p);

    Storage* storage = furi_record_open(RECORD_STORAGE);

    File* file = storage_file_alloc(storage);

    /* Données application : /ext/apps_data/<app_name> */
    /* Répertoires absents créés automatiquement */

    if(!storage_file_open(file, APP_DATA_PATH("test.txt"), FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        FURI_LOG_E(TAG, "Failed to open file");
    }
    if(!storage_file_write(file, "Hello World!", strlen("Hello World!"))) {
        FURI_LOG_E(TAG, "Failed to write to file");
    }
    storage_file_close(file);

    storage_file_free(file);

    furi_record_close(RECORD_STORAGE);

    return 0;
}
