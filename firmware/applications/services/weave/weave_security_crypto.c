/* Generic crypto wrappers: no resident context or new worker. */
#include "weave_security.h"
#include <mbedtls/aes.h>
#include <mbedtls/md.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/rsa.h>
#include <string.h>

static const uint8_t empty[1] = {0};

static mbedtls_md_type_t digest_type(WeaveDigest digest) {
    switch(digest) {
    case WeaveDigestSha1: return MBEDTLS_MD_SHA1;
    case WeaveDigestSha256: return MBEDTLS_MD_SHA256;
    case WeaveDigestSha384: return MBEDTLS_MD_SHA384;
    case WeaveDigestSha512: return MBEDTLS_MD_SHA512;
    default: return MBEDTLS_MD_NONE;
    }
}

size_t weave_security_digest_size(WeaveDigest digest) {
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(digest_type(digest));
    return info ? mbedtls_md_get_size(info) : 0;
}

bool weave_security_digest(
    WeaveDigest digest, const uint8_t* data, size_t size, uint8_t* out, size_t out_size) {
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(digest_type(digest));
    if(!info || !out || (!data && size) || out_size != mbedtls_md_get_size(info)) return false;
    if(digest == WeaveDigestSha256) return weave_security_sha256(data, size, out);
    bool ok = mbedtls_md(info, data ? data : empty, size, out) == 0;
    if(!ok) weave_security_wipe(out, out_size);
    return ok;
}

bool weave_security_hmac(
    WeaveDigest digest, const uint8_t* key, size_t key_size, const uint8_t* data,
    size_t size, uint8_t* out, size_t out_size) {
    const mbedtls_md_info_t* info = mbedtls_md_info_from_type(digest_type(digest));
    if(!info || !out || (!key && key_size) || (!data && size) ||
       out_size != mbedtls_md_get_size(info)) return false;
    if(digest == WeaveDigestSha256) return weave_security_hmac_sha256(key, key_size, data, size, out);
    bool ok = mbedtls_md_hmac(info, key ? key : empty, key_size,
                              data ? data : empty, size, out) == 0;
    if(!ok) weave_security_wipe(out, out_size);
    return ok;
}

bool weave_security_aes_cbc(
    bool encrypt, const uint8_t* key, size_t key_size, const uint8_t* iv,
    const uint8_t* input, size_t size, uint8_t* output, size_t output_size) {
    if(!key || !iv || !input || !output || !size || size % 16 || size != output_size ||
       (key_size != 16 && key_size != 24 && key_size != 32)) return false;
    uint8_t working_iv[16];
    memcpy(working_iv, iv, sizeof(working_iv));
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    unsigned int bits = (unsigned int)(8 * key_size);
    int rc = encrypt ? mbedtls_aes_setkey_enc(&ctx, key, bits) :
                       mbedtls_aes_setkey_dec(&ctx, key, bits);
    bool ok = rc == 0 && mbedtls_aes_crypt_cbc(&ctx, encrypt ? MBEDTLS_AES_ENCRYPT :
        MBEDTLS_AES_DECRYPT, size, working_iv, input, output) == 0;
    mbedtls_aes_free(&ctx);
    weave_security_wipe(working_iv, sizeof(working_iv));
    if(!ok) weave_security_wipe(output, output_size);
    return ok;
}

bool weave_security_aes_ecb(
    bool encrypt, const uint8_t* key, size_t key_size,
    const uint8_t* input, uint8_t* output) {
    if(!key || !input || !output || (key_size != 16 && key_size != 24 && key_size != 32))
        return false;
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    unsigned int bits = (unsigned int)(8 * key_size);
    int rc = encrypt ? mbedtls_aes_setkey_enc(&ctx, key, bits) :
                       mbedtls_aes_setkey_dec(&ctx, key, bits);
    bool ok = rc == 0 && mbedtls_aes_crypt_ecb(&ctx, encrypt ? MBEDTLS_AES_ENCRYPT :
        MBEDTLS_AES_DECRYPT, input, output) == 0;
    mbedtls_aes_free(&ctx);
    if(!ok) weave_security_wipe(output, 16);
    return ok;
}

