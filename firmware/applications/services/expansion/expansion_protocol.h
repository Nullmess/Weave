/**
 * @file expansion_protocol.h
 * @brief Flipper Expansion Protocol parser reference implementation.
 *
 * This file is licensed separately under The Unlicense.
 * See https://unlicense.org/ for more details.
 *
 * This parser is written with low-spec hardware in mind. It does not use
 * dynamic memory allocation or Flipper-specific libraries and can be
 * included directly into any module's firmware's sources.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EXPANSION_PROTOCOL_DEFAULT_BAUD_RATE (9600UL)

#define EXPANSION_PROTOCOL_MAX_DATA_SIZE (64U)

#define EXPANSION_PROTOCOL_TIMEOUT_MS (250U)

/* Temps mort après changement baud rate */

#define EXPANSION_PROTOCOL_BAUD_CHANGE_DT_MS (25U)

typedef enum {
    ExpansionFrameTypeHeartbeat = 1, /**< Heartbeat frame. */
    ExpansionFrameTypeStatus = 2, /**< Status report frame. */
    ExpansionFrameTypeBaudRate = 3, /**< Baud rate negotiation frame. */
    ExpansionFrameTypeControl = 4, /**< Control frame. */
    ExpansionFrameTypeData = 5, /**< Data frame. */
    ExpansionFrameTypeReserved, /**< Special value. */
} ExpansionFrameType;

typedef enum {
    ExpansionFrameErrorNone = 0x00,
    ExpansionFrameErrorUnknown = 0x01, /**< An unknown error has occurred (generic response). */
    ExpansionFrameErrorBaudRate = 0x02, /* Baud rate demandé non supporté */
} ExpansionFrameError;

typedef enum {
    /* Session RPC doit être inactive */

    ExpansionFrameControlCommandStartRpc = 0x00,
    /* Session RPC doit être active */

    ExpansionFrameControlCommandStopRpc = 0x01,
    /* OTG GPIO hors session RPC seulement ; sinon via RPC */

    ExpansionFrameControlCommandEnableOtg = 0x02,
    /* OTG GPIO hors session RPC seulement ; sinon via RPC */

    ExpansionFrameControlCommandDisableOtg = 0x03,
} ExpansionFrameControlCommand;

#pragma pack(push, 1)

/**
 * @brief Frame header structure.
 */
typedef struct {
    uint8_t type;
} ExpansionFrameHeader;

/**
 * @brief Heartbeat frame contents.
 */
typedef struct {
    /** Empty. */
} ExpansionFrameHeartbeat;

/**
 * @brief Status frame contents.
 */
typedef struct {
    uint8_t error; /**< Reported error code. @see ExpansionFrameError. */
} ExpansionFrameStatus;

/**
 * @brief Baud rate frame contents.
 */
typedef struct {
    uint32_t baud; /**< Requested baud rate. */
} ExpansionFrameBaudRate;

/**
 * @brief Control frame contents.
 */
typedef struct {
    uint8_t command; /**< Control command number. @see ExpansionFrameControlCommand. */
} ExpansionFrameControl;

/**
 * @brief Data frame contents.
 */
typedef struct {
    /* Taille strictement inférieure à EXPANSION_PROTOCOL_MAX_DATA_SIZE */
    uint8_t size;
    /* Données valides sur ExpansionFrameData::size octets */
    uint8_t bytes[EXPANSION_PROTOCOL_MAX_DATA_SIZE];
} ExpansionFrameData;

/**
 * @brief Expansion protocol frame structure.
 */
typedef struct {
    ExpansionFrameHeader header; /* Header obligatoire */
    union {
        ExpansionFrameHeartbeat heartbeat; /**< Heartbeat frame contents. */
        ExpansionFrameStatus status; /**< Status frame contents. */
        ExpansionFrameBaudRate baud_rate; /**< Baud rate frame contents. */
        ExpansionFrameControl control; /**< Control frame contents. */
        ExpansionFrameData data; /**< Data frame contents. */
    } content;
} ExpansionFrame;

#pragma pack(pop)

/**
 * @brief Expansion checksum type.
 */
typedef uint8_t ExpansionFrameChecksum;

/* Capacité octets ; renvoie nombre octets reçus */

typedef size_t (*ExpansionFrameReceiveCallback)(uint8_t* data, size_t data_size, void* context);

/* Taille octets ; renvoie nombre octets envoyés */

typedef size_t (*ExpansionFrameSendCallback)(const uint8_t* data, size_t data_size, void* context);

/* Frame complète valide requise ; taille encodée en octets */

