#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RPC_BUFFER_SIZE (1024)

#define RECORD_RPC "rpc"

/* Interface RPC pour ouverture session uniquement */
typedef struct Rpc Rpc;
/** Rpc session interface */
typedef struct RpcSession RpcSession;

/* Callback transmission données au client */
typedef void (*RpcSendBytesCallback)(void* context, uint8_t* bytes, size_t bytes_len);

typedef void (*RpcBufferIsEmptyCallback)(void* context);
/* Callback informe transport ; aucune destruction/fermeture automatique */

typedef void (*RpcSessionClosedCallback)(void* context);
/* Callback après fermeture session et fin opérations */

typedef void (*RpcSessionTerminatedCallback)(void* context);

/** RPC owner */
typedef enum {
    RpcOwnerUnknown = 0,
    RpcOwnerBle,
    RpcOwnerUsb,
    RpcOwnerUart,
    RpcOwnerCount,
} RpcOwner;

RpcOwner rpc_session_get_owner(RpcSession* session);

/* open puis set_context, set_send_bytes_callback, set_close_callback */
/* Feed pendant session puis close ; NULL si RPC occupé */

RpcSession* rpc_session_open(Rpc* rpc, RpcOwner owner);

/* Après close : aucun callback garanti, désassignation inutile */

void rpc_session_close(RpcSession* session);

void rpc_session_set_context(RpcSession* session, void* context);

/* RPC API interdite dans SendBytesCallback ; NULL admis */

void rpc_session_set_send_bytes_callback(RpcSession* session, RpcSendBytesCallback callback);

/* Callback buffer vide, NULL admis */

void rpc_session_set_buffer_is_empty_callback(
    RpcSession* session,
    RpcBufferIsEmptyCallback callback);

/* RPC API interdite dans SessionClosedCallback ; NULL admis */

void rpc_session_set_close_callback(RpcSession* session, RpcSessionClosedCallback callback);

void rpc_session_set_terminated_callback(
    RpcSession* session,
    RpcSessionTerminatedCallback callback);

/* Renvoie octets consommés ; timeout attente consommation buffer */

size_t rpc_session_feed(RpcSession* session, const uint8_t* buffer, size_t size, uint32_t timeout);

/* Capacité RPC disponible en octets */

size_t rpc_session_get_available_size(RpcSession* session);

#ifdef __cplusplus
}
#endif
