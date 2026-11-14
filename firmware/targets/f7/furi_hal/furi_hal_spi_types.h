#pragma once

#include <stdint.h>
#include <stddef.h>

#include <furi_hal_gpio.h>

#include <stm32wbxx_ll_spi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriHalSpiBus FuriHalSpiBus;
typedef struct FuriHalSpiBusHandle FuriHalSpiBusHandle;

/** FuriHal spi bus states */
typedef enum {
    FuriHalSpiBusEventInit, /* Init bus au démarrage système */
    FuriHalSpiBusEventDeinit, /* Deinit bus à arrêt système */
    FuriHalSpiBusEventLock, /* Bus lock avant activation */
    FuriHalSpiBusEventUnlock, /* Bus unlock après désactivation */
    FuriHalSpiBusEventActivate, /* Activation bus avant activation handle */
    FuriHalSpiBusEventDeactivate, /* Désactivation bus après désactivation handle */
} FuriHalSpiBusEvent;

/** FuriHal spi bus event callback */
typedef void (*FuriHalSpiBusEventCallback)(FuriHalSpiBus* bus, FuriHalSpiBusEvent event);

/** FuriHal spi bus */
struct FuriHalSpiBus {
    SPI_TypeDef* spi;
    FuriHalSpiBusEventCallback callback;
    const FuriHalSpiBusHandle* current_handle;
};

/** FuriHal spi handle states */
typedef enum {
    FuriHalSpiBusHandleEventInit, /* Init au démarrage : GPIO en idle */
    FuriHalSpiBusHandleEventDeinit, /* Deinit à arrêt : GPIO état par défaut */
    FuriHalSpiBusHandleEventActivate, /* Activation handle : GPIO connectées, config bus appliquée */
    FuriHalSpiBusHandleEventDeactivate, /* Désactivation handle : GPIO déconnectées, config bus reset */
} FuriHalSpiBusHandleEvent;

/** FuriHal spi handle event callback */
typedef void (*FuriHalSpiBusHandleEventCallback)(
    const FuriHalSpiBusHandle* handle,
    FuriHalSpiBusHandleEvent event);

/** FuriHal spi handle */
struct FuriHalSpiBusHandle {
    FuriHalSpiBus* bus;
    FuriHalSpiBusHandleEventCallback callback;
    const GpioPin* miso;
    const GpioPin* mosi;
    const GpioPin* sck;
    const GpioPin* cs;
};

#ifdef __cplusplus
}
#endif
