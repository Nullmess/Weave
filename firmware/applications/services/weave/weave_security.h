/** Weave security primitives and password-protected secret vault for FAPs.
 * API v3. All secrets remain on the Flipper; no RPC/companion is involved.
 * Vault keys are scoped to the calling Furi app ID and require consent.
 * User-facing FAPs must collect a sufficiently strong passphrase securely.
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WEAVE_SECURITY_API_VERSION 3U
#define WEAVE_SECURITY_KEY_SIZE 32U
#define WEAVE_SECURITY_SALT_SIZE 16U
#define WEAVE_SECURITY_NONCE_SIZE 12U
#define WEAVE_SECURITY_TAG_SIZE 16U
#define WEAVE_SECRET_MAX_SIZE 2048U

typedef enum {
    WeaveSecurityOk = 0,
    WeaveSecurityInvalid,
    WeaveSecurityCryptoError,
    WeaveSecurityStorageError,
    WeaveSecurityNotFound,
    WeaveSecurityAuthenticationFailed,
    WeaveSecurityBufferTooSmall,
    WeaveSecurityDenied,
} WeaveSecurityResult;

/* No RNG state is exported; furi_hal_random supplies entropy. */
bool weave_security_random(uint8_t* out, size_t size);
void weave_security_wipe(void* data, size_t size);
bool weave_security_equal(const uint8_t* a, const uint8_t* b, size_t size);
bool weave_security_sha256(const uint8_t* data, size_t size, uint8_t out[32]);
bool weave_security_hmac_sha256(
    const uint8_t* key, size_t key_len, const uint8_t* data, size_t size, uint8_t out[32]);
/* RFC 5869 HKDF SHA-256. salt and info may be NULL only if their size is zero. */
bool weave_security_hkdf_sha256(
    const uint8_t* key, size_t key_size, const uint8_t* salt, size_t salt_size,
    const uint8_t* info, size_t info_size, uint8_t* out, size_t out_size);
/* PBKDF2 HMAC SHA-256. Caller chooses iterations >= 10000. */
bool weave_security_pbkdf2_sha256(
    const uint8_t* password, size_t password_size, const uint8_t* salt,
    size_t salt_size, uint32_t iterations, uint8_t* out, size_t out_size);
/* AES-256-GCM. Never reuse a nonce with the same key. Plaintext output is
 * wiped on authentication failure; output must not overlap input. */
bool weave_security_aead_encrypt(
    const uint8_t key[32], const uint8_t nonce[12], const uint8_t* aad,
    size_t aad_size, const uint8_t* plaintext, size_t size, uint8_t* ciphertext,
    uint8_t tag[16]);
bool weave_security_aead_decrypt(
    const uint8_t key[32], const uint8_t nonce[12], const uint8_t* aad,
    size_t aad_size, const uint8_t* ciphertext, size_t size,
    const uint8_t tag[16], uint8_t* plaintext);
/* Generic crypto, API v2. All operations are synchronous and allocate only
 * while called. Run expensive key operations in a suitably sized worker stack.
 * Output buffers must not overlap any input or each other. SHA-1 is for legacy
 * interoperability (e.g. HOTP), not for new signatures. */
typedef enum {
    WeaveDigestSha1 = 1,
    WeaveDigestSha256 = 2,
    WeaveDigestSha384 = 3,
    WeaveDigestSha512 = 4,
} WeaveDigest;

size_t weave_security_digest_size(WeaveDigest digest);
bool weave_security_digest(
    WeaveDigest digest, const uint8_t* data, size_t size, uint8_t* out, size_t out_size);
bool weave_security_hmac(
    WeaveDigest digest, const uint8_t* key, size_t key_size, const uint8_t* data,
    size_t size, uint8_t* out, size_t out_size);

/* Raw CBC, no padding or authentication; use only when the enclosing protocol
 * defines integrity/padding (e.g. CTAP PIN/UV). IV is 16 bytes and is unchanged.
 * Key length 16, 24 or 32 bytes. Input/output sizes must be equal multiples of 16. */
bool weave_security_aes_cbc(
    bool encrypt, const uint8_t* key, size_t key_size, const uint8_t* iv,
    const uint8_t* input, size_t size, uint8_t* output, size_t output_size);

/* One raw AES block, only for protocol-defined operations (e.g. PIV challenge). */
bool weave_security_aes_ecb(
    bool encrypt, const uint8_t* key, size_t key_size,
    const uint8_t* input, uint8_t* output);

typedef enum {
    WeaveCurveP256 = 1,
    WeaveCurveP384 = 2,
    WeaveCurveX25519 = 3,
} WeaveCurve;

/* P-256/P-384 private scalar is big-endian (32/48 bytes), public key is SEC1
 * uncompressed (65/97 bytes); ECDSA signatures are fixed-width r||s (64/96).
 * X25519 uses RFC 7748 little-endian 32-byte keys and shared secret. X25519
 * cannot sign. ECDH returns raw secret: caller must apply protocol KDF.
 * All output capacities below must equal the indicated encoding sizes. */
bool weave_security_ec_generate(
    WeaveCurve curve, uint8_t* private_key, size_t private_size,
    uint8_t* public_key, size_t public_size);
bool weave_security_ec_public(
    WeaveCurve curve, const uint8_t* private_key, size_t private_size,
    uint8_t* public_key, size_t public_size);
bool weave_security_ecdsa_sign(
    WeaveCurve curve, const uint8_t* private_key, size_t private_size,
    const uint8_t* hash, size_t hash_size, uint8_t* signature, size_t signature_size);
