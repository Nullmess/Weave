

#pragma once

#include <stdint.h>
#include <stdbool.h>

#include <datetime/datetime.h>
#include <core/common_defines.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriHalRtcFlagDebug = (1 << 0),
    FuriHalRtcFlagStorageFormatInternal = (1 << 1),
    FuriHalRtcFlagLock = (1 << 2),
    FuriHalRtcFlagC2Update = (1 << 3),
    FuriHalRtcFlagHandOrient = (1 << 4),
    FuriHalRtcFlagLegacySleep = (1 << 5),
    FuriHalRtcFlagStealthMode = (1 << 6),
    FuriHalRtcFlagDetailedFilename = (1 << 7),
} FuriHalRtcFlag;

typedef enum {
    FuriHalRtcBootModeNormal = 0, /* Boot normal par défaut */
    FuriHalRtcBootModeDfu, /* Boot DFU via bootloader MCU ST */
    FuriHalRtcBootModePreUpdate, /* Boot update : préparation */
    FuriHalRtcBootModeUpdate, /* Boot update : phase principale */
    FuriHalRtcBootModePostUpdate, /* Boot update : finalisation */
} FuriHalRtcBootMode;

typedef enum {
    FuriHalRtcHeapTrackModeNone = 0,
    FuriHalRtcHeapTrackModeMain, /* Tracking allocations : thread application principal */
    FuriHalRtcHeapTrackModeTree, /* Tracking allocations : thread application et enfants */
    FuriHalRtcHeapTrackModeAll, /* Tracking allocations : tous les threads */
} FuriHalRtcHeapTrackMode;

typedef enum {
    FuriHalRtcRegisterHeader,
    FuriHalRtcRegisterSystem,
    FuriHalRtcRegisterVersion,
    FuriHalRtcRegisterLfsFingerprint FURI_DEPRECATED, /* Empreinte géométrie LFS */
    FuriHalRtcRegisterFaultData,
    FuriHalRtcRegisterPinFails,
    /* Index entrée FS du firmware à installer */
    FuriHalRtcRegisterUpdateFolderFSIndex,
    FuriHalRtcRegisterPinValue, /* PIN encodé */

    FuriHalRtcRegisterMAX, /* Valeur interne réservée */
} FuriHalRtcRegister;

typedef enum {
    FuriHalRtcLocaleUnitsMetric = 0x0,
    FuriHalRtcLocaleUnitsImperial = 0x1,
} FuriHalRtcLocaleUnits;

typedef enum {
    FuriHalRtcLocaleTimeFormat24h = 0x0,
    FuriHalRtcLocaleTimeFormat12h = 0x1,
} FuriHalRtcLocaleTimeFormat;

typedef enum {
    FuriHalRtcLocaleDateFormatDMY = 0x0, /* Jour/mois/année */
    FuriHalRtcLocaleDateFormatMDY = 0x1, /* Mois/jour/année */
    FuriHalRtcLocaleDateFormatYMD = 0x2, /* Année/mois/jour */
} FuriHalRtcLocaleDateFormat;

typedef enum {
    FuriHalRtcLogDeviceUsart = 0x0, /* Par défaut : USART */
    FuriHalRtcLogDeviceLpuart = 0x1, /* Par défaut : LPUART */
    FuriHalRtcLogDeviceReserved = 0x2, /* Réservé */
    FuriHalRtcLogDeviceNone = 0x3, /* Désactive les logs série */
} FuriHalRtcLogDevice;

typedef enum {
    FuriHalRtcLogBaudRate230400 = 0x0, /**< 230400 baud */
    FuriHalRtcLogBaudRate9600 = 0x1, /**< 9600 baud */
    FuriHalRtcLogBaudRate38400 = 0x2, /**< 38400 baud */
    FuriHalRtcLogBaudRate57600 = 0x3, /**< 57600 baud */
    FuriHalRtcLogBaudRate115200 = 0x4, /**< 115200 baud */
    FuriHalRtcLogBaudRate460800 = 0x5, /**< 460800 baud */
    FuriHalRtcLogBaudRate921600 = 0x6, /**< 921600 baud */
    FuriHalRtcLogBaudRate1843200 = 0x7, /**< 1843200 baud */
} FuriHalRtcLogBaudRate;

