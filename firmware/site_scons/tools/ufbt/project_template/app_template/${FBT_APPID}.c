#include <furi.h>

/* Généré par fbt depuis PNG du dossier images */
#include <@FBT_APPID@_icons.h>

int32_t @FBT_APPID@_app(void* p) {
    UNUSED(p);
    FURI_LOG_I("TEST", "Hello world");
    FURI_LOG_I("TEST", "I'm @FBT_APPID@!");

    return 0;
}
