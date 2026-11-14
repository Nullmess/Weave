#pragma once

#include "mf_classic.h"
#include <lib/nfc/protocols/iso14443_3a/iso14443_3a_poller.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct MfClassicPoller MfClassicPoller;

typedef enum {
    MfClassicPollerEventTypeRequestMode, /* Mode à renseigner par application */

    MfClassicPollerEventTypeRequestReadSector, /* Poller demande données pour lecture secteur */

    MfClassicPollerEventTypeRequestSectorTrailer, /* Poller demande sector trailer pour écriture bloc */
    MfClassicPollerEventTypeRequestWriteBlock, /* Poller demande données pour écriture bloc */

    MfClassicPollerEventTypeRequestKey, /* Poller demande clé authentification secteur */
    MfClassicPollerEventTypeNextSector, /* Secteur suivant pendant attaque dictionnaire */
    MfClassicPollerEventTypeDataUpdate, /**< Poller updates data. */
    MfClassicPollerEventTypeFoundKeyA, /**< Poller found key A. */
    MfClassicPollerEventTypeFoundKeyB, /**< Poller found key B. */
    MfClassicPollerEventTypeKeyAttackStart, /**< Poller starts key attack. */
    MfClassicPollerEventTypeKeyAttackStop, /**< Poller stops key attack. */
    MfClassicPollerEventTypeKeyAttackNextSector, /* Secteur suivant pendant attaque clé */

    MfClassicPollerEventTypeCardDetected, /**< Poller detected card. */
    MfClassicPollerEventTypeCardLost, /**< Poller lost card. */
    MfClassicPollerEventTypeSuccess, /**< Poller succeeded. */
    MfClassicPollerEventTypeFail, /**< Poller failed. */
} MfClassicPollerEventType;

/**
 * @brief MfClassic poller mode.
 */
typedef enum {
    MfClassicPollerModeRead, /**< Poller reading mode. */
    MfClassicPollerModeWrite, /**< Poller writing mode. */
    MfClassicPollerModeDictAttackStandard, /**< Poller dictionary attack mode. */
    MfClassicPollerModeDictAttackCUID, /**< Poller CUID dictionary attack mode. */
    MfClassicPollerModeDictAttackEnhanced, /**< Poller enhanced dictionary attack mode. */
} MfClassicPollerMode;

/**
 * @brief MfClassic poller nested attack phase.
 */
typedef enum {
    MfClassicNestedPhaseNone, /* Aucune attaque nested effectuée */
    MfClassicNestedPhaseAnalyzePRNG, /* Analyse nonces pour identifier PRNG faible */
    MfClassicNestedPhaseDictAttack, /* Recherche clés compatibles PRNG/parité des nonces */
    MfClassicNestedPhaseDictAttackVerify, /* Validation clé par authentification secteur */
    MfClassicNestedPhaseDictAttackResume, /* Reprise nested depuis dernière clé invalide */
    MfClassicNestedPhaseCalibrate, /* Calcul nonce plaintext ultérieur ; PRNG faible uniquement */
    MfClassicNestedPhaseRecalibrate, /* Collecte nonce plaintext pour attaque nested static encrypted */
    MfClassicNestedPhaseCollectNtEnc, /* Journaliser nonces nested pour récupération clés */
    MfClassicNestedPhaseFinished, /**< Nested attack has finished */
} MfClassicNestedPhase;

/**
 * @brief MfClassic pseudorandom number generator (PRNG) type.
 */
typedef enum {
    MfClassicPrngTypeUnknown, /* Tag non testé */
    MfClassicPrngTypeNoTag, /* Aucun tag détecté pendant test */
    MfClassicPrngTypeWeak, // Weak PRNG, standard Nested
    MfClassicPrngTypeHard, // Hard PRNG, Hardnested
} MfClassicPrngType;

/**
 * @brief MfClassic authentication backdoor type.
 */
typedef enum {
    MfClassicBackdoorUnknown, /* Tag non testé */
    MfClassicBackdoorNone, /* Aucune backdoor observée */
    MfClassicBackdoorAuth1, /* Tag répond à backdoor authentification V1 */
    MfClassicBackdoorAuth2, /* Backdoor V2 ; parfois chiffrement statique */
    MfClassicBackdoorAuth3, /* Backdoor V3 ; nonce chiffré statique */
} MfClassicBackdoor;

/* À remplir sur event RequestMode */

typedef struct {
    MfClassicPollerMode mode; /* Mode du poller */
    const MfClassicData* data; /* Données du poller */
} MfClassicPollerEventDataRequestMode;

/* Rempli par poller sur event NextSector */

typedef struct {
    uint8_t current_sector; /**< Current sector number. */
} MfClassicPollerEventDataDictAttackNextSector;

/* Rempli par poller sur event DataUpdate */

typedef struct {
    uint8_t sectors_read;
    uint8_t keys_found;
    uint8_t current_sector; /**< Current sector number. */
    MfClassicNestedPhase nested_phase; /**< Nested attack phase. */
    MfClassicPrngType prng_type; /* PRNG faible ou robuste */
    MfClassicBackdoor backdoor; /**< Backdoor type. */
    uint16_t nested_target_key; /* Clé cible pour attaque nested */
    uint16_t
        msb_count; /* Nombre de MSB distincts en attaque Hardnested */
} MfClassicPollerEventDataUpdate;

