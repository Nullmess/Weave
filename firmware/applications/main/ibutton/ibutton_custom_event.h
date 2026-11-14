#pragma once

typedef enum {
    /* Events 0..99 réservés aux boutons et index */
    iButtonCustomEventReserved = 100,

    iButtonCustomEventBack,
    iButtonCustomEventTextEditResult,
    iButtonCustomEventByteEditChanged,
    iButtonCustomEventByteEditResult,
    iButtonCustomEventWorkerEmulated,
    iButtonCustomEventWorkerRead,
    iButtonCustomEventWorkerWriteOK,
    iButtonCustomEventWorkerWriteSameKey,
    iButtonCustomEventWorkerWriteNoDetect,
    iButtonCustomEventWorkerWriteCannotWrite,

    iButtonCustomEventRpcLoadFile,
    iButtonCustomEventRpcExit,
    iButtonCustomEventRpcSessionClose,
} iButtonCustomEvent;