static int crypto_rng(void* context, unsigned char* out, size_t size) {
    (void)context;
    return weave_security_random(out, size) ? 0 : -1;
}

static mbedtls_ecp_group_id curve_id(WeaveCurve curve) {
    switch(curve) {
    case WeaveCurveP256: return MBEDTLS_ECP_DP_SECP256R1;
    case WeaveCurveP384: return MBEDTLS_ECP_DP_SECP384R1;
    case WeaveCurveX25519: return MBEDTLS_ECP_DP_CURVE25519;
    default: return MBEDTLS_ECP_DP_NONE;
    }
}

static size_t curve_width(WeaveCurve curve) {
    switch(curve) {
    case WeaveCurveP256:
    case WeaveCurveX25519: return 32;
    case WeaveCurveP384: return 48;
    default: return 0;
    }
}

static size_t public_width(WeaveCurve curve) {
    size_t n = curve_width(curve);
    return !n ? 0 : curve == WeaveCurveX25519 ? n : 2 * n + 1;
}

static bool read_private(
    WeaveCurve curve, mbedtls_ecp_group* group, mbedtls_mpi* d,
    const uint8_t* key, size_t n) {
    if(curve == WeaveCurveX25519) {
        /* RFC 7748 scalar decoding (clamping). */
        uint8_t scalar[32];
        memcpy(scalar, key, sizeof(scalar));
        scalar[0] &= 248;
        scalar[31] &= 127;
        scalar[31] |= 64;
        int rc = mbedtls_mpi_read_binary_le(d, scalar, sizeof(scalar));
        weave_security_wipe(scalar, sizeof(scalar));
        if(rc) return false;
    } else if(mbedtls_mpi_read_binary(d, key, n) != 0) return false;
    return mbedtls_ecp_check_privkey(group, d) == 0;
}

static bool read_public(
    WeaveCurve curve, mbedtls_ecp_group* group, mbedtls_ecp_point* q,
    const uint8_t* key, size_t n) {
    if(curve != WeaveCurveX25519 && key[0] != 4) return false;
    return mbedtls_ecp_point_read_binary(group, q, key, n) == 0 &&
           mbedtls_ecp_check_pubkey(group, q) == 0;
}

static bool write_public(
    mbedtls_ecp_group* group, mbedtls_ecp_point* q, uint8_t* key, size_t n) {
    size_t written = 0;
    return mbedtls_ecp_point_write_binary(group, q, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                          &written, key, n) == 0 && written == n;
}

bool weave_security_ec_generate(
    WeaveCurve curve, uint8_t* private_key, size_t private_size,
    uint8_t* public_key, size_t public_size) {
    size_t n = curve_width(curve);
    if(!n || !private_key || !public_key || private_size != n ||
       public_size != public_width(curve)) return false;
    mbedtls_ecp_group group;
    mbedtls_ecp_point q;
    mbedtls_mpi d;
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&q);
    mbedtls_mpi_init(&d);
    bool ok = mbedtls_ecp_group_load(&group, curve_id(curve)) == 0 &&
              mbedtls_ecp_gen_keypair(&group, &d, &q, crypto_rng, NULL) == 0;
    if(ok) {
        int rc = curve == WeaveCurveX25519 ? mbedtls_mpi_write_binary_le(&d, private_key, n) :
                                           mbedtls_mpi_write_binary(&d, private_key, n);
        ok = rc == 0 && write_public(&group, &q, public_key, public_size);
    }
    mbedtls_mpi_free(&d);
    mbedtls_ecp_point_free(&q);
    mbedtls_ecp_group_free(&group);
    if(!ok) {
        weave_security_wipe(private_key, private_size);
        weave_security_wipe(public_key, public_size);
    }
    return ok;
}

