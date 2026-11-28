#include <furi.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <furi_hal_usb.h>
#include <furi_hal_usb_weave.h>
#include <mbedtls/gcm.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <stdio.h>
#include <string.h>
#include "weave_platform.h"
#include "weave_permissions.h"
#include "weave_keystore.h"
#include "weave_internal.h"
#include "weave_usb.h"
#include "weave_vault_journal.h"

/* Strict host doubles. Never linked into firmware. */
static FuriMutex mutexes[4]; static unsigned mutex_count;
FuriMutex* furi_mutex_alloc(int type) { (void)type; assert(mutex_count<4); return &mutexes[mutex_count++]; }
int furi_mutex_acquire(FuriMutex* m,int wait) { (void)wait; assert(!m->held); m->held=1; return 0; }
int furi_mutex_release(FuriMutex* m) { assert(m->held); m->held=0; return 0; }
int32_t furi_kernel_lock(void) { return 0; }
void furi_kernel_unlock(void) {}
static bool memory_ok=true;
size_t memmgr_heap_get_max_free_block(void) { return memory_ok ? 100000U : 0U; }
static const char* appid="app-a"; static bool consent=true;
FuriThreadId furi_thread_get_current_id(void) { return (FuriThreadId)&appid; }
const char* furi_thread_get_appid(FuriThreadId id) { assert(id==&appid); return appid; }
struct DialogsApp { int dummy; }; struct DialogMessage { int dummy; };
static DialogsApp dialogs; static DialogMessage message; static unsigned prompts;
DialogMessage* dialog_message_alloc(void) { return &message; }
void dialog_message_free(DialogMessage* d) { assert(d==&message); }
void dialog_message_set_header(DialogMessage* d,const char* s,uint8_t x,uint8_t y,Align a,Align b) { assert(d==&message&&s); (void)x;(void)y;(void)a;(void)b; }
void dialog_message_set_text(DialogMessage* d,const char* s,uint8_t x,uint8_t y,Align a,Align b) { dialog_message_set_header(d,s,x,y,a,b); }
void dialog_message_set_buttons(DialogMessage* d,const char* a,const char* b,const char* c) { assert(d==&message&&a&&c); (void)b; }
DialogMessageButton dialog_message_show(DialogsApp* d,DialogMessage* m) { assert(d==&dialogs&&m==&message); prompts++; return consent?DialogMessageButtonRight:DialogMessageButtonLeft; }
void furi_hal_random_fill_buf(uint8_t* p,uint32_t n) { assert(RAND_bytes(p,(int)n)==1); }

struct Storage { int dummy; }; static Storage storage;
#define MAX_FILES 32
#define FILE_MAX 2300
struct Entry { char path[128]; uint8_t data[FILE_MAX]; size_t size; bool dir; };
static struct Entry entries[MAX_FILES];
struct File { struct Entry* entry; size_t offset; bool opened; };
static size_t write_limit=SIZE_MAX; static bool sync_ok=true; static bool stat_error=false;
static struct Entry* entry(const char* path,bool create) {
    struct Entry* empty=NULL;
    for(unsigned i=0;i<MAX_FILES;i++) {
        if(entries[i].path[0] && !strcmp(entries[i].path,path)) return &entries[i];
        if(!entries[i].path[0] && !empty) empty=&entries[i];
    }
    if(!create) return NULL;
    assert(empty && strlen(path)<sizeof(empty->path)); strcpy(empty->path,path); return empty;
}
void* furi_record_open(const char* record) { if(!strcmp(record,RECORD_DIALOGS))return &dialogs; assert(!strcmp(record,RECORD_STORAGE)); return &storage; }
void furi_record_close(const char* record) { assert(!strcmp(record,RECORD_STORAGE)||!strcmp(record,RECORD_DIALOGS)); }
File* storage_file_alloc(Storage* s) { assert(s==&storage); return calloc(1,sizeof(File)); }
void storage_file_free(File* f) { free(f); }
bool storage_file_open(File* f,const char* path,FS_AccessMode access,FS_OpenMode mode) {
    (void)access; f->entry=entry(path,mode==FSOM_CREATE_ALWAYS);
    if(!f->entry || f->entry->dir) return false;
    f->opened=true; f->offset=0; if(mode==FSOM_CREATE_ALWAYS) f->entry->size=0; return true;
}
bool storage_file_close(File* f) { assert(f->opened); f->opened=false; return true; }
bool storage_file_is_open(File* f) { return f->opened; }
size_t storage_file_read(File* f,void* out,size_t n) {
    assert(f->opened); if(n>f->entry->size-f->offset) n=f->entry->size-f->offset;
    memcpy(out,f->entry->data+f->offset,n); f->offset+=n; return n;
}
size_t storage_file_write(File* f,const void* in,size_t n) {
    assert(f->opened); if(n>write_limit)n=write_limit; assert(f->offset+n<=FILE_MAX);
    memcpy(f->entry->data+f->offset,in,n); f->offset+=n; f->entry->size=f->offset; return n;
}
bool storage_file_sync(File* f) { assert(f->opened); return sync_ok; }
FS_Error storage_common_stat(Storage* s,const char* path,FileInfo* info) {
    assert(s==&storage); if(stat_error) return FSE_NOT_READY;
    struct Entry* e=entry(path,false); if(!e)return FSE_NOT_EXIST;
    if(info){info->size=e->size;info->flags=e->dir?FSF_DIRECTORY:0;} return FSE_OK;
}
FS_Error storage_common_remove(Storage* s,const char* path) {
    assert(s==&storage); struct Entry* e=entry(path,false); if(!e)return FSE_NOT_EXIST;
    memset(e,0,sizeof(*e));return FSE_OK;
}
FS_Error storage_common_mkdir(Storage* s,const char* path) {
    assert(s==&storage); if(entry(path,false))return FSE_EXIST; entry(path,true)->dir=true;return FSE_OK;
}
bool file_info_is_dir(const FileInfo* info) { return (info->flags&FSF_DIRECTORY)!=0; }

