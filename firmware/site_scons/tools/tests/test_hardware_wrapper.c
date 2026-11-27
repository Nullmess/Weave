/* Host-only HAL double: verify API contract, not the STM32 AES peripheral. */
#include "weave_security.h"
#include "furi_hal_crypto.h"
#include <openssl/evp.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

void weave_security_wipe(void* data, size_t size) {
    if(!data) return;
    volatile uint8_t* p=data;
    while(size--) *p++=0;
}
bool weave_security_equal(const uint8_t* a,const uint8_t* b,size_t n) {
    uint8_t v=0;
    for(size_t i=0;i<n;++i) v|=a[i]^b[i];
    return v==0;
}
static unsigned calls;
static bool force_error;
FuriHalCryptoGCMState furi_hal_crypto_gcm_encrypt_and_tag(const uint8_t* key,
    const uint8_t* iv,const uint8_t* aad,size_t aad_len,const uint8_t* input,
    uint8_t* output,size_t size,uint8_t* tag) {
    assert((uintptr_t)key%4==0);++calls;
    if(force_error){memset(output,0xa5,size);memset(tag,0xa5,16);return FuriHalCryptoGCMStateError;}
    EVP_CIPHER_CTX* c=EVP_CIPHER_CTX_new();int n=0,m=0;
    assert(c);
    assert(EVP_EncryptInit_ex(c,EVP_aes_256_gcm(),NULL,key,iv));
    if(aad_len)assert(EVP_EncryptUpdate(c,NULL,&n,aad,(int)aad_len));
    assert(EVP_EncryptUpdate(c,output,&n,input,(int)size));
    assert(EVP_EncryptFinal_ex(c,output+n,&m));
    assert(EVP_CIPHER_CTX_ctrl(c,EVP_CTRL_GCM_GET_TAG,16,tag));
    EVP_CIPHER_CTX_free(c);return FuriHalCryptoGCMStateOk;
}
FuriHalCryptoGCMState furi_hal_crypto_gcm_decrypt_and_verify(const uint8_t* key,
    const uint8_t* iv,const uint8_t* aad,size_t aad_len,const uint8_t* input,
    uint8_t* output,size_t size,const uint8_t* tag) {
    assert((uintptr_t)key%4==0);++calls;
    if(force_error){memset(output,0xa5,size);return FuriHalCryptoGCMStateError;}
    EVP_CIPHER_CTX* c=EVP_CIPHER_CTX_new();int n=0,m=0;
    assert(c);
    assert(EVP_DecryptInit_ex(c,EVP_aes_256_gcm(),NULL,key,iv));
    if(aad_len)assert(EVP_DecryptUpdate(c,NULL,&n,aad,(int)aad_len));
    assert(EVP_DecryptUpdate(c,output,&n,input,(int)size));
    assert(EVP_CIPHER_CTX_ctrl(c,EVP_CTRL_GCM_SET_TAG,16,(void*)tag));
    int ok=EVP_DecryptFinal_ex(c,output+n,&m);EVP_CIPHER_CTX_free(c);
    return ok?FuriHalCryptoGCMStateOk:FuriHalCryptoGCMStateAuthFailure;
}
int main(void) {
    assert(weave_security_aead_hw_selftest());
    _Alignas(uint32_t) uint8_t raw_key[33]={0};
    uint8_t nonce[12]={0},aad[16]={0},in[128],out[128],decoded[128],tag[16],zero[128]={0};
    memset(in,0x53,sizeof(in));memset(raw_key+1,0x34,32);
    const uint8_t* key=raw_key+1;
    for(size_t len=16;len<=128;len+=16){
        ++nonce[0];
        assert(weave_security_aead_encrypt_hw(key,nonce,aad,16,in,len,out,tag));
        assert(weave_security_aead_decrypt_hw(key,nonce,aad,16,out,len,tag,decoded));
        assert(!memcmp(in,decoded,len));
        tag[0]^=1;
        assert(!weave_security_aead_decrypt_hw(key,nonce,aad,16,out,len,tag,decoded));
        assert(!memcmp(decoded,zero,len));
    }
    unsigned before=calls;
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,15,in,16,out,tag));
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,16,in,15,out,tag));
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,16,in,0,out,tag));
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,16,in,65552,out,tag));
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,16,in,16,in+1,tag));
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,16,in,16,out,out+2));
    assert(calls==before);
    force_error=true;
    assert(!weave_security_aead_encrypt_hw(key,nonce,aad,16,in,16,out,tag));
    assert(!memcmp(out,zero,16)&&!memcmp(tag,zero,16));
    assert(!weave_security_aead_decrypt_hw(key,nonce,aad,16,in,16,tag,decoded));
    assert(!memcmp(decoded,zero,16));
    puts("Hardware wrapper host double: KAT, 8 message sizes, tag failure, bounds, overlap and error wiping PASS");
}
