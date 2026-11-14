

#pragma once

#include <furi.h>
#include <m-dict.h>
#include "cli_registry.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CLI_BUILTIN_COMMAND_STACK_SIZE (4 * 1024U)

typedef struct {
    void* context; /* Contexte transmis aux callbacks */
    CliCommandExecuteCallback execute_callback; /* Callback exécution commande */
    CliCommandFlag flags;
    size_t stack_depth;
} CliRegistryCommand;

DICT_DEF2(CliCommandDict, FuriString*, FURI_STRING_OPLIST, CliRegistryCommand, M_POD_OPLIST);

#define M_OPL_CliCommandDict_t() DICT_OPLIST(CliCommandDict, FURI_STRING_OPLIST, M_POD_OPLIST)

bool cli_registry_get_command(
    CliRegistry* registry,
    FuriString* command,
    CliRegistryCommand* result);

void cli_registry_lock(CliRegistry* registry);

void cli_registry_unlock(CliRegistry* registry);

/* Entourer appel avec cli_registry_lock/unlock */

CliCommandDict_t* cli_registry_get_commands(CliRegistry* registry);

#ifdef __cplusplus
}
#endif