/* Actual AES-GCM through OpenSSL, matching the mbedTLS calls in weave_security.c. */
void mbedtls_gcm_init(mbedtls_gcm_context* c){memset(c,0,sizeof(*c));}
void mbedtls_gcm_free(mbedtls_gcm_context* c){weave_security_wipe(c,sizeof(*c));}
int mbedtls_gcm_setkey(mbedtls_gcm_context* c,int alg,const uint8_t* k,unsigned n){assert(alg==1&&n==256);memcpy(c->key,k,32);return 0;}
int mbedtls_gcm_crypt_and_tag(mbedtls_gcm_context* c,int mode,size_t n,const uint8_t* nonce,size_t nn,
    const uint8_t* aad,size_t an,const uint8_t* in,uint8_t* out,size_t tn,uint8_t* tag){
    assert(mode==1&&nn==12&&tn==16); EVP_CIPHER_CTX* ctx=EVP_CIPHER_CTX_new(); int a=0,b=0;
    bool ok=ctx&&EVP_EncryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,c->key,nonce)==1 &&
        EVP_EncryptUpdate(ctx,NULL,&a,aad,(int)an)==1 && EVP_EncryptUpdate(ctx,out,&a,in,(int)n)==1 &&
        EVP_EncryptFinal_ex(ctx,out+a,&b)==1 && EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_GET_TAG,16,tag)==1;
    EVP_CIPHER_CTX_free(ctx); return ok?0:-1;
}
int mbedtls_gcm_auth_decrypt(mbedtls_gcm_context* c,size_t n,const uint8_t* nonce,size_t nn,
    const uint8_t* aad,size_t an,const uint8_t* tag,size_t tn,const uint8_t* in,uint8_t* out){
    assert(nn==12&&tn==16); EVP_CIPHER_CTX* ctx=EVP_CIPHER_CTX_new(); int a=0,b=0;
    bool ok=ctx&&EVP_DecryptInit_ex(ctx,EVP_aes_256_gcm(),NULL,c->key,nonce)==1 &&
        EVP_DecryptUpdate(ctx,NULL,&a,aad,(int)an)==1 && EVP_DecryptUpdate(ctx,out,&a,in,(int)n)==1 &&
        EVP_CIPHER_CTX_ctrl(ctx,EVP_CTRL_GCM_SET_TAG,16,(void*)tag)==1 && EVP_DecryptFinal_ex(ctx,out+a,&b)==1;
    EVP_CIPHER_CTX_free(ctx);return ok?0:-1;
}
/* Independent Ed25519 backend. Other private primitives deliberately fail:
 * this suite cannot establish their cryptographic correctness. */
