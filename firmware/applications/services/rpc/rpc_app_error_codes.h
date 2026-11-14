#pragma once

typedef enum {
    RpcAppSystemErrorCodeNone,
    RpcAppSystemErrorCodeParseFile, /* Parsing/structure/paramètres invalides ; détails via debug */
    RpcAppSystemErrorCodeRegionLock, /* Fonction interdite par région */
    RpcAppSystemErrorCodeInternalParse, /* Paramètres protocole invalides ou données non supportées */
} RpcAppSystemErrorCode;
