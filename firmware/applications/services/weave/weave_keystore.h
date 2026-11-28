/** Software keystore, API v1. Keys are addressed by application-local labels.
 * No private-key export operation is provided. This is NOT a secure element,
 * hardware-backed key isolation, or a defense against malicious native code.
 * Use a strong passphrase. All operations are local and synchronous; run on
 * a suitably sized worker stack. Passwords and input/output must not overlap.
 * The FAP is responsible for user-presence and operation-level authorization.
 * Do not race deletion against use of the same key. */
#pragma once
#include "weave_security.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WEAVE_KEYSTORE_API_VERSION 1U
typedef enum {
    WeaveKeyP256Sign = 1,
    WeaveKeyP384Sign,
    WeaveKeyEd25519,
    WeaveKeyRsa2048Sign,
    WeaveKeyRsa3072Sign,
    WeaveKeyP256Derive,
    WeaveKeyP384Derive,
    WeaveKeyX25519,
    WeaveKeyRsa2048Oaep,
    WeaveKeyRsa3072Oaep,
} WeaveKeyAlgorithm;
/* Generate only: an existing label is never overwritten. namespace/name max
 * 48 bytes each. Public output sizes and formats match Security v4. */
WeaveSecurityResult weave_keystore_generate(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* password, size_t password_size,
    uint8_t* public_key, size_t public_size);
WeaveSecurityResult weave_keystore_public(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* password, size_t password_size,
    uint8_t* public_key, size_t public_size);
/* ECDSA and RSA sign a digest, Ed25519 signs the original message. For Ed25519,
 * digest must be 0 and pss false. SHA-1 is not permitted. RSA supports PSS and
 * PKCS#1 v1.5 signatures; ECDSA requires pss=false. */
WeaveSecurityResult weave_keystore_sign(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* password, size_t password_size,
    bool pss, WeaveDigest digest, const uint8_t* input, size_t input_size,
    uint8_t* signature, size_t signature_size);
/* Only derive-purpose keys. Raw ECDH shared secret; apply a protocol KDF. */
WeaveSecurityResult weave_keystore_derive(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* password, size_t password_size,
    const uint8_t* peer, size_t peer_size, uint8_t* secret, size_t secret_size);
/* Only OAEP-purpose keys. SHA-256/MGF1-SHA-256. */
WeaveSecurityResult weave_keystore_decrypt_oaep(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* password, size_t password_size,
    const uint8_t* label, size_t label_size, const uint8_t* input, size_t input_size,
    uint8_t* output, size_t capacity, size_t* output_size);
/* Delete requires the application's Keystore consent (not a password).
 * This removes the encrypted files, not a guaranteed physical media erase. */
WeaveSecurityResult weave_keystore_delete(const char* ns, const char* name);
#ifdef __cplusplus
}
#endif
