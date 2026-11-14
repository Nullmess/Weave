

#pragma once

#include <furi.h>
#include <m-array.h>
#include <toolbox/pipe.h>
#include "cli_command.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CliRegistry CliRegistry;

/**
 * @brief Allocates a `CliRegistry`.
 */
CliRegistry* cli_registry_alloc(void);

/**
 * @brief Frees a `CliRegistry`.
 */
void cli_registry_free(CliRegistry* registry);

void cli_registry_add_command(
    CliRegistry* registry,
    const char* name,
    CliCommandFlag flags,
    CliCommandExecuteCallback callback,
    void* context);

/* stack_size définit stack thread commande */

void cli_registry_add_command_ex(
    CliRegistry* registry,
    const char* name,
    CliCommandFlag flags,
    CliCommandExecuteCallback callback,
    void* context,
    size_t stack_size);

void cli_registry_delete_command(CliRegistry* registry, const char* name);

void cli_registry_remove_external_commands(CliRegistry* registry);

void cli_registry_reload_external_commands(
    CliRegistry* registry,
    const CliCommandExternalConfig* config);

#ifdef __cplusplus
}
#endif