/* À remplir sur event RequestKey */

typedef struct {
    MfClassicKey key; /* Clé du poller */
    MfClassicKeyType key_type; /* Type clé A/B pour attaque dictionnaire CUID */
    bool key_provided; /* Clé fournie */
} MfClassicPollerEventDataKeyRequest;

/* À remplir sur event RequestReadSector */

typedef struct {
    uint8_t sector_num;
    MfClassicKey key; /* Clé du poller */
    MfClassicKeyType key_type;
    bool key_provided; /* Clé fournie */
} MfClassicPollerEventDataReadSectorRequest;

/* À remplir sur event RequestSectorTrailer */

typedef struct {
    uint8_t sector_num;
    MfClassicBlock sector_trailer;
    bool sector_trailer_provided; /* Sector trailer fourni */
} MfClassicPollerEventDataSectorTrailerRequest;

/* À remplir sur event RequestWriteBlock */

typedef struct {
    uint8_t block_num;
    MfClassicBlock write_block;
    bool write_block_provided; /* Bloc fourni */
} MfClassicPollerEventDataWriteBlockRequest;

/* Rempli par poller sur event KeyAttackNextSector */

typedef struct {
    uint8_t current_sector; /**< Current sector number. */
} MfClassicPollerEventKeyAttackData;

typedef union {
    MfClassicError error; /* Code erreur sur event MfClassicPollerEventTypeFail */
    MfClassicPollerEventDataRequestMode poller_mode; /**< Poller mode context. */
    MfClassicPollerEventDataDictAttackNextSector next_sector_data; /**< Next sector context. */
    MfClassicPollerEventDataKeyRequest key_request_data; /**< Key request context. */
    MfClassicPollerEventDataUpdate data_update; /**< Data update context. */
    MfClassicPollerEventDataReadSectorRequest
        read_sector_request_data;
    MfClassicPollerEventKeyAttackData key_attack_data; /**< Key attack context. */
    MfClassicPollerEventDataSectorTrailerRequest sec_tr_data; /**< Sector trailer request context. */
    MfClassicPollerEventDataWriteBlockRequest write_block_data;
} MfClassicPollerEventData;

typedef struct {
    MfClassicPollerEventType type; /**< Event type. */
    MfClassicPollerEventData* data; /* Données propres à l’événement */
} MfClassicPollerEvent;

/* Usage uniquement dans callback ; backdoor_auth active auth backdoor */
MfClassicError mf_classic_poller_get_nt(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicKeyType key_type,
    MfClassicNt* nt,
    bool backdoor_auth);

/* Usage uniquement dans callback ; backdoor_auth active auth backdoor */
MfClassicError mf_classic_poller_get_nt_nested(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicKeyType key_type,
    MfClassicNt* nt,
    bool backdoor_auth);

/* Usage uniquement dans callback ; initialise état crypto */
MfClassicError mf_classic_poller_auth(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicKey* key,
    MfClassicKeyType key_type,
    MfClassicAuthContext* data,
    bool backdoor_auth);

/* Usage uniquement dans callback ; early_ret après nonce chiffré */
MfClassicError mf_classic_poller_auth_nested(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicKey* key,
    MfClassicKeyType key_type,
    MfClassicAuthContext* data,
    bool backdoor_auth,
    bool early_ret);

/* Usage uniquement dans callback ; Halt et reset état crypto */
MfClassicError mf_classic_poller_halt(MfClassicPoller* instance);

/* Usage uniquement dans le callback */
MfClassicError mf_classic_poller_read_block(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicBlock* data);

/* Usage uniquement dans le callback */
MfClassicError mf_classic_poller_write_block(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicBlock* data);

/* Usage uniquement dans callback ; résultat dans bloc transfert interne */
/* Value_transfer requis pour appliquer le résultat au tag */
MfClassicError mf_classic_poller_value_cmd(
    MfClassicPoller* instance,
    uint8_t block_num,
    MfClassicValueCommand cmd,
    int32_t data);

/* Usage uniquement dans callback ; bloc rempli par value_cmd */
MfClassicError mf_classic_poller_value_transfer(MfClassicPoller* instance, uint8_t block_num);

/* Usage uniquement dans callback ; fwt en cycles carrier */
MfClassicError mf_classic_poller_send_standard_frame(
    MfClassicPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt_fc);

/* Usage uniquement dans callback ; fwt en cycles carrier */
MfClassicError mf_classic_poller_send_frame(
    MfClassicPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt_fc);

/* Usage uniquement dans callback ; fwt en cycles carrier */
/* TX : parité fournie ; RX : données et parité reçues */
MfClassicError mf_classic_poller_send_custom_parity_frame(
    MfClassicPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt_fc);

/* Usage uniquement dans callback ; TX plaintext, RX déchiffré, fwt carrier */
MfClassicError mf_classic_poller_send_encrypted_frame(
    MfClassicPoller* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt_fc);

#ifdef __cplusplus
}
#endif