bool weave_security_ec_public(
    WeaveCurve curve, const uint8_t* private_key, size_t private_size,
    uint8_t* public_key, size_t public_size) {
    size_t n = curve_width(curve);
    if(!n || !private_key || !public_key || private_size != n ||
       public_size != public_width(curve)) return false;
    mbedtls_ecp_keypair key;
    mbedtls_ecp_keypair_init(&key);
    size_t written = 0;
    bool ok = mbedtls_ecp_read_key(curve_id(curve), &key, private_key, n) == 0 &&
              mbedtls_ecp_keypair_calc_public(&key, crypto_rng, NULL) == 0 &&
              mbedtls_ecp_write_public_key(&key, MBEDTLS_ECP_PF_UNCOMPRESSED,
                                           &written, public_key, public_size) == 0 &&
              written == public_size;
    mbedtls_ecp_keypair_free(&key);
    if(!ok) weave_security_wipe(public_key, public_size);
    return ok;
}

static bool signature_hash_size(size_t n) {
    return n == 32 || n == 48 || n == 64;
}

bool weave_security_ecdsa_sign(
    WeaveCurve curve, const uint8_t* private_key, size_t private_size,
    const uint8_t* hash, size_t hash_size, uint8_t* signature, size_t signature_size) {
    size_t n = curve_width(curve);
    if(!n || curve == WeaveCurveX25519 || !private_key || !hash || !signature ||
       private_size != n || signature_size != 2 * n || !signature_hash_size(hash_size)) return false;
    mbedtls_ecp_group group;
    mbedtls_mpi d, r, s;
    mbedtls_ecp_group_init(&group);
    mbedtls_mpi_init(&d);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    bool ok = mbedtls_ecp_group_load(&group, curve_id(curve)) == 0 &&
              read_private(curve, &group, &d, private_key, n) &&
              mbedtls_ecdsa_sign(&group, &r, &s, &d, hash, hash_size, crypto_rng, NULL) == 0 &&
              mbedtls_mpi_write_binary(&r, signature, n) == 0 &&
              mbedtls_mpi_write_binary(&s, signature + n, n) == 0;
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_group_free(&group);
    if(!ok) weave_security_wipe(signature, signature_size);
    return ok;
}

bool weave_security_ecdsa_verify(
    WeaveCurve curve, const uint8_t* public_key, size_t public_size,
    const uint8_t* hash, size_t hash_size, const uint8_t* signature, size_t signature_size) {
    size_t n = curve_width(curve);
    if(!n || curve == WeaveCurveX25519 || !public_key || !hash || !signature ||
       public_size != public_width(curve) || signature_size != 2 * n ||
       !signature_hash_size(hash_size)) return false;
    mbedtls_ecp_group group;
    mbedtls_ecp_point q;
    mbedtls_mpi r, s;
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&q);
    mbedtls_mpi_init(&r);
    mbedtls_mpi_init(&s);
    bool ok = mbedtls_ecp_group_load(&group, curve_id(curve)) == 0 &&
              read_public(curve, &group, &q, public_key, public_size) &&
              mbedtls_mpi_read_binary(&r, signature, n) == 0 &&
              mbedtls_mpi_read_binary(&s, signature + n, n) == 0 &&
              mbedtls_ecdsa_verify(&group, hash, hash_size, &q, &r, &s) == 0;
    mbedtls_mpi_free(&s);
    mbedtls_mpi_free(&r);
    mbedtls_ecp_point_free(&q);
    mbedtls_ecp_group_free(&group);
    return ok;
}

bool weave_security_ecdh(
    WeaveCurve curve, const uint8_t* private_key, size_t private_size,
    const uint8_t* peer_key, size_t peer_size, uint8_t* secret, size_t secret_size) {
    size_t n = curve_width(curve);
    if(!n || !private_key || !peer_key || !secret || private_size != n ||
       secret_size != n || peer_size != public_width(curve)) return false;
    mbedtls_ecp_group group;
    mbedtls_ecp_point q;
    mbedtls_mpi d, z;
    mbedtls_ecp_group_init(&group);
    mbedtls_ecp_point_init(&q);
    mbedtls_mpi_init(&d);
    mbedtls_mpi_init(&z);
    bool ok = mbedtls_ecp_group_load(&group, curve_id(curve)) == 0 &&
              read_private(curve, &group, &d, private_key, n) &&
              read_public(curve, &group, &q, peer_key, peer_size) &&
              mbedtls_ecdh_compute_shared(&group, &z, &q, &d, crypto_rng, NULL) == 0 &&
              mbedtls_mpi_cmp_int(&z, 0) != 0;
    if(ok) ok = (curve == WeaveCurveX25519 ? mbedtls_mpi_write_binary_le(&z, secret, n) :
                                             mbedtls_mpi_write_binary(&z, secret, n)) == 0;
    mbedtls_mpi_free(&z);
    mbedtls_mpi_free(&d);
    mbedtls_ecp_point_free(&q);
    mbedtls_ecp_group_free(&group);
    if(!ok) weave_security_wipe(secret, secret_size);
    return ok;
}

