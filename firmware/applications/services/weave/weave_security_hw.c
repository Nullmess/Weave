/* Explicit, bounded hardware AES-GCM. Software remains the default backend. */
#include "weave_security.h"
#include <furi_hal_crypto.h>
#include <string.h>

static bool overlaps(const void* a, size_t an, const void* b, size_t bn) {
    if(!an || !bn) return false;
    uintptr_t x = (uintptr_t)a, y = (uintptr_t)b;
    return x <= y ? y - x < an : x - y < bn;
}
static bool valid(const uint8_t* key, const uint8_t* nonce, const uint8_t* aad,
                  size_t aad_size, const uint8_t* in, size_t size,
                  uint8_t* out, const uint8_t* tag) {
    return key && nonce && tag && in && out && (aad || !aad_size) && size &&
           size <= 65536U && aad_size <= 65536U && !(size % 16U) && !(aad_size % 16U) &&
           !overlaps(out, size, in, size) && !overlaps(out, size, key, 32) &&
           !overlaps(out, size, nonce, 12) && !overlaps(out, size, aad, aad_size) &&
           !overlaps(out, size, tag, 16) && !overlaps(tag, 16, key, 32) &&
           !overlaps(tag, 16, nonce, 12) && !overlaps(tag, 16, aad, aad_size) &&
           !overlaps(tag, 16, in, size);
}
bool weave_security_aead_encrypt_hw(const uint8_t key[32], const uint8_t nonce[12],
    const uint8_t* aad, size_t aad_size, const uint8_t* plaintext, size_t size,
    uint8_t* ciphertext, uint8_t tag[16]) {
    if(!valid(key, nonce, aad, aad_size, plaintext, size, ciphertext, tag)) return false;
    uint32_t aligned_key[8]; memcpy(aligned_key, key, sizeof(aligned_key));
    bool ok = furi_hal_crypto_gcm_encrypt_and_tag((const uint8_t*)aligned_key, nonce,
        aad, aad_size, plaintext, ciphertext, size, tag) == FuriHalCryptoGCMStateOk;
    weave_security_wipe(aligned_key, sizeof(aligned_key));
    if(!ok) { weave_security_wipe(ciphertext, size); weave_security_wipe(tag, 16); }
    return ok;
}
bool weave_security_aead_decrypt_hw(const uint8_t key[32], const uint8_t nonce[12],
    const uint8_t* aad, size_t aad_size, const uint8_t* ciphertext, size_t size,
    const uint8_t tag[16], uint8_t* plaintext) {
    if(!valid(key, nonce, aad, aad_size, ciphertext, size, plaintext, tag)) return false;
    uint32_t aligned_key[8]; memcpy(aligned_key, key, sizeof(aligned_key));
    bool ok = furi_hal_crypto_gcm_decrypt_and_verify((const uint8_t*)aligned_key, nonce,
        aad, aad_size, ciphertext, plaintext, size, tag) == FuriHalCryptoGCMStateOk;
    weave_security_wipe(aligned_key, sizeof(aligned_key));
    if(!ok) weave_security_wipe(plaintext, size);
    return ok;
}
bool weave_security_aead_hw_selftest(void) {
    static const uint8_t expected[32] = {
        0xce,0xa7,0x40,0x3d,0x4d,0x60,0x6b,0x6e,0x07,0x4e,0xc5,0xd3,0xba,0xf3,0x9d,0x18,
        0xd0,0xd1,0xc8,0xa7,0x99,0x99,0x6b,0xf0,0x26,0x5b,0x98,0xb5,0xd4,0x8a,0xb9,0x19,
    };
    uint8_t key[32] = {0}, nonce[12] = {0}, input[16] = {0};
    uint8_t cipher[16], tag[16], decoded[16];
    bool ok = weave_security_aead_encrypt_hw(key, nonce, NULL, 0, input, 16, cipher, tag) &&
        weave_security_equal(cipher, expected, 16) && weave_security_equal(tag, expected+16, 16) &&
        weave_security_aead_decrypt_hw(key, nonce, NULL, 0, cipher, 16, tag, decoded) &&
        weave_security_equal(decoded, input, 16);
    if(ok) {
        static const uint8_t expected_aad[32] = {
            0xc5,0x54,0x71,0xf1,0x8e,0x45,0x8e,0x47,0xe0,0xc2,0x93,0xc4,0x08,0x11,0xd1,0x12,
            0x92,0xd0,0xb8,0x92,0xce,0x48,0x69,0x09,0xce,0x21,0x29,0xb7,0xc6,0x08,0xa6,0xb2,
        };
        uint8_t aad[16], zero[16] = {0};
        memset(key, 0x42, sizeof(key)); memset(nonce, 0x23, sizeof(nonce));
        memset(aad, 0x17, sizeof(aad)); memset(input, 0x91, sizeof(input));
        ok = weave_security_aead_encrypt_hw(key, nonce, aad, 16, input, 16, cipher, tag) &&
            weave_security_equal(cipher, expected_aad, 16) &&
            weave_security_equal(tag, expected_aad+16, 16) &&
            weave_security_aead_decrypt_hw(key, nonce, aad, 16, cipher, 16, tag, decoded) &&
            weave_security_equal(decoded, input, 16);
        if(ok) {
            tag[0] ^= 1;
            memset(decoded, 0xa5, sizeof(decoded));
            ok = !weave_security_aead_decrypt_hw(key, nonce, aad, 16, cipher, 16, tag, decoded) &&
                weave_security_equal(decoded, zero, 16);
        }
    }
    weave_security_wipe(decoded, sizeof(decoded));
    return ok;
}
