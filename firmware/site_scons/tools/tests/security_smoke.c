/* Generic Weave API regression FAP. Public vectors only; no persistent data. */
#include <furi.h>
#include <dialogs/dialogs.h>
#include <weave/weave_security.h>
#include <string.h>

static const uint8_t sha_expected[32] = {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};
static const uint8_t hmac_expected[32] = {0x9c, 0x19, 0x6e, 0x32, 0xdc, 0x01, 0x75, 0xf8, 0x6f, 0x4b, 0x1c, 0xb8, 0x92, 0x89, 0xd6, 0x61, 0x9d, 0xe6, 0xbe, 0xe6, 0x99, 0xe4, 0xc3, 0x78, 0xe6, 0x83, 0x09, 0xed, 0x97, 0xa1, 0xa6, 0xab};
static const uint8_t kdf_expected[32] = {0x5e, 0xc0, 0x2b, 0x91, 0xa4, 0xb5, 0x9c, 0x6f, 0x59, 0xdd, 0x5f, 0xbe, 0x4c, 0xa6, 0x49, 0xec, 0xe4, 0xfa, 0x85, 0x68, 0xcd, 0xb8, 0xba, 0x36, 0xcf, 0x41, 0x42, 0x6e, 0x88, 0x05, 0x52, 0x2b};
static const uint8_t input[] = "abc", key[] = "key", password[] = "password", salt[] = "salt";

static bool zeroed(const void* ptr, size_t len) {
    const uint8_t* bytes = ptr;
    uint8_t value = 0;
    for(size_t i = 0; i < len; ++i) value |= bytes[i];
    return value == 0;
}

static const char* run_legacy(void) {
    uint8_t out[32];
    if(!weave_security_sha256(input, 3, out) || !weave_security_equal(out, sha_expected, 32))
        return "SHA-256";
    if(!weave_security_hmac_sha256(key, 3, input, 3, out) ||
       !weave_security_equal(out, hmac_expected, 32)) return "HMAC";
    if(!weave_security_pbkdf2_sha256(password, 8, salt, 4, 10000, out, 32) ||
       !weave_security_equal(out, kdf_expected, 32)) return "PBKDF2";
    uint8_t aes_key[32] = {0}, nonce[12] = {0}, plain[16] = {0}, cipher[16], tag[16];
    if(!weave_security_aead_encrypt(aes_key, nonce, NULL, 0, plain, 16, cipher, tag) ||
       !weave_security_aead_decrypt(aes_key, nonce, NULL, 0, cipher, 16, tag, out) ||
       !zeroed(out, 16)) return "AES-GCM";
    tag[0] ^= 1;
    memset(out, 0xa5, 16);
    if(weave_security_aead_decrypt(aes_key, nonce, NULL, 0, cipher, 16, tag, out) ||
       !zeroed(out, 16)) return "GCM reject";
    weave_security_wipe(out, sizeof(out));
    return NULL;
}

#if SMOKE_V4
static const char* run_v4(void) {
    if(weave_security_api_version() != 4) return "API version";
    WeaveSecurityHmacSha256Context ctx;
    uint8_t out[32];
    if(!weave_security_hmac_sha256_init(&ctx, key, 3)) return "HMAC init";
    bool ok = weave_security_hmac_sha256_compute(&ctx, input, 3, out) &&
        weave_security_equal(out, hmac_expected, 32);
    weave_security_hmac_sha256_clear(&ctx);
    if(!ok || !zeroed(&ctx, sizeof(ctx))) return "HMAC context";
    WeaveSecurityJob job;
    if(!weave_security_job_begin_pbkdf2(&job, password, 8, salt, 4, 10000, 32))
        return "Job init";
    uint64_t done = 0, total = 0, previous = 0;
    WeaveSecurityJobState state;
    do {
        state = weave_security_job_step(&job, 128);
        if(!weave_security_job_progress(&job, &done, &total) || total != 10000 ||
           done <= previous || done - previous > 128) {
            weave_security_job_cancel(&job); return "Job progress";
        }
        previous = done;
        furi_delay_tick(1);
    } while(state == WeaveSecurityJobRunning);
    ok = state == WeaveSecurityJobDone && done == total &&
         weave_security_job_result(&job, out, 32) &&
         weave_security_equal(out, kdf_expected, 32) && zeroed(&job, sizeof(job));
    weave_security_job_cancel(&job);
    if(!ok) return "Job result";
    if(!weave_security_job_begin_sha256(&job, input, 3)) return "Hash job init";
    weave_security_job_step(&job, 1);
    weave_security_job_cancel(&job);
    if(weave_security_job_step(&job, 1) != WeaveSecurityJobCancelled ||
       weave_security_job_result(&job, out, 32)) return "Job cancel";
    if(!weave_security_job_begin_hmac_sha256(&job, key, 3, input, 3)) return "HMAC job init";
    while(weave_security_job_step(&job, 1) == WeaveSecurityJobRunning) {}
    ok = weave_security_job_result(&job, out, 32) && weave_security_equal(out, hmac_expected, 32);
    weave_security_job_cancel(&job);
    if(!ok) return "HMAC job";
    if(!weave_security_aead_hw_selftest()) return "HW AES-GCM";
    weave_security_wipe(out, sizeof(out));
    return NULL;
}
#endif

int32_t weave_smoke_app(void* arg) {
    UNUSED(arg);
    const char* error = run_legacy();
#if SMOKE_V4
    if(!error) error = run_v4();
#endif
    FURI_LOG_I("WeaveSmoke", "Security API %s: %s; stack free=%lu",
        SMOKE_V4 ? "v4" : "v3 legacy", error ? error : "PASS",
        (unsigned long)furi_thread_get_stack_space(furi_thread_get_current_id()));
    DialogsApp* dialogs = furi_record_open(RECORD_DIALOGS);
    DialogMessage* message = dialog_message_alloc();
    dialog_message_set_header(message, error ? "FAIL" : "PASS", 64, 5, AlignCenter, AlignTop);
    dialog_message_set_text(message, error ? error : "Security API verified", 64, 30, AlignCenter, AlignCenter);
    dialog_message_set_buttons(message, NULL, "OK", NULL);
    dialog_message_show(dialogs, message);
    dialog_message_free(message);
    furi_record_close(RECORD_DIALOGS);
    return error ? 1 : 0;
}
