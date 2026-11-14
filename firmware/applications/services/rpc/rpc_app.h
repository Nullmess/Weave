/* Commandes RPC asynchrones via callback */
/* Messages : flipperzero-protobuf/application.proto */

#pragma once

#include "rpc.h"
#include "rpc_app_error_codes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RpcAppSystemEventDataTypeNone,
    RpcAppSystemEventDataTypeString, /* String terminée par NUL */
    RpcAppSystemEventDataTypeInt32, /* Entier signé 32 bits */
    RpcAppSystemEventDataTypeBytes, /* Zéro ou plusieurs octets */
} RpcAppSystemEventDataType;

/* Champs valides uniquement pour le type correspondant */

typedef struct {
    RpcAppSystemEventDataType
        type;
    union {
        const char* string;
        int32_t i32;
        struct {
            const uint8_t* ptr;
            size_t size;
        } bytes;
    };
} RpcAppSystemEventData;

typedef enum {
    /* Event invalide interdit au callback */

    RpcAppEventTypeInvalid,
    /* Session fermée : contexte RPC désormais invalide */

    RpcAppEventTypeSessionClose,
    /* AppExit : application doit quitter */

    RpcAppEventTypeAppExit,
    /* LoadFile : sémantique dépend application */

    RpcAppEventTypeLoadFile,
    /* ButtonPress : sémantique dépend application */

    RpcAppEventTypeButtonPress,
    /* ButtonRelease : sémantique dépend application */

    RpcAppEventTypeButtonRelease,
    /* ButtonPressRelease : sémantique dépend application */

    RpcAppEventTypeButtonPressRelease,
    /* Transport RPC pour échanges de données arbitraires */

    RpcAppEventTypeDataExchange,
} RpcAppSystemEventType;

typedef struct {
    RpcAppSystemEventType type;
    RpcAppSystemEventData data;
} RpcAppSystemEvent;

/* Pointer event valide uniquement pendant le callback */
typedef void (*RpcAppSystemCallback)(const RpcAppSystemEvent* event, void* context);

typedef struct RpcAppSystem RpcAppSystem;

void rpc_system_app_set_callback(
    RpcAppSystem* rpc_app,
    RpcAppSystemCallback callback,
    void* context);

/* Appeler une fois après acquisition contexte et configuration callback */

void rpc_system_app_send_started(RpcAppSystem* rpc_app);

/* Notifier lors fermeture application, généralement dans free */

void rpc_system_app_send_exited(RpcAppSystem* rpc_app);

/* Confirmation obligatoire : AppExit/LoadFile/ButtonPress/ButtonRelease */
/* Confirmer aussi ButtonPressRelease/DataExchange ; sinon timeout client */
/* result=true : commande traitée avec succès */

void rpc_system_app_confirm(RpcAppSystem* rpc_app, bool result);

/* Code arbitraire transmis au client via GetError */

void rpc_system_app_set_error_code(RpcAppSystem* rpc_app, uint32_t error_code);

/* Texte arbitraire transmis au client via GetError */

void rpc_system_app_set_error_text(RpcAppSystem* rpc_app, const char* error_text);

/* Reset : code 0, texte vide */

void rpc_system_app_error_reset(RpcAppSystem* rpc_app);

void rpc_system_app_exchange_data(RpcAppSystem* rpc_app, const uint8_t* data, size_t data_size);

#ifdef __cplusplus
}
#endif