bool weave_security_ed25519_public(const uint8_t* seed,size_t n,uint8_t* out,size_t m){
    EVP_PKEY* k=EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,NULL,seed,n);
    bool ok=k&&m==32&&EVP_PKEY_get_raw_public_key(k,out,&m)==1;EVP_PKEY_free(k);return ok;
}
bool weave_security_ed25519_generate(uint8_t* seed,size_t n,uint8_t* out,size_t m){return RAND_bytes(seed,(int)n)==1&&weave_security_ed25519_public(seed,n,out,m);}
bool weave_security_ed25519_sign(const uint8_t* seed,size_t n,const uint8_t* in,size_t len,uint8_t* sig,size_t m){
    EVP_PKEY* k=EVP_PKEY_new_raw_private_key(EVP_PKEY_ED25519,NULL,seed,n);EVP_MD_CTX* c=EVP_MD_CTX_new();
    bool ok=k&&c&&EVP_DigestSignInit(c,NULL,NULL,NULL,k)==1&&EVP_DigestSign(c,sig,&m,in,len)==1;
    EVP_MD_CTX_free(c);EVP_PKEY_free(k);return ok;
}
size_t weave_security_digest_size(WeaveDigest d){return d==WeaveDigestSha256?32:d==WeaveDigestSha384?48:d==WeaveDigestSha512?64:0;}
#define UNUSED(x) (void)(x)
bool weave_security_ec_generate(WeaveCurve a,uint8_t*b,size_t c,uint8_t*d,size_t e){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);return false;}
bool weave_security_ec_public(WeaveCurve a,const uint8_t*b,size_t c,uint8_t*d,size_t e){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);return false;}
bool weave_security_ecdsa_sign(WeaveCurve a,const uint8_t*b,size_t c,const uint8_t*d,size_t e,uint8_t*f,size_t g){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);UNUSED(f);UNUSED(g);return false;}
bool weave_security_ecdh(WeaveCurve a,const uint8_t*b,size_t c,const uint8_t*d,size_t e,uint8_t*f,size_t g){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);UNUSED(f);UNUSED(g);return false;}
bool weave_security_rsa_generate(uint16_t a,uint8_t*b,size_t c,uint8_t*d,size_t e){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);return false;}
bool weave_security_rsa_public(const uint8_t*a,size_t b,uint8_t*c,size_t d){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);return false;}
bool weave_security_rsa_sign(bool a,WeaveDigest b,const uint8_t*c,size_t d,const uint8_t*e,size_t f,uint8_t*g,size_t h){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);UNUSED(f);UNUSED(g);UNUSED(h);return false;}
bool weave_security_rsa_oaep_decrypt(const uint8_t*a,size_t b,const uint8_t*c,size_t d,const uint8_t*e,size_t f,uint8_t*g,size_t h,size_t*i){UNUSED(a);UNUSED(b);UNUSED(c);UNUSED(d);UNUSED(e);UNUSED(f);UNUSED(g);UNUSED(h);UNUSED(i);return false;}