static bool rsa_width(size_t n) { return n == 256 || n == 384; }
static const uint8_t rsa_e[] = {1, 0, 1};

static bool rsa_load_private(mbedtls_rsa_context* ctx, const uint8_t* key, size_t n) {
    return mbedtls_rsa_import_raw(ctx, NULL, 0, key, n / 2, key + n / 2, n / 2,
                                   NULL, 0, rsa_e, sizeof(rsa_e)) == 0 &&
           mbedtls_rsa_complete(ctx) == 0 && mbedtls_rsa_get_len(ctx) == n &&
           mbedtls_rsa_check_privkey(ctx) == 0;
}

static bool rsa_load_public(mbedtls_rsa_context* ctx, const uint8_t* key, size_t n) {
    return mbedtls_rsa_import_raw(ctx, key, n, NULL, 0, NULL, 0, NULL, 0,
                                   rsa_e, sizeof(rsa_e)) == 0 &&
           mbedtls_rsa_complete(ctx) == 0 && mbedtls_rsa_get_len(ctx) == n &&
           mbedtls_rsa_check_pubkey(ctx) == 0;
}

bool weave_security_rsa_generate(
    uint16_t bits, uint8_t* private_key, size_t private_size,
    uint8_t* public_key, size_t public_size) {
    size_t n = bits / 8;
    if((bits != 2048 && bits != 3072) || !private_key || !public_key ||
       private_size != n || public_size != n) return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    bool ok = mbedtls_rsa_gen_key(&ctx, crypto_rng, NULL, bits, 65537) == 0 &&
              mbedtls_rsa_export_raw(&ctx, public_key, n, private_key, n / 2,
                  private_key + n / 2, n / 2, NULL, 0, NULL, 0) == 0;
    mbedtls_rsa_free(&ctx);
    if(!ok) {
        weave_security_wipe(private_key, private_size);
        weave_security_wipe(public_key, public_size);
    }
    return ok;
}

bool weave_security_rsa_public(
    const uint8_t* private_key, size_t private_size, uint8_t* public_key, size_t public_size) {
    if(!private_key || !public_key || !rsa_width(private_size) || public_size != private_size)
        return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    bool ok = rsa_load_private(&ctx, private_key, private_size) &&
              mbedtls_rsa_export_raw(&ctx, public_key, public_size, NULL, 0,
                                     NULL, 0, NULL, 0, NULL, 0) == 0;
    mbedtls_rsa_free(&ctx);
    if(!ok) weave_security_wipe(public_key, public_size);
    return ok;
}

static bool rsa_digest_valid(WeaveDigest digest, size_t n) {
    return digest != WeaveDigestSha1 && signature_hash_size(n) &&
           weave_security_digest_size(digest) == n;
}

bool weave_security_rsa_sign(
    bool pss, WeaveDigest digest, const uint8_t* private_key, size_t private_size,
    const uint8_t* hash, size_t hash_size, uint8_t* signature, size_t signature_size) {
    if(!private_key || !hash || !signature || !rsa_width(private_size) ||
       signature_size != private_size || !rsa_digest_valid(digest, hash_size)) return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    mbedtls_md_type_t md = digest_type(digest);
    bool ok = rsa_load_private(&ctx, private_key, private_size) &&
              mbedtls_rsa_set_padding(&ctx, pss ? MBEDTLS_RSA_PKCS_V21 : MBEDTLS_RSA_PKCS_V15, md) == 0;
    if(ok) {
        int rc = pss ? mbedtls_rsa_rsassa_pss_sign_ext(&ctx, crypto_rng, NULL, md,
                         (unsigned int)hash_size, hash, (int)hash_size, signature) :
                       mbedtls_rsa_pkcs1_sign(&ctx, crypto_rng, NULL, md,
                         (unsigned int)hash_size, hash, signature);
        ok = rc == 0;
    }
    mbedtls_rsa_free(&ctx);
    if(!ok) weave_security_wipe(signature, signature_size);
    return ok;
}

