/* Crypto non audité ; données extractibles via debugger/code modifié */
/* AES : attaques théoriques possibles ; livraison des clés non sécurisée */
/* Enclave FUS sur core2 : core2 vivant requis */
/* Clé enclave chargée : registres de clé AES verrouillés */
/* Slot 0 : master réservé au provisioning */
/* Slots 1..10 : clés communes à tous les appareils */
/* Slot 11 unique, vide avant première utilisation ou provisioning */
/* Slots utilisateur append-only, disponibilité non garantie */
/* Slots utilisateur interdits aux applications publiques */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Slot master réservé au provisioning, jamais aux applications */
#define FURI_HAL_CRYPTO_ENCLAVE_MASTER_KEY_SLOT (0u)

/* Clés factory identiques sur tous les appareils */
#define FURI_HAL_CRYPTO_ENCLAVE_FACTORY_KEY_SLOT_START (1u)
#define FURI_HAL_CRYPTO_ENCLAVE_FACTORY_KEY_SLOT_END   (10u)

/* Clé unique créée au premier usage ; ensure_key requis */
#define FURI_HAL_CRYPTO_ENCLAVE_UNIQUE_KEY_SLOT (11u)

/* Slots utilisateur interdits aux applications publiques */
#define FURI_HAL_CRYPTO_ENCLAVE_USER_KEY_SLOT_START (12u)
#define FURI_HAL_CRYPTO_ENCLAVE_USER_KEY_SLOT_END   (100u)

/* Obsolète : indicateur crypto avancé supprimé avant v1.0 */
#define FURI_HAL_CRYPTO_ADVANCED_AVAIL 1

typedef enum {
    FuriHalCryptoKeyTypeMaster, /* Clé master */
    FuriHalCryptoKeyTypeSimple, /* Clé non chiffrée */
    FuriHalCryptoKeyTypeEncrypted, /* Chiffrement par la clé master */
} FuriHalCryptoKeyType;

/* Taille clé en bits */
typedef enum {
    FuriHalCryptoKeySize128,
    FuriHalCryptoKeySize256,
} FuriHalCryptoKeySize;

typedef struct {
    FuriHalCryptoKeyType type;
    FuriHalCryptoKeySize size;
    uint8_t* data;
} FuriHalCryptoKey;

typedef enum {
    FuriHalCryptoGCMStateOk,
    FuriHalCryptoGCMStateError,
    FuriHalCryptoGCMStateAuthFailure, /* Tags différents : authentification échouée */
} FuriHalCryptoGCMState;

void furi_hal_crypto_init(void);

/* Retour true si toutes les clés enclave intactes */
bool furi_hal_crypto_enclave_verify(uint8_t* keys_nb, uint8_t* valid_keys_nb);

/* Enclave séquentielle : génération des slots factory_end jusqu’au slot requis */
/* Clés générées par RNG hardware ; false si enclave corrompue */
bool furi_hal_crypto_enclave_ensure_key(uint8_t key_slot);

/* Retour true si succès ; slot écrit en sortie */
bool furi_hal_crypto_enclave_store_key(FuriHalCryptoKey* key, uint8_t* slot);

/* IV 16 bytes ; associer avec enclave_unload_key */
bool furi_hal_crypto_enclave_load_key(uint8_t slot, const uint8_t* iv);

/* Associer uniquement avec enclave_load_key */
bool furi_hal_crypto_enclave_unload_key(uint8_t slot);

/* Clé 32 bytes, IV 16 bytes ; associer avec unload_key */
bool furi_hal_crypto_load_key(const uint8_t* key, const uint8_t* iv);

/* Associer uniquement avec load_key */
bool furi_hal_crypto_unload_key(void);

/* Buffers en bytes ; retour true si succès */
bool furi_hal_crypto_encrypt(const uint8_t* input, uint8_t* output, size_t size);

/* Buffers en bytes ; retour true si succès */
bool furi_hal_crypto_decrypt(const uint8_t* input, uint8_t* output, size_t size);

/* AES-CTR : chiffrement/déchiffrement ; init/deinit AES internes */
/* Clé 32 bytes, IV 12 bytes ; longueurs en bytes */
bool furi_hal_crypto_ctr(
    const uint8_t* key,
    const uint8_t* iv,
    const uint8_t* input,
    uint8_t* output,
    size_t length);

/* GCM : clé 32 bytes, IV 12 bytes, tag 16 bytes */
/* Comparaison tag constante ; plaintext rejeté si tags différents */
/* Longueurs en bytes ; init/deinit AES internes */
bool furi_hal_crypto_gcm(
    const uint8_t* key,
    const uint8_t* iv,
    const uint8_t* aad,
    size_t aad_length,
    const uint8_t* input,
    uint8_t* output,
    size_t length,
    uint8_t* tag,
    bool decrypt);

/* GCM : clé 32 bytes, IV 12 bytes, tag 16 bytes */
/* Longueurs en bytes ; init/deinit AES internes */
FuriHalCryptoGCMState furi_hal_crypto_gcm_encrypt_and_tag(
    const uint8_t* key,
    const uint8_t* iv,
    const uint8_t* aad,
    size_t aad_length,
    const uint8_t* input,
    uint8_t* output,
    size_t length,
    uint8_t* tag);

/* GCM : clé 32 bytes, IV 12 bytes, tag 16 bytes */
/* Longueurs en bytes ; init/deinit AES internes */
/* Retour AuthFailure si tag incompatible */
FuriHalCryptoGCMState furi_hal_crypto_gcm_decrypt_and_verify(
    const uint8_t* key,
    const uint8_t* iv,
    const uint8_t* aad,
    size_t aad_length,
    const uint8_t* input,
    uint8_t* output,
    size_t length,
    const uint8_t* tag);

#ifdef __cplusplus
}
#endif
