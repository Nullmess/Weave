#pragma once

typedef enum {
    /* Events 0..99 réservés aux boutons et index */
    NfcCustomEventReserved = 100,

    // Mf classic dict attack events
    NfcCustomEventDictAttackComplete,
    NfcCustomEventDictAttackSkip,
    NfcCustomEventDictAttackDataUpdate,

    NfcCustomEventCardDetected,
    NfcCustomEventCardLost,

    NfcCustomEventViewExit,
    NfcCustomEventRetry,
    NfcCustomEventWorkerExit,
    NfcCustomEventWorkerUpdate,
    NfcCustomEventWrongCard,
    NfcCustomEventTimerExpired,
    NfcCustomEventByteInputDone,
    NfcCustomEventTextInputDone,
    NfcCustomEventDictAttackDone,

    NfcCustomEventRpcLoadFile,
    NfcCustomEventRpcExit,
    NfcCustomEventRpcSessionClose,

    NfcCustomEventPollerSuccess,
    NfcCustomEventPollerIncomplete,
    NfcCustomEventPollerFailure,

    NfcCustomEventListenerUpdate,
} NfcCustomEvent;