bool weave_security_rsa_verify(
    bool pss, WeaveDigest digest, const uint8_t* public_key, size_t public_size,
    const uint8_t* hash, size_t hash_size, const uint8_t* signature, size_t signature_size) {
    if(!public_key || !hash || !signature || !rsa_width(public_size) ||
       signature_size != public_size || !rsa_digest_valid(digest, hash_size)) return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    mbedtls_md_type_t md = digest_type(digest);
    bool ok = rsa_load_public(&ctx, public_key, public_size) &&
              mbedtls_rsa_set_padding(&ctx, pss ? MBEDTLS_RSA_PKCS_V21 : MBEDTLS_RSA_PKCS_V15, md) == 0;
    if(ok) {
        int rc = pss ? mbedtls_rsa_rsassa_pss_verify_ext(&ctx, md, (unsigned int)hash_size,
                          hash, md, (int)hash_size, signature) :
                       mbedtls_rsa_pkcs1_verify(&ctx, md, (unsigned int)hash_size, hash, signature);
        ok = rc == 0;
    }
    mbedtls_rsa_free(&ctx);
    return ok;
}

bool weave_security_rsa_private_block(
    const uint8_t* private_key, size_t private_size, const uint8_t* input,
    size_t input_size, uint8_t* output, size_t output_size) {
    if(!private_key || !input || !output || !rsa_width(private_size) ||
       input_size != private_size || output_size != private_size || input == output) return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    bool ok = rsa_load_private(&ctx, private_key, private_size) &&
              mbedtls_rsa_private(&ctx, crypto_rng, NULL, input, output) == 0;
    mbedtls_rsa_free(&ctx);
    if(!ok) weave_security_wipe(output, output_size);
    return ok;
}

bool weave_security_rsa_oaep_encrypt(
    const uint8_t* public_key, size_t public_size, const uint8_t* label, size_t label_size,
    const uint8_t* input, size_t input_size, uint8_t* output, size_t output_size) {
    if(!public_key || !rsa_width(public_size) || (!label && label_size) ||
       (!input && input_size) || input_size > public_size - 66 || !output ||
       output_size != public_size) return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    bool ok = rsa_load_public(&ctx, public_key, public_size) &&
        mbedtls_rsa_set_padding(&ctx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256) == 0 &&
        mbedtls_rsa_rsaes_oaep_encrypt(&ctx, crypto_rng, NULL, label ? label : empty,
                                      label_size, input_size, input ? input : empty, output) == 0;
    mbedtls_rsa_free(&ctx);
    if(!ok) weave_security_wipe(output, output_size);
    return ok;
}

bool weave_security_rsa_oaep_decrypt(
    const uint8_t* private_key, size_t private_size, const uint8_t* label, size_t label_size,
    const uint8_t* input, size_t input_size, uint8_t* output, size_t output_capacity,
    size_t* output_size) {
    if(!output_size) return false;
    *output_size = 0;
    if(!private_key || !rsa_width(private_size) || (!label && label_size) ||
       !input || input_size != private_size || !output || !output_capacity) return false;
    mbedtls_rsa_context ctx;
    mbedtls_rsa_init(&ctx);
    bool ok = rsa_load_private(&ctx, private_key, private_size) &&
        mbedtls_rsa_set_padding(&ctx, MBEDTLS_RSA_PKCS_V21, MBEDTLS_MD_SHA256) == 0 &&
        mbedtls_rsa_rsaes_oaep_decrypt(&ctx, crypto_rng, NULL, label ? label : empty,
                                      label_size, output_size, input, output, output_capacity) == 0;
    mbedtls_rsa_free(&ctx);
    if(!ok) {
        *output_size = 0;
        weave_security_wipe(output, output_capacity);
    }
    return ok;
}
