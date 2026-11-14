#pragma once

#include <furi.h>
#include <toolbox/cli/cli_command.h>
#include <nfc/nfc.h>

/* Context action créé avant exécution, géré par callbacks de l’action */

typedef void NfcCliActionContext;

typedef NfcCliActionContext* (*NfcCliActionContextAlloc)(Nfc* nfc);

typedef void (*NfcCliActionContextFree)(NfcCliActionContext* action_ctx);

/* Réutilisation context décidée par action, notamment raw keep_field */

typedef bool (*NfcCliActionContextCanReuse)(NfcCliActionContext* ctx);

/* Pipe shell utilisable pour terminaison commande */

typedef void (*NfcCliActionHandlerCallback)(PipeSide* pipe, NfcCliActionContext* ctx);

/* Parser obligatoire par clé ; false empêche exécution et affiche erreur */

typedef bool (*NfcCliArgParseCallback)(FuriString* value, NfcCliActionContext* ctx);

typedef struct NfcCliKeyDescriptor NfcCliKeyDescriptor;

typedef struct NfcCliActionDescriptor NfcCliActionDescriptor;

typedef struct NfcCliCommandDescriptor NfcCliCommandDescriptor;