static inline size_t expansion_frame_get_encoded_size(const ExpansionFrame* frame) {
    switch(frame->header.type) {
    case ExpansionFrameTypeHeartbeat:
        return sizeof(frame->header);
    case ExpansionFrameTypeStatus:
        return sizeof(frame->header) + sizeof(frame->content.status);
    case ExpansionFrameTypeBaudRate:
        return sizeof(frame->header) + sizeof(frame->content.baud_rate);
    case ExpansionFrameTypeControl:
        return sizeof(frame->header) + sizeof(frame->content.control);
    case ExpansionFrameTypeData:
        return sizeof(frame->header) + sizeof(frame->content.data.size) + frame->content.data.size;
    default:
        return 0;
    }
}

/* remaining_size=0 si complète ; false si calcul impossible */

static inline bool expansion_frame_get_remaining_size(
    const ExpansionFrame* frame,
    size_t received_size,
    size_t* remaining_size) {
    if(received_size < sizeof(ExpansionFrameHeader)) {
        /* Type frame encore inconnu */
        *remaining_size = sizeof(ExpansionFrameHeader);
        return true;
    }

    const size_t received_content_size = received_size - sizeof(ExpansionFrameHeader);
    size_t content_size;

    switch(frame->header.type) {
    case ExpansionFrameTypeHeartbeat:
        content_size = 0;
        break;
    case ExpansionFrameTypeStatus:
        content_size = sizeof(frame->content.status);
        break;
    case ExpansionFrameTypeBaudRate:
        content_size = sizeof(frame->content.baud_rate);
        break;
    case ExpansionFrameTypeControl:
        content_size = sizeof(frame->content.control);
        break;
    case ExpansionFrameTypeData:
        if(received_content_size < sizeof(frame->content.data.size)) {
            /* Taille data encore inconnue */
            content_size = sizeof(frame->content.data.size);
        } else if(frame->content.data.size > sizeof(frame->content.data.bytes)) {

            return false;
        } else {
            content_size = sizeof(frame->content.data.size) + frame->content.data.size;
        }
        break;
    default:
        return false;
    }

    if(content_size > received_content_size) {
        *remaining_size = content_size - received_content_size;
    } else {
        *remaining_size = 0;
    }

    return true;
}

typedef enum {
    ExpansionProtocolStatusOk,
    ExpansionProtocolStatusErrorFormat, /**< Invalid frame type. */
    ExpansionProtocolStatusErrorChecksum, /**< Checksum mismatch. */
    ExpansionProtocolStatusErrorCommunication, /**< Input/output error. */
} ExpansionProtocolStatus;

/* Checksum XOR léger pour détection basique erreur */

static inline ExpansionFrameChecksum
    expansion_protocol_get_checksum(const uint8_t* data, size_t data_size) {
    ExpansionFrameChecksum checksum = 0;
    for(size_t i = 0; i < data_size; ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

/* Callback receive répété jusqu’à frame complète */

static inline ExpansionProtocolStatus expansion_protocol_decode(
    ExpansionFrame* frame,
    ExpansionFrameReceiveCallback receive,
    void* context) {
    size_t total_size = 0;
    size_t remaining_size;

    while(true) {
        if(!expansion_frame_get_remaining_size(frame, total_size, &remaining_size)) {
            return ExpansionProtocolStatusErrorFormat;
        } else if(remaining_size == 0) {
            break;
        }

        const size_t received_size =
            receive((uint8_t*)frame + total_size, remaining_size, context);

        if(received_size == 0) {
            return ExpansionProtocolStatusErrorCommunication;
        }

        total_size += received_size;
    }

    ExpansionFrameChecksum checksum;
    const size_t received_size = receive(&checksum, sizeof(checksum), context);

    if(received_size != sizeof(checksum)) {
        return ExpansionProtocolStatusErrorCommunication;
    } else if(checksum != expansion_protocol_get_checksum((const uint8_t*)frame, total_size)) {
        return ExpansionProtocolStatusErrorChecksum;
    } else {
        return ExpansionProtocolStatusOk;
    }
}

static inline ExpansionProtocolStatus expansion_protocol_encode(
    const ExpansionFrame* frame,
    ExpansionFrameSendCallback send,
    void* context) {
    const size_t encoded_size = expansion_frame_get_encoded_size(frame);
    if(encoded_size == 0) {
        return ExpansionProtocolStatusErrorFormat;
    }

    const ExpansionFrameChecksum checksum =
        expansion_protocol_get_checksum((const uint8_t*)frame, encoded_size);

    if((send((const uint8_t*)frame, encoded_size, context) != encoded_size) ||
       (send(&checksum, sizeof(checksum), context) != sizeof(checksum))) {
        return ExpansionProtocolStatusErrorCommunication;
    } else {
        return ExpansionProtocolStatusOk;
    }
}

#ifdef __cplusplus
}
#endif
