

#pragma once

#include <furi.h>
#include <toolbox/pipe.h>
#include <lib/flipper_application/flipper_application.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_PLUGIN_API_VERSION 1

typedef enum {
    CliCommandFlagDefault = 0,
    CliCommandFlagParallelSafe = (1 << 0), /* Exécution parallèle avec autres applications autorisée */
    CliCommandFlagInsomniaSafe = (1 << 1), /* Exécution avec insomnia autorisée */
    CliCommandFlagDontAttachStdio = (1 << 2), /* Sans connexion du pipe I/O au stdio du thread */
    CliCommandFlagUseShellThread =
        (1
         << 3), /* Sans thread séparé ; incompatible DontAttachStdio */

    /* Flags internes ; ne pas assigner directement */

    CliCommandFlagExternal = (1 << 4), /* Commande provenant fichier fal */
} CliCommandFlag;

/* Commande sur thread propre ; args après nom, context utilisateur */
/* Pipe attaché à stdio sauf DontAttachStdio */

typedef void (*CliCommandExecuteCallback)(PipeSide* pipe, FuriString* args, void* context);

typedef struct {
    char* name;
    CliCommandExecuteCallback execute_callback;
    CliCommandFlag flags;
    size_t stack_depth;
} CliCommandDescriptor;

typedef struct {
    const char* search_directory; /* Répertoire recherche */
    const char* fal_prefix; /* Préfixe noms fichiers commandes */
    const char* appid; //<! Expected plugin-reported appid
} CliCommandExternalConfig;

/* Pipe stdio requis ; ETX/session close détectés, zéro/un octet consommé */

bool cli_is_pipe_broken_or_is_etx_next_char(PipeSide* side);

/** Print unified cmd usage tip
 *
 * @param      cmd    cmd name
 * @param      usage  usage tip
 * @param      arg    arg passed by user
 */
void cli_print_usage(const char* cmd, const char* usage, const char* arg);

/* Sleep ms interrompable ETX/close ; pipe stdio, zéro/un octet consommé */

bool cli_sleep(PipeSide* side, uint32_t duration_in_ms);

#define CLI_COMMAND_INTERFACE(name, execute_callback, flags, stack_depth, app_id) \
    static const CliCommandDescriptor cli_##name##_desc = {                       \
        #name,                                                                    \
        &execute_callback,                                                        \
        flags,                                                                    \
        stack_depth,                                                              \
    };                                                                            \
                                                                                  \
    static const FlipperAppPluginDescriptor plugin_descriptor = {                 \
        .appid = app_id,                                                          \
        .ep_api_version = CLI_PLUGIN_API_VERSION,                                 \
        .entry_point = &cli_##name##_desc,                                        \
    };                                                                            \
                                                                                  \
    const FlipperAppPluginDescriptor* cli_##name##_ep(void) {                     \
        return &plugin_descriptor;                                                \
    }

#ifdef __cplusplus
}
#endif
