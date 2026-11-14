#pragma once

#include <furi.h>
#include <toolbox/pipe.h>
#include "../cli_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_SHELL_STACK_SIZE (4 * 1024U)

typedef struct CliShell CliShell;

/* Message of the Day depuis thread shell au démarrage */

typedef void (*CliShellMotd)(void* context);

/* ext_config NULL si commandes externes inutiles */

CliShell* cli_shell_alloc(
    CliShellMotd motd,
    void* context,
    PipeSide* pipe,
    CliRegistry* registry,
    const CliCommandExternalConfig* ext_config);

void cli_shell_free(CliShell* shell);

/* Start non bloquant, shell thread séparé */

void cli_shell_start(CliShell* shell);

/* Join bloquant */

void cli_shell_join(CliShell* shell);

/* Texte optionnel avant prompt >: */

void cli_shell_set_prompt(CliShell* shell, const char* prompt);

#ifdef __cplusplus
}
#endif
