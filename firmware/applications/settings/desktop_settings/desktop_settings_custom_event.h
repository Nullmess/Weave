#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* Réserver 100 aux boutons et sous-menus */
    DesktopSettingsCustomEventExit = 100,
    DesktopSettingsCustomEventDone,

    DesktopSettingsCustomEvent1stPinEntered,
    DesktopSettingsCustomEventPinsEqual,
    DesktopSettingsCustomEventPinsDifferent,

    DesktopSettingsCustomEventSetPin,
    DesktopSettingsCustomEventChangePin,
    DesktopSettingsCustomEventDisablePin,

    DesktopSettingsCustomEventSetDefault,
    DesktopSettingsCustomEventSetDummy,
} DesktopSettingsCustomEvent;

#ifdef __cplusplus
}
#endif