void furi_hal_rtc_init_early(void);

void furi_hal_rtc_deinit_early(void);

void furi_hal_rtc_init(void);

/* Appeler avant transport mode pour réveiller le chargeur */
/* ALARM configurée sur PC13, bouton Back */

void furi_hal_rtc_prepare_for_shutdown(void);

void furi_hal_rtc_sync_shadow(void);

void furi_hal_rtc_reset_registers(void);

uint32_t furi_hal_rtc_get_register(FuriHalRtcRegister reg);

void furi_hal_rtc_set_register(FuriHalRtcRegister reg, uint32_t value);

void furi_hal_rtc_set_log_level(uint8_t level);

uint8_t furi_hal_rtc_get_log_level(void);

void furi_hal_rtc_set_log_device(FuriHalRtcLogDevice device);

FuriHalRtcLogDevice furi_hal_rtc_get_log_device(void);

void furi_hal_rtc_set_log_baud_rate(FuriHalRtcLogBaudRate baud_rate);

FuriHalRtcLogBaudRate furi_hal_rtc_get_log_baud_rate(void);

void furi_hal_rtc_set_flag(FuriHalRtcFlag flag);

void furi_hal_rtc_reset_flag(FuriHalRtcFlag flag);

bool furi_hal_rtc_is_flag_set(FuriHalRtcFlag flag);

void furi_hal_rtc_set_boot_mode(FuriHalRtcBootMode mode);

FuriHalRtcBootMode furi_hal_rtc_get_boot_mode(void);

void furi_hal_rtc_set_heap_track_mode(FuriHalRtcHeapTrackMode mode);

FuriHalRtcHeapTrackMode furi_hal_rtc_get_heap_track_mode(void);

void furi_hal_rtc_set_locale_units(FuriHalRtcLocaleUnits value);

FuriHalRtcLocaleUnits furi_hal_rtc_get_locale_units(void);

void furi_hal_rtc_set_locale_timeformat(FuriHalRtcLocaleTimeFormat value);

FuriHalRtcLocaleTimeFormat furi_hal_rtc_get_locale_timeformat(void);

void furi_hal_rtc_set_locale_dateformat(FuriHalRtcLocaleDateFormat value);

FuriHalRtcLocaleDateFormat furi_hal_rtc_get_locale_dateformat(void);

void furi_hal_rtc_set_datetime(DateTime* datetime);

void furi_hal_rtc_get_datetime(DateTime* datetime);

/* datetime=NULL conserve l’heure ; enabled contrôle l’alarme */

void furi_hal_rtc_set_alarm(const DateTime* datetime, bool enabled);

/* Renvoie true si alarme configurée */

bool furi_hal_rtc_get_alarm(DateTime* datetime);

typedef void (*FuriHalRtcAlarmCallback)(void* context);

/* Callback indépendant de la configuration alarme */
/* Normalement en ISR ; possible immédiatement si alarme déjà déclenchée */

void furi_hal_rtc_set_alarm_callback(FuriHalRtcAlarmCallback callback, void* context);

void furi_hal_rtc_set_fault_data(uint32_t value);

uint32_t furi_hal_rtc_get_fault_data(void);

void furi_hal_rtc_set_pin_fails(uint32_t value);

uint32_t furi_hal_rtc_get_pin_fails(void);

void furi_hal_rtc_set_pin_value(uint32_t value);

uint32_t furi_hal_rtc_get_pin_value(void);

/* Secondes depuis UNIX epoch */

uint32_t furi_hal_rtc_get_timestamp(void);

#ifdef __cplusplus
}
#endif
