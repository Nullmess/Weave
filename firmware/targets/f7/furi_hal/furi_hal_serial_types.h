#pragma once

#include <furi.h>

/**
 * UART channels
 */
typedef enum {
    FuriHalSerialIdUsart,
    FuriHalSerialIdLpuart,

    FuriHalSerialIdMax,
} FuriHalSerialId;

typedef enum {
    FuriHalSerialDirectionTx,
    FuriHalSerialDirectionRx,

    FuriHalSerialDirectionMax,
} FuriHalSerialDirection;

/* Bits data hors start/stop/parité ; 6 nécessite parité, 9 sans parité */

typedef enum {
    FuriHalSerialDataBits6,
    FuriHalSerialDataBits7,
    FuriHalSerialDataBits8,
    FuriHalSerialDataBits9,

    FuriHalSerialDataBitsMax,
} FuriHalSerialDataBits;

typedef enum {
    FuriHalSerialParityNone,
    FuriHalSerialParityEven,
    FuriHalSerialParityOdd,

    FuriHalSerialParityMax,
} FuriHalSerialParity;

/* LPUART stop bits entiers1/2 uniquement */

typedef enum {
    FuriHalSerialStopBits0_5,
    FuriHalSerialStopBits1,
    FuriHalSerialStopBits1_5,
    FuriHalSerialStopBits2,

    FuriHalSerialStopBits2Max,
} FuriHalSerialStopBits;

typedef struct FuriHalSerialHandle FuriHalSerialHandle;
