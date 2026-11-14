/* Plugin firmware/API privée via CompoundApiInterface */

#include "app_api.h"
#include "plugin_interface.h"

#include <flipper_application/flipper_application.h>
#include <furi.h>

static void advanced_plugin2_method1(int arg1) {
    /* Implémentation fournie par l’application hôte */
    app_api_accumulator_mul(arg1);
}

static void advanced_plugin2_method2(void) {
    /* Accumulator stocké dans l’application hôte */
    FURI_LOG_I("TEST", "Plugin 2, accumulator: %lu", app_api_accumulator_get());
}

/* Implémentation interface app/plugin */
static const AdvancedPlugin advanced_plugin2 = {
    .name = "Advanced Plugin 2",
    .method1 = &advanced_plugin2_method1,
    .method2 = &advanced_plugin2_method2,
};

/* Descriptor conforme au contrat plugin */
static const FlipperAppPluginDescriptor advanced_plugin2_descriptor = {
    .appid = PLUGIN_APP_ID,
    .ep_api_version = PLUGIN_API_VERSION,
    .entry_point = &advanced_plugin2,
};

/* Entrée plugin : pointeur vers descriptor const */
const FlipperAppPluginDescriptor* advanced_plugin2_ep(void) {
    return &advanced_plugin2_descriptor;
}
