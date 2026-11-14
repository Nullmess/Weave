#include "../archive_i.h"
#include "../helpers/archive_browser.h"
#include <storage/storage.h>

/* Compatibility fallback: there is no storage chooser. File Manager opens
 * directly in /ext even when entered via an old saved scene reference. */
void archive_scene_root_on_enter(void* context) {
    ArchiveApp* app = context;
    archive_open_storage(app->browser, STORAGE_EXT_PATH_PREFIX);
    scene_manager_next_scene(app->scene_manager, ArchiveAppSceneBrowser);
}

bool archive_scene_root_on_event(void* context, SceneManagerEvent event) {
    ArchiveApp* app = context;
    if(event.type == SceneManagerEventTypeBack) {
        view_dispatcher_stop(app->view_dispatcher);
        return true;
    }
    return false;
}

void archive_scene_root_on_exit(void* context) {
    UNUSED(context);
}