FuriHalUsbInterface usb_cdc_single,usb_cdc_dual,usb_weave_hid,usb_weave_ccid,usb_weave_composite;
static FuriHalUsbInterface* current=&usb_cdc_single;
static bool restore_ok=true,acquire_ok=true,prepared=false;
FuriHalUsbInterface* furi_hal_usb_get_config(void){return current;}
bool furi_hal_usb_acquire_config(FuriHalUsbInterface* e,FuriHalUsbInterface* n,void* c){UNUSED(c);assert(e==current);if(!acquire_ok)return false;current=n;return true;}
bool furi_hal_usb_release_config(FuriHalUsbInterface* e,FuriHalUsbInterface* n){assert(e==current);if(!restore_ok)return false;current=n;return true;}
bool weave_usb_buffers_prepare(void){assert(!prepared);prepared=true;return true;}
void weave_usb_buffers_release(void){assert(prepared);prepared=false;}
bool furi_hal_weave_usb_configure_hid(const uint8_t* d,size_t n,uint8_t r){UNUSED(d);UNUSED(n);UNUSED(r);return true;}
bool furi_hal_weave_usb_connected(void){return prepared;}
bool furi_hal_weave_usb_receive(FuriHalWeaveUsbChannel a,uint8_t*b,size_t c,size_t*d){UNUSED(a);UNUSED(b);UNUSED(c);*d=0;return false;}
bool furi_hal_weave_usb_transmit(FuriHalWeaveUsbChannel a,const uint8_t*b,size_t c){UNUSED(a);UNUSED(b);UNUSED(c);return true;}
bool furi_hal_weave_usb_notify_card(bool p){UNUSED(p);return true;}
static WeaveTransport gateway=WeaveTransportBle;
WeaveTransport weave_get_transport(Weave* w){assert(w);return gateway;}
extern void weave_security_init(void);
extern void weave_permissions_init(void);
/* BLE service is tested separately; this fixture models only USB. */
bool weave_ble_release_app(const char* id) {(void)id;return true;}
bool weave_nfc_release_app(const char* id) {(void)id;return true;}
extern void weave_usb_broker_init(void);
static void test_usb(void){
    weave_usb_broker_init();weave_permissions_init();int owner=0; uint8_t buf[64]={0};size_t size=0;
    assert(weave_platform_capabilities()==2559);
    assert(!(weave_platform_capabilities() & (WEAVE_CAP_HARDWARE_ANTI_ROLLBACK | WEAVE_CAP_MEMORY_ISOLATION)));
    assert(weave_platform_state((Weave*)&owner)==(WEAVE_STATE_GATEWAY_BLE|WEAVE_STATE_USB_CDC_AVAILABLE));
    appid="weave";consent=false;assert(!weave_permission_check(WeavePermissionUsb));
    appid="bad/id";assert(!weave_permission_check(WeavePermissionUsb));
    appid="app-a";consent=false;assert(weave_usb_acquire(&owner,WeaveUsbModeHid)==WeaveUsbDenied&&!prepared);
    consent=true;acquire_ok=false;assert(weave_usb_acquire(&owner,WeaveUsbModeHid)==WeaveUsbBusy&&!prepared);
    acquire_ok=true;assert(weave_usb_acquire(&owner,WeaveUsbModeHid)==WeaveUsbOk&&prepared);
    assert(weave_usb_acquire(&owner,WeaveUsbModeHid)==WeaveUsbOk);
    assert(weave_usb_acquire(&owner,WeaveUsbModeCcid)==WeaveUsbBusy);
    appid="app-b";assert(weave_usb_acquire(&owner,WeaveUsbModeHid)==WeaveUsbBusy);
    assert(weave_usb_receive(&owner,WeaveUsbChannelHid,buf,sizeof(buf),&size)==WeaveUsbNotOwner);
    assert(weave_usb_transmit(&owner,WeaveUsbChannelHid,buf,sizeof(buf))==WeaveUsbNotOwner);
    assert(weave_usb_release(&owner)==WeaveUsbNotOwner&&!weave_usb_connected(&owner));
    assert(weave_usb_release_app("app-b")&&prepared);
    appid="app-a";assert(weave_usb_connected(&owner));restore_ok=false;
    assert(!weave_usb_release_app("app-a"));assert(!weave_usb_connected(&owner));
    assert(weave_usb_acquire(&owner,WeaveUsbModeHid)==WeaveUsbBusy);
    restore_ok=true;appid="app-b";assert(weave_usb_acquire(&owner,WeaveUsbModeCcid)==WeaveUsbOk);
    assert(weave_permission_has(WeavePermissionUsb));
    weave_permission_revoke_self();assert(!prepared&&current==&usb_cdc_single);
    assert(!weave_permission_has(WeavePermissionUsb));
    assert(weave_usb_release_app("app-b")&&!prepared&&current==&usb_cdc_single);
    appid="app-a";assert(weave_permission_has(WeavePermissionUsb));
    puts("USB: permission, token+app ownership, failed switch, orphan retry, loader cleanup; capabilities PASS");
}
static void test_journal(void){
    uint8_t first[60],second[60],out[WEAVE_VAULT_RECORD_MAX];memset(first,0x11,sizeof(first));memset(second,0x22,sizeof(second));size_t n=0;
    const char* path="/journal";
    assert(weave_vault_write(&storage,path,first,sizeof(first),false)==WeaveSecurityOk);
    for(size_t cut=0;cut<108;cut++){
        write_limit=cut;assert(weave_vault_write(&storage,path,second,sizeof(second),false)==WeaveSecurityStorageError);
        assert(weave_vault_read(&storage,path,out,&n)==WeaveSecurityOk&&n==sizeof(first)&&!memcmp(out,first,n));
    }
    write_limit=SIZE_MAX;assert(weave_vault_write(&storage,path,second,sizeof(second),false)==WeaveSecurityOk);
    assert(weave_vault_read(&storage,path,out,&n)==WeaveSecurityOk&&!memcmp(out,second,n));
    entry("/journal.1",false)->data[20]^=1;assert(weave_vault_read(&storage,path,out,&n)==WeaveSecurityOk&&!memcmp(out,first,n));
    stat_error=true;assert(weave_vault_read(&storage,path,out,&n)==WeaveSecurityStorageError);stat_error=false;
    assert(weave_vault_remove(&storage,path)==WeaveSecurityOk);
    assert(weave_vault_read(&storage,path,out,&n)==WeaveSecurityNotFound);
    struct Entry* legacy=entry("/legacy.bak",true);memcpy(legacy->data,first,sizeof(first));legacy->size=sizeof(first);
    assert(weave_vault_read(&storage,"/legacy",out,&n)==WeaveSecurityOk&&!memcmp(out,first,n));
    write_limit=50;assert(weave_vault_write(&storage,"/legacy",second,sizeof(second),false)==WeaveSecurityStorageError);
    assert(weave_vault_read(&storage,"/legacy",out,&n)==WeaveSecurityOk&&!memcmp(out,first,n));
    write_limit=SIZE_MAX;assert(weave_vault_write(&storage,"/legacy",second,sizeof(second),false)==WeaveSecurityOk);
    assert(!entry("/legacy.bak",false));assert(weave_vault_remove(&storage,"/legacy")==WeaveSecurityOk);
    memory_ok=false;assert(weave_vault_read(&storage,path,out,&n)==WeaveSecurityStorageError);memory_ok=true;
    puts("Journal: all 108 torn-write lengths, legacy migration, backup recovery, corruption, I/O, memory pressure PASS");
}
static void legacy_path(const char* ns,const char* name,uint8_t aad[162],size_t* aad_len,char path[128]) {
    size_t a=strlen(appid)+1,b=strlen(ns)+1,c=strlen(name)+1;
    memcpy(aad,appid,a);memcpy(aad+a,ns,b);memcpy(aad+a+b,name,c);*aad_len=a+b+c;
    uint8_t hash[32];assert(weave_security_sha256(aad,*aad_len,hash));
    strcpy(path,"/ext/.weave/vault/");size_t base=strlen(path);
    for(size_t i=0;i<sizeof(hash);i++) snprintf(path+base+2*i,3,"%02x",hash[i]);
}
static void test_legacy_authenticated(void) {
    const uint8_t pw[]="legacy password", old[]="old secret", new_value[]="new secret", wrong[]="wrong";
    uint8_t aad[162], key[32], out[64];size_t an=0,n=0;char path[128],slot_path[128];
    legacy_path("legacy","item",aad,&an,path);
    struct Entry* legacy=entry(path,true);uint8_t* r=legacy->data;
    memcpy(r,"WVS1",4);r[4]=0;r[5]=0;r[6]=0x4e;r[7]=0x20;
    memset(r+8,0x67,28);
    assert(weave_security_pbkdf2_sha256(pw,sizeof(pw),r+8,16,20000,key,sizeof(key)));
    assert(weave_security_aead_encrypt(key,r+24,aad,an,old,sizeof(old),r+52,r+36));
    weave_security_wipe(key,sizeof(key));legacy->size=52+sizeof(old);
    assert(weave_secret_load("legacy","item",pw,sizeof(pw),out,sizeof(out),&n)==WeaveSecurityOk&&n==sizeof(old)&&!memcmp(out,old,n));
    assert(weave_secret_store("legacy","item",pw,sizeof(pw),old,sizeof(old))==WeaveSecurityOk);
    assert(!entry(path,false));
    assert(weave_secret_store("legacy","item",pw,sizeof(pw),new_value,sizeof(new_value))==WeaveSecurityOk);
    assert(weave_secret_load("legacy","item",pw,sizeof(pw),out,sizeof(out),&n)==WeaveSecurityOk&&n==sizeof(new_value)&&!memcmp(out,new_value,n));
    /* A complete envelope with bad GCM must fail, not return the old secret. */
    assert(snprintf(slot_path,sizeof(slot_path),"%s.1",path)<(int)sizeof(slot_path));
    struct Entry* newer=entry(slot_path,false);assert(newer&&newer->size>68);
    newer->data[16+52]^=1;
    assert(weave_security_sha256(newer->data,newer->size-32,newer->data+newer->size-32));
    assert(weave_secret_load("legacy","item",pw,sizeof(pw),out,sizeof(out),&n)==WeaveSecurityAuthenticationFailed&&n==0);
    assert(weave_secret_load("legacy","item",wrong,sizeof(wrong),out,sizeof(out),&n)==WeaveSecurityAuthenticationFailed&&n==0);
    assert(weave_secret_delete("legacy","item")==WeaveSecurityOk);
    puts("Legacy WVS1 authenticated read/migration; complete but tampered GCM rejected without fallback: PASS");
}
static void test_vault_keystore(void){
    weave_security_init();const uint8_t pw[]="strong-test-passphrase",msg[]="signed message",wrong[]="wrong";
    uint8_t pub[32],pub2[32],sig[64],out[80];size_t n=0;
    assert(weave_secret_store("ns","same",pw,sizeof(pw),msg,sizeof(msg))==WeaveSecurityOk);
    assert(weave_keystore_generate("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub,sizeof(pub))==WeaveSecurityOk);
    assert(weave_keystore_generate("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub2,sizeof(pub2))==WeaveSecurityInvalid);
    assert(weave_keystore_public("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub2,sizeof(pub2))==WeaveSecurityOk&&!memcmp(pub,pub2,32));
    assert(weave_secret_load("ns","same",pw,sizeof(pw),out,sizeof(out),&n)==WeaveSecurityOk&&n==sizeof(msg)&&!memcmp(out,msg,n));
    assert(weave_keystore_sign("ns","same",WeaveKeyEd25519,pw,sizeof(pw),false,0,msg,sizeof(msg),sig,sizeof(sig))==WeaveSecurityOk);
    EVP_PKEY* k=EVP_PKEY_new_raw_public_key(EVP_PKEY_ED25519,NULL,pub,sizeof(pub));EVP_MD_CTX* c=EVP_MD_CTX_new();
    assert(k&&c&&EVP_DigestVerifyInit(c,NULL,NULL,NULL,k)==1&&EVP_DigestVerify(c,sig,sizeof(sig),msg,sizeof(msg))==1);
    EVP_MD_CTX_free(c);EVP_PKEY_free(k);
    memset(sig,0xaa,sizeof(sig));assert(weave_keystore_sign("ns","same",WeaveKeyEd25519,wrong,sizeof(wrong),false,0,msg,sizeof(msg),sig,sizeof(sig))==WeaveSecurityAuthenticationFailed);
    for(unsigned i=0;i<sizeof(sig);i++)assert(sig[i]==0);
    assert(weave_keystore_sign("ns","same",WeaveKeyEd25519,pw,sizeof(pw),false,WeaveDigestSha256,msg,sizeof(msg),sig,sizeof(sig))==WeaveSecurityInvalid);
    assert(weave_keystore_derive("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub,sizeof(pub),out,32)==WeaveSecurityInvalid);
    assert(weave_keystore_public("ns","same",WeaveKeyX25519,pw,sizeof(pw),pub2,sizeof(pub2))==WeaveSecurityInvalid);
    appid="app-b";assert(weave_keystore_public("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub2,sizeof(pub2))==WeaveSecurityNotFound);appid="app-a";
    weave_permission_revoke_self();consent=false;assert(weave_keystore_public("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub2,sizeof(pub2))==WeaveSecurityDenied);consent=true;
    assert(weave_keystore_delete("ns","same")==WeaveSecurityOk);
    assert(weave_keystore_public("ns","same",WeaveKeyEd25519,pw,sizeof(pw),pub2,sizeof(pub2))==WeaveSecurityNotFound);
    assert(weave_secret_load("ns","same",pw,sizeof(pw),out,sizeof(out),&n)==WeaveSecurityOk);
    assert(weave_secret_delete("ns","same")==WeaveSecurityOk);
    test_legacy_authenticated();
    weave_permission_revoke_self();appid="app-b";weave_permission_revoke_self();appid="app-a";
    puts("Vault/keystore: AES-GCM, Ed25519 verified, label collision, purpose, wrong password, app isolation, consent, deletion PASS");
}
int main(void){test_usb();test_journal();test_vault_keystore();return 0;}
