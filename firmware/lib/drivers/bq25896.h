#pragma once

#include "bq25896_reg.h"

#include <stdbool.h>
#include <stdint.h>
#include <furi_hal_i2c.h>

bool bq25896_init(const FuriHalI2cBusHandle* handle);

void bq25896_set_boost_lim(const FuriHalI2cBusHandle* handle, BoostLim boost_lim);

void bq25896_poweroff(const FuriHalI2cBusHandle* handle);

ChrgStat bq25896_get_charge_status(const FuriHalI2cBusHandle* handle);

/** Is currently charging */
bool bq25896_is_charging(const FuriHalI2cBusHandle* handle);

/* Charge terminée avec chargeur connecté */
bool bq25896_is_charging_done(const FuriHalI2cBusHandle* handle);

void bq25896_enable_charging(const FuriHalI2cBusHandle* handle);

void bq25896_disable_charging(const FuriHalI2cBusHandle* handle);

void bq25896_enable_otg(const FuriHalI2cBusHandle* handle);

void bq25896_disable_otg(const FuriHalI2cBusHandle* handle);

/** Is otg enabled */
bool bq25896_is_otg_enabled(const FuriHalI2cBusHandle* handle);

/* VREG en mV */
uint16_t bq25896_get_vreg_voltage(const FuriHalI2cBusHandle* handle);

/* VREG 3840..4208 mV par pas de 16 mV */

void bq25896_set_vreg_voltage(const FuriHalI2cBusHandle* handle, uint16_t vreg_voltage);

bool bq25896_check_otg_fault(const FuriHalI2cBusHandle* handle);

/* VBUS en mV */
uint16_t bq25896_get_vbus_voltage(const FuriHalI2cBusHandle* handle);

/* VSYS en mV */
uint16_t bq25896_get_vsys_voltage(const FuriHalI2cBusHandle* handle);

/* VBAT en mV */
uint16_t bq25896_get_vbat_voltage(const FuriHalI2cBusHandle* handle);

/* Courant VBAT en mA */
uint16_t bq25896_get_vbat_current(const FuriHalI2cBusHandle* handle);

/* Voltage NTC en milli-pourcent de REGN */
uint32_t bq25896_get_ntc_mpct(const FuriHalI2cBusHandle* handle);
