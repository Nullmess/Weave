
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <core/string.h>
#include <core/common_defines.h>
#include <toolbox/property.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    FuriHalPowerICCharger,
    FuriHalPowerICFuelGauge,
} FuriHalPowerIC;

void furi_hal_power_init(void);

/* Gauge vivant, profile correct et auto-diagnostic valide */
bool furi_hal_power_gauge_is_ok(void);

bool furi_hal_power_is_shutdown_requested(void);

/* Insomnia : 0 inactif, sinon nombre de détenteurs */
uint16_t furi_hal_power_insomnia_level(void);

/* Incrémente insomnia ; associer avec insomnia_exit */
void furi_hal_power_insomnia_enter(void);

/* Décrémente insomnia ; associer avec insomnia_enter */
void furi_hal_power_insomnia_exit(void);

bool furi_hal_power_sleep_available(void);

void furi_hal_power_sleep(void);

/* Capacité restante en pourcentage */
uint8_t furi_hal_power_get_pct(void);

/* État santé batterie en pourcentage */
uint8_t furi_hal_power_get_bat_health_pct(void);

bool furi_hal_power_is_charging(void);

/* Retour true si charge terminée et chargeur connecté */
bool furi_hal_power_is_charging_done(void);

void furi_hal_power_shutdown(void);

void furi_hal_power_off(void);

FURI_NORETURN void furi_hal_power_reset(void);

/* Contrôle bas niveau : privilégier le service power */
bool furi_hal_power_enable_otg(void);

/* Contrôle bas niveau : privilégier le service power */
void furi_hal_power_disable_otg(void);

bool furi_hal_power_check_otg_fault(void);

/* Désactivation OTG en cas de fault */
void furi_hal_power_check_otg_status(void);

bool furi_hal_power_is_otg_enabled(void);

/* Limite tension de charge en V */
float furi_hal_power_get_battery_charge_voltage_limit(void);

/* Tension en V, arrondie vers la valeur valide inférieure */
void furi_hal_power_set_battery_charge_voltage_limit(float voltage);

/* Capacité restante en mAh */
uint32_t furi_hal_power_get_battery_remaining_capacity(void);

/* Capacité pleine charge en mAh */
uint32_t furi_hal_power_get_battery_full_capacity(void);

/* Capacité profile batterie en mAh */
uint32_t furi_hal_power_get_battery_design_capacity(void);

/* Tension batterie en V */
float furi_hal_power_get_battery_voltage(FuriHalPowerIC ic);

/* Courant batterie en A */
float furi_hal_power_get_battery_current(FuriHalPowerIC ic);

/* Température en °C */
float furi_hal_power_get_battery_temperature(FuriHalPowerIC ic);

/* Tension USB en V */
float furi_hal_power_get_usb_voltage(void);

/* Activation 3,3 V sur GPIO externes et SD */
void furi_hal_power_enable_external_3_3v(void);

/* Désactivation 3,3 V sur GPIO externes et SD */
void furi_hal_power_disable_external_3_3v(void);

/* Charge suspendue pour alimentation propre */
void furi_hal_power_suppress_charge_enter(void);

void furi_hal_power_suppress_charge_exit(void);

void furi_hal_power_info_get(PropertyValueCallback callback, char sep, void* context);

void furi_hal_power_debug_get(PropertyValueCallback callback, void* context);

#ifdef __cplusplus
}
#endif