bool weave_security_ecdsa_verify(
    WeaveCurve curve, const uint8_t* public_key, size_t public_size,
    const uint8_t* hash, size_t hash_size, const uint8_t* signature, size_t signature_size);
bool weave_security_ecdh(
    WeaveCurve curve, const uint8_t* private_key, size_t private_size,
    const uint8_t* peer_key, size_t peer_size, uint8_t* secret, size_t secret_size);

/* RSA 2048/3072, exponent 65537. The private format is fixed-width P||Q
 * (each half the modulus width); public format is the big-endian modulus.
 * Key generation is expensive. Use a worker, not the GUI or NFC callback.
 * PKCS#1 v1.5 or PSS signatures over SHA-256/384/512 digests. PSS uses the
 * same hash for MGF1, salt length equal to hash length. */
bool weave_security_rsa_generate(
    uint16_t bits, uint8_t* private_key, size_t private_size,
    uint8_t* public_key, size_t public_size);
bool weave_security_rsa_public(
    const uint8_t* private_key, size_t private_size, uint8_t* public_key, size_t public_size);
bool weave_security_rsa_sign(
    bool pss, WeaveDigest digest, const uint8_t* private_key, size_t private_size,
    const uint8_t* hash, size_t hash_size, uint8_t* signature, size_t signature_size);
bool weave_security_rsa_verify(
    bool pss, WeaveDigest digest, const uint8_t* public_key, size_t public_size,
    const uint8_t* hash, size_t hash_size, const uint8_t* signature, size_t signature_size);
/* Raw blinded RSA private primitive for protocols where the host constructs
 * the encoded block (e.g. PIV GENERAL AUTHENTICATE). Not a signature or
 * encryption scheme by itself. The caller must authorize each use, enforce
 * key purpose and protocol padding rules, and prevent decryption oracles.
 * Input/output are exactly the modulus width, and must not overlap. */
bool weave_security_rsa_private_block(
    const uint8_t* private_key, size_t private_size, const uint8_t* input,
    size_t input_size, uint8_t* output, size_t output_size);
/* RSA-OAEP (SHA-256, MGF1-SHA-256). Optional label, no PKCS#1 v1.5 decryption.
 * Decrypt failure must not be exposed as a padding oracle by the caller. */
bool weave_security_rsa_oaep_encrypt(
    const uint8_t* public_key, size_t public_size, const uint8_t* label, size_t label_size,
    const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size);
bool weave_security_rsa_oaep_decrypt(
    const uint8_t* private_key, size_t private_size, const uint8_t* label, size_t label_size,
    const uint8_t* input, size_t input_size, uint8_t* output, size_t output_capacity,
    size_t* output_size);

/* RFC 8032 Ed25519: 32-byte private seed, 32-byte public key, 64-byte
 * signature. Pure Ed25519 signs the original message, not a digest.
 * Ed25519ph is a separate domain and takes exactly a SHA-512 digest.
 * Buffers must not overlap. Empty messages may use NULL with size zero.
 * The seed is never modified; callers must wipe/store it securely.
 * Synchronous operations: use a suitably sized worker stack, and authorize
 * signing in the enclosing application. Verification uses Monocypher's
 * RFC 8032 cofactored equation; use public keys enrolled by a trusted process.
 */
#define WEAVE_ED25519_SEED_SIZE 32U
#define WEAVE_ED25519_PUBLIC_SIZE 32U
#define WEAVE_ED25519_SIGNATURE_SIZE 64U

bool weave_security_ed25519_generate(
    uint8_t* seed, size_t seed_size, uint8_t* public_key, size_t public_size);
bool weave_security_ed25519_public(
    const uint8_t* seed, size_t seed_size, uint8_t* public_key, size_t public_size);
bool weave_security_ed25519_sign(
    const uint8_t* seed, size_t seed_size, const uint8_t* message,
    size_t message_size, uint8_t* signature, size_t signature_size);
bool weave_security_ed25519_verify(
    const uint8_t* public_key, size_t public_size, const uint8_t* message,
    size_t message_size, const uint8_t* signature, size_t signature_size);
bool weave_security_ed25519ph_sign(
    const uint8_t* seed, size_t seed_size, const uint8_t* digest,
    size_t digest_size, uint8_t* signature, size_t signature_size);
bool weave_security_ed25519ph_verify(
    const uint8_t* public_key, size_t public_size, const uint8_t* digest,
    size_t digest_size, const uint8_t* signature, size_t signature_size);

/* Each stored value is salted, separately encrypted, and authenticated.
 * Namespace and name are authenticated as associated data. Max 2048 bytes.
 * Password is never stored. Wrong password/tampering have the same error.
 * Namespace is an application-local label, not an OS sandbox.
 * Existing step-2 vault entries use a legacy unscoped format and are not
 * automatically accessible; no destructive migration is attempted.
 * Untrusted/low-entropy passwords are vulnerable to offline dictionary attacks.
 */
WeaveSecurityResult weave_secret_store(
    const char* namespace_id, const char* name, const uint8_t* password,
    size_t password_size, const uint8_t* value, size_t value_size);
WeaveSecurityResult weave_secret_load(
    const char* namespace_id, const char* name, const uint8_t* password,
    size_t password_size, uint8_t* output, size_t capacity, size_t* output_size);
WeaveSecurityResult weave_secret_delete(const char* namespace_id, const char* name);
#ifdef __cplusplus
}
#endif
