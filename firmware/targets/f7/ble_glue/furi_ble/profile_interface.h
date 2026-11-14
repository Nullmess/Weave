#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <gap.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriHalBleProfileTemplate FuriHalBleProfileTemplate;

/* Structure obligatoirement premier champ des profils concrets */
typedef struct {
    /* Config profil obligatoire pour vérifier appartenance instance */

    const FuriHalBleProfileTemplate* config;
} FuriHalBleProfileBase;

typedef void* FuriHalBleProfileParams;

typedef FuriHalBleProfileBase* (*FuriHalBleProfileStart)(FuriHalBleProfileParams profile_params);
typedef void (*FuriHalBleProfileStop)(FuriHalBleProfileBase* profile);
typedef void (*FuriHalBleProfileGetGapConfig)(
    GapConfig* target_config,
    FuriHalBleProfileParams profile_params);

struct FuriHalBleProfileTemplate {

    FuriHalBleProfileStart start;
    /* Vérifier appartenance instance au profil avant destruction */
    FuriHalBleProfileStop stop;
    /* Configuration GAP avant démarrage profil */
    FuriHalBleProfileGetGapConfig get_gap_config;
};

#ifdef __cplusplus
}
#endif
