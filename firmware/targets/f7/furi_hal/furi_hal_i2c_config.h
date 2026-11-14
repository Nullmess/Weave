#pragma once

#include <furi_hal_i2c_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Bus power interne I2C1 sous reset hors utilisation */
extern FuriHalI2cBus furi_hal_i2c_bus_power;

/* Bus externe I2C3 sous reset hors utilisation */
extern FuriHalI2cBus furi_hal_i2c_bus_external;

/* I2C interne : PA9 SCL, PA10 SDA ; flottants après release ; 400 kHz */

extern const FuriHalI2cBusHandle furi_hal_i2c_handle_power;

/* I2C externe : PC0 SCL, PC1 SDA ; flottants après release ; 100 kHz */

extern const FuriHalI2cBusHandle furi_hal_i2c_handle_external;

#ifdef __cplusplus
}
#endif
