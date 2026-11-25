/* RFC 8032 via Monocypher 4.0.3 (BSD-2-Clause OR CC0-1.0).
 * No persistent state. Monocypher destroys the seed passed to key_pair,
 * so always pass a temporary copy. Only Weave wrappers are exported.
 */
#include "weave_security.h"
#include <monocypher-ed25519.h>
#include <string.h>

static const uint8_t empty_message[1] = {0};

static void ed_keypair(const uint8_t* seed, uint8_t* secret, uint8_t* pub) {
    uint8_t copy[WEAVE_ED25519_SEED_SIZE];
    memcpy(copy, seed, sizeof(copy));
    crypto_ed25519_key_pair(secret, pub, copy);
    weave_security_wipe(copy, sizeof(copy));
}

bool weave_security_ed25519_public(
    const uint8_t* seed, size_t seed_size, uint8_t* public_key, size_t public_size) {
    if(!seed || seed_size != WEAVE_ED25519_SEED_SIZE || !public_key ||
       public_size != WEAVE_ED25519_PUBLIC_SIZE) return false;
    uint8_t secret[64];
    ed_keypair(seed, secret, public_key);
    weave_security_wipe(secret, sizeof(secret));
    return true;
}

bool weave_security_ed25519_generate(
    uint8_t* seed, size_t seed_size, uint8_t* public_key, size_t public_size) {
    if(!seed || seed_size != WEAVE_ED25519_SEED_SIZE || !public_key ||
       public_size != WEAVE_ED25519_PUBLIC_SIZE) return false;
    if(!weave_security_random(seed, seed_size)) {
        weave_security_wipe(seed, seed_size);
        weave_security_wipe(public_key, public_size);
        return false;
    }
    return weave_security_ed25519_public(seed, seed_size, public_key, public_size);
}

bool weave_security_ed25519_sign(
    const uint8_t* seed, size_t seed_size, const uint8_t* message,
    size_t message_size, uint8_t* signature, size_t signature_size) {
    if(!seed || seed_size != WEAVE_ED25519_SEED_SIZE || (!message && message_size) ||
       !signature || signature_size != WEAVE_ED25519_SIGNATURE_SIZE) return false;
    uint8_t secret[64], pub[WEAVE_ED25519_PUBLIC_SIZE];
    ed_keypair(seed, secret, pub);
    crypto_ed25519_sign(signature, secret, message ? message : empty_message, message_size);
    weave_security_wipe(secret, sizeof(secret));
    return true;
}

bool weave_security_ed25519_verify(
    const uint8_t* public_key, size_t public_size, const uint8_t* message,
    size_t message_size, const uint8_t* signature, size_t signature_size) {
    if(!public_key || public_size != WEAVE_ED25519_PUBLIC_SIZE || (!message && message_size) ||
       !signature || signature_size != WEAVE_ED25519_SIGNATURE_SIZE) return false;
    return crypto_ed25519_check(signature, public_key,
                                message ? message : empty_message, message_size) == 0;
}

bool weave_security_ed25519ph_sign(
    const uint8_t* seed, size_t seed_size, const uint8_t* digest,
    size_t digest_size, uint8_t* signature, size_t signature_size) {
    if(!seed || seed_size != WEAVE_ED25519_SEED_SIZE || !digest || digest_size != 64 ||
       !signature || signature_size != WEAVE_ED25519_SIGNATURE_SIZE) return false;
    uint8_t secret[64], pub[WEAVE_ED25519_PUBLIC_SIZE];
    ed_keypair(seed, secret, pub);
    crypto_ed25519_ph_sign(signature, secret, digest);
    weave_security_wipe(secret, sizeof(secret));
    return true;
}

bool weave_security_ed25519ph_verify(
    const uint8_t* public_key, size_t public_size, const uint8_t* digest,
    size_t digest_size, const uint8_t* signature, size_t signature_size) {
    if(!public_key || public_size != WEAVE_ED25519_PUBLIC_SIZE || !digest || digest_size != 64 ||
       !signature || signature_size != WEAVE_ED25519_SIGNATURE_SIZE) return false;
    return crypto_ed25519_ph_check(signature, public_key, digest) == 0;
}
