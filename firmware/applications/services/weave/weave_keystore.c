#include "weave_keystore.h"
#include "weave_internal.h"
#include "weave_permissions.h"
#include <string.h>

#define KEY_HEADER 5U
#define KEY_RECORD_MAX (KEY_HEADER + 384U)
typedef enum { KeySign, KeyDerive, KeyOaep } KeyPurpose;
typedef struct { size_t private_size, public_size, signature_size; WeaveCurve curve; KeyPurpose purpose; bool ed, rsa; } KeySpec;
static bool spec(WeaveKeyAlgorithm algorithm, KeySpec* s) {
    switch(algorithm) {
    case WeaveKeyP256Sign: *s = (KeySpec){32,65,64,WeaveCurveP256,KeySign,false,false}; break;
    case WeaveKeyP384Sign: *s = (KeySpec){48,97,96,WeaveCurveP384,KeySign,false,false}; break;
    case WeaveKeyEd25519: *s = (KeySpec){32,32,64,0,KeySign,true,false}; break;
    case WeaveKeyRsa2048Sign: *s = (KeySpec){256,256,256,0,KeySign,false,true}; break;
    case WeaveKeyRsa3072Sign: *s = (KeySpec){384,384,384,0,KeySign,false,true}; break;
    case WeaveKeyP256Derive: *s = (KeySpec){32,65,0,WeaveCurveP256,KeyDerive,false,false}; break;
    case WeaveKeyP384Derive: *s = (KeySpec){48,97,0,WeaveCurveP384,KeyDerive,false,false}; break;
    case WeaveKeyX25519: *s = (KeySpec){32,32,0,WeaveCurveX25519,KeyDerive,false,false}; break;
    case WeaveKeyRsa2048Oaep: *s = (KeySpec){256,256,0,0,KeyOaep,false,true}; break;
    case WeaveKeyRsa3072Oaep: *s = (KeySpec){384,384,0,0,KeyOaep,false,true}; break;
    default: return false;
    }
    return true;
}
static bool names_valid(const char* ns, const char* name, const uint8_t* pw, size_t pw_size) {
    return ns && name && *ns && *name && strlen(ns) <= 48U && strlen(name) <= 48U && pw && pw_size;
}
static WeaveSecurityResult load(const char* ns, const char* name, WeaveKeyAlgorithm algorithm,
    const uint8_t* pw, size_t pw_size, const KeySpec* s, uint8_t record[KEY_RECORD_MAX]) {
    size_t n = 0;
    WeaveSecurityResult result = weave_vault_load_internal(true, ns, name, pw, pw_size,
        record, KEY_RECORD_MAX, &n);
    if(result == WeaveSecurityOk && (n != KEY_HEADER + s->private_size ||
       memcmp(record, "WKS1", 4) || record[4] != (uint8_t)algorithm))
        result = WeaveSecurityInvalid;
    return result;
}
static WeaveSecurityResult finish(WeaveSecurityResult result, uint8_t record[KEY_RECORD_MAX],
    uint8_t* out, size_t out_size) {
    weave_security_wipe(record, KEY_RECORD_MAX);
    if(result != WeaveSecurityOk && out) weave_security_wipe(out, out_size);
    return result;
}
WeaveSecurityResult weave_keystore_generate(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* pw, size_t pw_size,
    uint8_t* public_key, size_t public_size) {
    KeySpec s;
    if(!spec(algorithm, &s) || !names_valid(ns, name, pw, pw_size) ||
       !public_key || public_size != s.public_size) return WeaveSecurityInvalid;
    if(!weave_permission_check(WeavePermissionKeystore)) return WeaveSecurityDenied;
    uint8_t record[KEY_RECORD_MAX] = {0};
    memcpy(record, "WKS1", 4); record[4] = (uint8_t)algorithm;
    uint8_t* key = record + KEY_HEADER;
    bool ok = s.ed ? weave_security_ed25519_generate(key,s.private_size,public_key,public_size) :
        s.rsa ? weave_security_rsa_generate((uint16_t)(s.private_size*8),key,s.private_size,public_key,public_size) :
        weave_security_ec_generate(s.curve,key,s.private_size,public_key,public_size);
    WeaveSecurityResult result = ok ? weave_vault_store_internal(true,true,ns,name,pw,pw_size,
        record,KEY_HEADER+s.private_size) : WeaveSecurityCryptoError;
    return finish(result,record,public_key,public_size);
}
WeaveSecurityResult weave_keystore_public(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* pw, size_t pw_size,
    uint8_t* public_key, size_t public_size) {
    KeySpec s;
    if(!spec(algorithm, &s) || !names_valid(ns,name,pw,pw_size) ||
       !public_key || public_size != s.public_size) return WeaveSecurityInvalid;
    uint8_t record[KEY_RECORD_MAX] = {0};
    WeaveSecurityResult result = load(ns,name,algorithm,pw,pw_size,&s,record);
    if(result == WeaveSecurityOk) {
        const uint8_t* key = record + KEY_HEADER;
        bool ok = s.ed ? weave_security_ed25519_public(key,s.private_size,public_key,public_size) :
            s.rsa ? weave_security_rsa_public(key,s.private_size,public_key,public_size) :
            weave_security_ec_public(s.curve,key,s.private_size,public_key,public_size);
        if(!ok) result = WeaveSecurityCryptoError;
    }
    return finish(result,record,public_key,public_size);
}
WeaveSecurityResult weave_keystore_sign(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* pw, size_t pw_size,
    bool pss, WeaveDigest digest, const uint8_t* input, size_t input_size,
    uint8_t* signature, size_t signature_size) {
    KeySpec s;
    if(!spec(algorithm,&s) || s.purpose != KeySign || !names_valid(ns,name,pw,pw_size) ||
       (!input && input_size) || !signature || signature_size != s.signature_size)
        return WeaveSecurityInvalid;
    if(s.ed ? (pss || digest != 0) :
       ((!s.rsa && pss) || (digest != WeaveDigestSha256 && digest != WeaveDigestSha384 &&
         digest != WeaveDigestSha512) || input_size != weave_security_digest_size(digest)))
        return WeaveSecurityInvalid;
    uint8_t record[KEY_RECORD_MAX] = {0};
    WeaveSecurityResult result = load(ns,name,algorithm,pw,pw_size,&s,record);
    if(result == WeaveSecurityOk) {
        const uint8_t* key = record + KEY_HEADER;
        bool ok = s.ed ? weave_security_ed25519_sign(key,s.private_size,input,input_size,signature,signature_size) :
            s.rsa ? weave_security_rsa_sign(pss,digest,key,s.private_size,input,input_size,signature,signature_size) :
            weave_security_ecdsa_sign(s.curve,key,s.private_size,input,input_size,signature,signature_size);
        if(!ok) result = WeaveSecurityCryptoError;
    }
    return finish(result,record,signature,signature_size);
}
WeaveSecurityResult weave_keystore_derive(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* pw, size_t pw_size,
    const uint8_t* peer, size_t peer_size, uint8_t* secret, size_t secret_size) {
    KeySpec s;
    if(!spec(algorithm,&s) || s.purpose != KeyDerive || !names_valid(ns,name,pw,pw_size) ||
       !peer || peer_size != s.public_size || !secret || secret_size != s.private_size)
        return WeaveSecurityInvalid;
    uint8_t record[KEY_RECORD_MAX] = {0};
    WeaveSecurityResult result = load(ns,name,algorithm,pw,pw_size,&s,record);
    if(result == WeaveSecurityOk && !weave_security_ecdh(s.curve,record+KEY_HEADER,s.private_size,
        peer,peer_size,secret,secret_size)) result = WeaveSecurityCryptoError;
    return finish(result,record,secret,secret_size);
}
WeaveSecurityResult weave_keystore_decrypt_oaep(const char* ns, const char* name,
    WeaveKeyAlgorithm algorithm, const uint8_t* pw, size_t pw_size,
    const uint8_t* label, size_t label_size, const uint8_t* input, size_t input_size,
    uint8_t* output, size_t capacity, size_t* output_size) {
    if(output_size) *output_size = 0;
    KeySpec s;
    if(!spec(algorithm,&s) || s.purpose != KeyOaep || !names_valid(ns,name,pw,pw_size) ||
       (!label && label_size) || !input || input_size != s.public_size ||
       !output || !output_size || !capacity) return WeaveSecurityInvalid;
    uint8_t record[KEY_RECORD_MAX] = {0};
    WeaveSecurityResult result = load(ns,name,algorithm,pw,pw_size,&s,record);
    if(result == WeaveSecurityOk && !weave_security_rsa_oaep_decrypt(record+KEY_HEADER,s.private_size,
        label,label_size,input,input_size,output,capacity,output_size)) result = WeaveSecurityCryptoError;
    if(result != WeaveSecurityOk) *output_size = 0;
    return finish(result,record,output,capacity);
}
WeaveSecurityResult weave_keystore_delete(const char* ns, const char* name) {
    return weave_vault_delete_internal(true,ns,name);
}
