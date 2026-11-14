/* Timings expérimentaux, dépendants fréquence I2C */
/* Reset puis attendre INITCOMP avant configuration */
/* Aucune mise à jour partielle ; reset avant nouvelle config */
/* Échec configuration : attendre 4 secondes avant retry */
/* Conserver sealed ; INITCOMP bloqué nécessite reset */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <furi_hal_i2c.h>

#define BQ27220_ERROR   0x0
#define BQ27220_SUCCESS 0x1

typedef struct {
    /* Octet bas, bit faible en premier */
    uint8_t BATT_ID : 3; /* Identification batterie */
    bool SNOOZE     : 1; /* Mode SNOOZE actif */
    bool BCA        : 1; /* Calibration carte fuel gauge active */
    bool CCA        : 1; /* Calibration Coulomb Counter active */
    uint8_t RSVD0   : 2; /* Réservé */
    /* Octet haut, bit faible en premier */
    uint8_t RSVD1; /* Réservé */
} Bq27220ControlStatus;

_Static_assert(sizeof(Bq27220ControlStatus) == 2, "Incorrect Bq27220ControlStatus structure size");

typedef struct {
    /* Octet bas, bit faible en premier */
    bool DSG      : 1; /* État DISCHARGE */
    bool SYSDWN   : 1; /* System down : arrêt système requis */
    bool TDA      : 1; /* Alarme fin décharge */
    bool BATTPRES : 1; /* Batterie présente */
    bool AUTH_GD  : 1; /* Insertion batterie détectée */
    bool OCVGD    : 1; /* Mesure OCV valide */
    bool TCA      : 1; /* Alarme fin charge */
    bool RSVD     : 1; /* Réservé */
    /* Octet haut, bit faible en premier */
    bool CHGINH   : 1; /* Charge inhibée */
    bool FC       : 1; /* Pleine charge détectée */
    bool OTD      : 1; /* Surtempérature pendant décharge */
    bool OTC      : 1; /* Surtempérature pendant charge */
    bool SLEEP    : 1; /* Mode SLEEP actif */
    bool OCVFAIL  : 1; /* Échec lecture OCV dû au courant */
    bool OCVCOMP  : 1; /* Mise à jour OCV terminée */
    bool FD       : 1; /* Décharge complète détectée */
} Bq27220BatteryStatus;

_Static_assert(sizeof(Bq27220BatteryStatus) == 2, "Incorrect Bq27220BatteryStatus structure size");

typedef enum {
    Bq27220OperationStatusSecSealed = 0b11,
    Bq27220OperationStatusSecUnsealed = 0b10,
    Bq27220OperationStatusSecFull = 0b01,
} Bq27220OperationStatusSec;

typedef struct {
    /* Octet bas, bit faible en premier */
    bool CALMD  : 1; /* Mode calibration actif */
    uint8_t SEC : 2; /* Niveau accès sécurité courant */
    bool EDV2   : 1; /* Seuil EDV2 dépassé */
    bool VDQ : 1; /* Cycle décharge qualifié ou non pour mise à jour FCC */
    bool INITCOMP  : 1; /* Init gauge terminée */
    bool SMTH      : 1; /* RemainingCapacity ajustée par smooth engine */
    bool BTPINT    : 1; /* Seuil BTP franchi */
    /* Octet haut, bit faible en premier */
    uint8_t RSVD1  : 2; /* Réservé */
    bool CFGUPDATE : 1; /* Mode CONFIG UPDATE */
    uint8_t RSVD0  : 5; /* Réservé */
} Bq27220OperationStatus;

_Static_assert(
    sizeof(Bq27220OperationStatus) == 2,
    "Incorrect Bq27220OperationStatus structure size");

typedef struct {
    /* Octet bas, bit faible en premier */
    bool FD       : 1; /* Décharge complète */
    bool FC       : 1; /* Charge complète */
    bool TD       : 1; /* Fin décharge */
    bool TC       : 1; /* Fin charge */
    bool RSVD0    : 1; /* Réservé */
    bool EDV      : 1; /* Tension au-dessus/dessous EDV0 */
    bool DSG      : 1; /* Décharge ou relaxation */
    bool CF       : 1; /* Conditionnement batterie nécessaire */
    /* Octet haut, bit faible en premier */
    uint8_t RSVD1 : 2; /* Réservé */
    bool FCCX     : 1; /* Clock fcc1hz vers CC : 0=1 Hz ; 1=16 Hz */
    uint8_t RSVD2 : 2; /* Réservé */
    bool EDV1     : 1; /* Tension au-dessus/dessous EDV1 */
    bool EDV2     : 1; /* Tension au-dessus/dessous EDV2 */
    bool VDQ      : 1; /* Cycle charge qualifié pour update FCC */
} Bq27220GaugingStatus;

_Static_assert(sizeof(Bq27220GaugingStatus) == 2, "Incorrect Bq27220GaugingStatus structure size");

typedef struct BQ27220DMData BQ27220DMData;

/* Vérifie ID 0220, profil 0 et data memory */
/* Unseal, vérifications/reset/configuration puis seal */

bool bq27220_init(const FuriHalI2cBusHandle* handle, const BQ27220DMData* data_memory);

bool bq27220_reset(const FuriHalI2cBusHandle* handle);

bool bq27220_seal(const FuriHalI2cBusHandle* handle);

bool bq27220_unseal(const FuriHalI2cBusHandle* handle);

/* État unsealed requis */

bool bq27220_full_access(const FuriHalI2cBusHandle* handle);

/* mV ou BQ27220_ERROR */

uint16_t bq27220_get_voltage(const FuriHalI2cBusHandle* handle);

/* mA ou BQ27220_ERROR */

int16_t bq27220_get_current(const FuriHalI2cBusHandle* handle);

bool bq27220_get_control_status(
    const FuriHalI2cBusHandle* handle,
    Bq27220ControlStatus* control_status);

bool bq27220_get_battery_status(
    const FuriHalI2cBusHandle* handle,
    Bq27220BatteryStatus* battery_status);

bool bq27220_get_operation_status(
    const FuriHalI2cBusHandle* handle,
    Bq27220OperationStatus* operation_status);

bool bq27220_get_gauging_status(
    const FuriHalI2cBusHandle* handle,
    Bq27220GaugingStatus* gauging_status);

/* Unités 0.1 kelvin */

uint16_t bq27220_get_temperature(const FuriHalI2cBusHandle* handle);

/* mAh ou BQ27220_ERROR */

uint16_t bq27220_get_full_charge_capacity(const FuriHalI2cBusHandle* handle);

/* mAh ou BQ27220_ERROR */

uint16_t bq27220_get_design_capacity(const FuriHalI2cBusHandle* handle);

/* mAh ou BQ27220_ERROR */

uint16_t bq27220_get_remaining_capacity(const FuriHalI2cBusHandle* handle);

/* Pourcentage ou BQ27220_ERROR */

uint16_t bq27220_get_state_of_charge(const FuriHalI2cBusHandle* handle);

/* Pourcentage ou BQ27220_ERROR */

uint16_t bq27220_get_state_of_health(const FuriHalI2cBusHandle* handle);
