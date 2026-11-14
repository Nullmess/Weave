#pragma once

#include <stm32wbxx_ll_i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriHalI2cBus FuriHalI2cBus;
typedef struct FuriHalI2cBusHandle FuriHalI2cBusHandle;

/** FuriHal i2c bus states */
typedef enum {
    FuriHalI2cBusEventInit, /* Init bus au démarrage système */
    FuriHalI2cBusEventDeinit, /* Deinit bus à arrêt système */
    FuriHalI2cBusEventLock, /* Bus lock avant activation */
    FuriHalI2cBusEventUnlock, /* Bus unlock après désactivation */
    FuriHalI2cBusEventActivate, /* Activation bus avant activation handle */
    FuriHalI2cBusEventDeactivate, /* Désactivation bus après désactivation handle */
} FuriHalI2cBusEvent;

/** FuriHal i2c bus event callback */
typedef void (*FuriHalI2cBusEventCallback)(FuriHalI2cBus* bus, FuriHalI2cBusEvent event);

/** FuriHal i2c bus */
struct FuriHalI2cBus {
    I2C_TypeDef* i2c;
    const FuriHalI2cBusHandle* current_handle;
    FuriHalI2cBusEventCallback callback;
};

/** FuriHal i2c handle states */
typedef enum {
    FuriHalI2cBusHandleEventActivate, /* Activation handle : GPIO connectées, config bus appliquée */
    FuriHalI2cBusHandleEventDeactivate, /* Désactivation handle : GPIO déconnectées, config bus reset */
} FuriHalI2cBusHandleEvent;

/** FuriHal i2c handle event callback */
typedef void (*FuriHalI2cBusHandleEventCallback)(
    const FuriHalI2cBusHandle* handle,
    FuriHalI2cBusHandleEvent event);

/** FuriHal i2c handle */
struct FuriHalI2cBusHandle {
    FuriHalI2cBus* bus;
    FuriHalI2cBusHandleEventCallback callback;
};

#ifdef __cplusplus
}
#endif
