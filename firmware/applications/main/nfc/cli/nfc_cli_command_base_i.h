#pragma once

#include "nfc_cli_command_base.h"
#include <toolbox/cli/cli_ansi.h>
#include <nfc/nfc.h>
#include <nfc/protocols/nfc_protocol.h>
#include "nfc_cli_command_processor.h"

/* Commandes CLI : fichiers commands/nfc_cli_command_<cmd>.c/h */
/* Ajouter header dans nfc_cli_commands.c et entrée tableau nfc_cli_commands */
/* Inclure source dans plugin cli_nfc du manifest */
/* Descriptor direct : parsing/arguments à gérer manuellement */
/* ADD_NFC_CLI_COMMAND : processor assure parsing/help/erreurs */
/* Déclarer <name>_cmd dans header, puis actions et clés */
/* Enregistrement shell direct déconseillé, sans parsing/help automatiques */

typedef struct {
    bool required : 1; /* Argument obligatoire ; absence provoque erreur */
    bool parameter : 1; /* Argument exige valeur si marqué, sinon switch on/off */
    bool multivalue : 1; /* Argument peut accepter plusieurs valeurs après son nom */
} FURI_PACKED NfcCliKeyFeatureSupport;

struct NfcCliKeyDescriptor {
    NfcCliKeyFeatureSupport features; /**< Features supported defining key behaviour */
    const char* long_name; /* Option longue préfixée -- */
    const char* short_name; /* Option courte préfixée - */
    const char* description;
    NfcCliArgParseCallback parse; /**< Parsing callback */
};

/**
 * @brief Describes action
 */
struct NfcCliActionDescriptor {
    const char* name; /* Action obligatoirement premier argument après commande */
    const char* description;
    size_t key_count;
    const NfcCliKeyDescriptor* keys;

    NfcCliActionHandlerCallback execute; /* Callback action après parsing réussi */
    NfcCliActionContextAlloc alloc; /**< Allocates action context during command processing */
    NfcCliActionContextFree free; /**< Frees action context */
    NfcCliActionContextCanReuse can_reuse; /**< Checks context reuse possibility */
};

/**
 * @brief Describes command
 */
struct NfcCliCommandDescriptor {
    const char* name;
    const char* description;
    size_t action_count;
    const NfcCliActionDescriptor** actions;
    CliCommandExecuteCallback callback;
};

/* Macro crée descriptor et callback vers command processor */

#define ADD_NFC_CLI_COMMAND(name, description, actions)                  \
    static void nfc_cli_command_##name##_callback(                       \
        PipeSide* pipe, FuriString* args, void* context);                \
                                                                         \
    const NfcCliCommandDescriptor name##_cmd = {                         \
        #name,                                                           \
        #description,                                                    \
        COUNT_OF(actions),                                               \
        actions,                                                         \
        nfc_cli_command_##name##_callback,                               \
    };                                                                   \
                                                                         \
    static void nfc_cli_command_##name##_callback(                       \
        PipeSide* pipe, FuriString* args, void* context) {               \
        nfc_cli_command_processor_run(&name##_cmd, pipe, args, context); \
    }
