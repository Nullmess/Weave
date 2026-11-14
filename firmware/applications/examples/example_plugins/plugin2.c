
#include "plugin_interface.h"

#include <flipper_application/flipper_application.h>

static int example_plugin2_method1(void) {
    return 1337;
}

static int example_plugin2_method2(int arg1, int arg2) {
    return arg1 - arg2;
}

/* Implémentation interface app/plugin */
static const ExamplePlugin example_plugin2 = {
    .name = "Demo App Plugin 2",
    .method1 = &example_plugin2_method1,
    .method2 = &example_plugin2_method2,
};

/* Descriptor conforme au contrat plugin */
static const FlipperAppPluginDescriptor example_plugin2_descriptor = {
    .appid = PLUGIN_APP_ID,
    .ep_api_version = PLUGIN_API_VERSION,
    .entry_point = &example_plugin2,
};

/* Entrée plugin : pointeur vers descriptor const */
const FlipperAppPluginDescriptor* example_plugin2_ep(void) {
    return &example_plugin2_descriptor;
}
