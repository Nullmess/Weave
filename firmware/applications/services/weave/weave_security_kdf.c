/* Generic SHA-256, HMAC, HKDF and resumable PBKDF2. GPL-3.0-or-later.
 * No allocation, shared state, secret-indexed tables or resident worker.
 * SHA-256 compression is specialized for 32-bit Cortex-M registers; the
 * eight-round loop and fixed-size HMAC block avoid redundant serialization.
 * Compression core adapted from the project's FlipIAM implementation.
 */
#include "weave_security.h"
#include <string.h>
static inline uint32_t r32(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
static const uint32_t k256[64]={
    0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
    0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
    0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
    0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
    0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
    0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
    0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
    0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u,
};
static const uint32_t iv256[8]={0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
    0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
static inline uint32_t load32(const uint8_t* p) {
    return ((uint32_t)p[0]<<24)|((uint32_t)p[1]<<16)|((uint32_t)p[2]<<8)|p[3];
}
static void sha256_words(uint32_t state[8],uint32_t w[64]) {
    for(unsigned i=16;i<64;i++) {
        uint32_t x=w[i-15],y=w[i-2];
        w[i]=w[i-16]+(r32(x,7)^r32(x,18)^(x>>3))+w[i-7]+(r32(y,17)^r32(y,19)^(y>>10));
    }
    uint32_t a=state[0],b=state[1],c=state[2],d=state[3],e=state[4],f=state[5],g=state[6],h=state[7];
#define R(A,B,C,D,E,F,G,H,I) do { \
    uint32_t t=(H)+(r32((E),6)^r32((E),11)^r32((E),25))+((G)^((E)&((F)^(G))))+k256[(I)]+w[(I)]; \
    uint32_t u=(r32((A),2)^r32((A),13)^r32((A),22))+(((A)&(B))|((C)&((A)|(B)))); \
    (D)+=t; (H)=t+u; \
} while(0)
    for(unsigned i=0;i<64;i+=8) {
        R(a,b,c,d,e,f,g,h,i); R(h,a,b,c,d,e,f,g,i+1);
        R(g,h,a,b,c,d,e,f,i+2); R(f,g,h,a,b,c,d,e,i+3);
        R(e,f,g,h,a,b,c,d,i+4); R(d,e,f,g,h,a,b,c,i+5);
        R(c,d,e,f,g,h,a,b,i+6); R(b,c,d,e,f,g,h,a,i+7);
    }
#undef R
    state[0]+=a;state[1]+=b;state[2]+=c;state[3]+=d;
    state[4]+=e;state[5]+=f;state[6]+=g;state[7]+=h;
}

static void store32(uint8_t* p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
typedef WeaveSecuritySha256State Sha256;
static void sha_init(Sha256* ctx, const uint32_t* initial, uint64_t bytes) {
    memset(ctx, 0, sizeof(*ctx));
    memcpy(ctx->state, initial, 32);
    ctx->bytes = bytes;
}
static void sha_compress(Sha256* ctx, const uint8_t* input, uint32_t w[64]) {
    for(size_t i = 0; i < 16; i++) w[i] = load32(input + 4*i);
    sha256_words(ctx->state, w);
}
static void sha_update(Sha256* ctx, const uint8_t* p, size_t n, uint32_t w[64]) {
    ctx->bytes += n;
    while(n) {
        if(!ctx->used && n >= 64) {
            sha_compress(ctx, p, w); p += 64; n -= 64; continue;
        }
        size_t take = 64 - ctx->used;
        if(take > n) take = n;
        memcpy(ctx->buffer + ctx->used, p, take);
        p += take; n -= take; ctx->used += take;
        if(ctx->used == 64) { sha_compress(ctx, ctx->buffer, w); ctx->used = 0; }
    }
}
static void sha_finish(Sha256* ctx, uint32_t out[8], uint32_t w[64]) {
    uint64_t bits = ctx->bytes * 8;
    ctx->buffer[ctx->used++] = 0x80;
    if(ctx->used > 56) {
        memset(ctx->buffer + ctx->used, 0, 64 - ctx->used);
        sha_compress(ctx, ctx->buffer, w); ctx->used = 0;
    }
    memset(ctx->buffer + ctx->used, 0, 56 - ctx->used);
    store32(ctx->buffer + 56, (uint32_t)(bits >> 32));
    store32(ctx->buffer + 60, (uint32_t)bits);
    sha_compress(ctx, ctx->buffer, w);
    memcpy(out, ctx->state, 32);
    weave_security_wipe(ctx, sizeof(*ctx));
}
static void words_out(uint8_t* out, const uint32_t words[8], size_t n) {
    for(size_t i = 0; i < n; ++i) out[i] = (uint8_t)(words[i/4] >> (24 - 8*(i%4)));
}
static void pads(uint32_t inner[8], uint32_t outer[8], const uint8_t* key,
                 size_t n, uint32_t w[64]) {
    uint8_t digest[32];
    if(n > 64) {
        Sha256 ctx; uint32_t words[8];
        sha_init(&ctx, iv256, 0); sha_update(&ctx, key, n, w); sha_finish(&ctx, words, w);
        words_out(digest, words, 32); weave_security_wipe(words, sizeof(words));
        key = digest; n = 32;
    }
    for(size_t i = 0; i < 16; ++i) w[i] = 0x36363636u;
    for(size_t i = 0; i < n; ++i) w[i/4] ^= (uint32_t)key[i] << (24 - 8*(i%4));
    memcpy(inner, iv256, 32); sha256_words(inner, w);
    for(size_t i = 0; i < 16; ++i) w[i] ^= 0x6a6a6a6au;
    memcpy(outer, iv256, 32); sha256_words(outer, w);
    weave_security_wipe(digest, sizeof(digest));
}
/* Hash exactly 32 bytes after a precomputed 64-byte HMAC pad. */
static void fixed32(const uint32_t pad[8], uint32_t value[8], uint32_t w[64]) {
    memcpy(w, value, 32); w[8] = 0x80000000u;
    memset(w+9, 0, 6*sizeof(uint32_t)); w[15] = 768;
    memcpy(value, pad, 32); sha256_words(value, w);
}
static void hmac_parts(const uint32_t inner[8], const uint32_t outer[8],
                       const uint8_t* a, size_t an, const uint8_t* b, size_t bn,
                       const uint8_t* c, size_t cn, uint32_t out[8], uint32_t w[64]) {
    Sha256 ctx;
    sha_init(&ctx, inner, 64);
    sha_update(&ctx, a, an, w); sha_update(&ctx, b, bn, w); sha_update(&ctx, c, cn, w);
    sha_finish(&ctx, out, w); fixed32(outer, out, w);
}
uint32_t weave_security_api_version(void) { return WEAVE_SECURITY_API_VERSION; }
bool weave_security_sha256(const uint8_t* data, size_t size, uint8_t out[32]) {
    if(!out || (!data && size)) return false;
    Sha256 ctx; uint32_t w[64], value[8];
    sha_init(&ctx, iv256, 0); sha_update(&ctx, data, size, w); sha_finish(&ctx, value, w);
    words_out(out, value, 32);
    weave_security_wipe(w, sizeof(w)); weave_security_wipe(value, sizeof(value));
    return true;
}
bool weave_security_hmac_sha256(const uint8_t* key, size_t key_len,
    const uint8_t* data, size_t size, uint8_t out[32]) {
    if(!out || (!key && key_len) || (!data && size)) return false;
    uint32_t inner[8], outer[8], value[8], w[64];
    pads(inner, outer, key, key_len, w);
    hmac_parts(inner, outer, data, size, NULL, 0, NULL, 0, value, w);
    words_out(out, value, 32);
    weave_security_wipe(inner, sizeof(inner)); weave_security_wipe(outer, sizeof(outer));
    weave_security_wipe(value, sizeof(value)); weave_security_wipe(w, sizeof(w));
    return true;
}
bool weave_security_hmac_sha256_init(WeaveSecurityHmacSha256Context* ctx,
    const uint8_t* key, size_t key_size) {
    if(!ctx) return false;
    weave_security_wipe(ctx, sizeof(*ctx));
    if(!key && key_size) return false;
    uint32_t w[64]; pads(ctx->inner, ctx->outer, key, key_size, w);
    weave_security_wipe(w, sizeof(w)); ctx->ready = 1;
    return true;
}
bool weave_security_hmac_sha256_compute(const WeaveSecurityHmacSha256Context* ctx,
    const uint8_t* data, size_t size, uint8_t out[32]) {
    if(!ctx || ctx->ready != 1 || !out || (!data && size)) return false;
    uint32_t value[8], w[64];
    hmac_parts(ctx->inner, ctx->outer, data, size, NULL, 0, NULL, 0, value, w);
    words_out(out, value, 32);
    weave_security_wipe(value, sizeof(value)); weave_security_wipe(w, sizeof(w));
    return true;
}
void weave_security_hmac_sha256_clear(WeaveSecurityHmacSha256Context* ctx) {
    if(ctx) weave_security_wipe(ctx, sizeof(*ctx));
}
bool weave_security_hkdf_sha256(const uint8_t* key, size_t key_size,
    const uint8_t* salt, size_t salt_size, const uint8_t* info, size_t info_size,
    uint8_t* out, size_t out_size) {
    if(!key || !key_size || (!salt && salt_size) || (!info && info_size) ||
       (!out && out_size) || out_size > 255U * 32U) return false;
    uint8_t prk[32], block[32]; uint32_t inner[8], outer[8], value[8], w[64];
    /* An empty HMAC key and a 32-byte zero key produce identical pads. */
    if(!weave_security_hmac_sha256(salt, salt_size, key, key_size, prk)) return false;
    pads(inner, outer, prk, sizeof(prk), w);
    size_t done = 0, previous = 0; uint8_t counter = 1;
    while(done < out_size) {
        hmac_parts(inner, outer, block, previous, info, info_size, &counter, 1, value, w);
        words_out(block, value, 32);
        size_t n = out_size - done; if(n > 32) n = 32;
        memcpy(out + done, block, n); done += n; previous = 32; ++counter;
    }
    weave_security_wipe(prk, sizeof(prk)); weave_security_wipe(block, sizeof(block));
    weave_security_wipe(inner, sizeof(inner)); weave_security_wipe(outer, sizeof(outer));
    weave_security_wipe(value, sizeof(value)); weave_security_wipe(w, sizeof(w));
    return true;
}
bool weave_security_job_begin_pbkdf2(WeaveSecurityJob* job, const uint8_t* password,
    size_t password_size, const uint8_t* salt, size_t salt_size, uint32_t iterations,
    size_t out_size) {
    if(!job) return false;
    weave_security_wipe(job, sizeof(*job));
    if(!password || !password_size || (!salt && salt_size) || salt_size > 64 ||
       iterations < 10000 || !out_size || out_size > 64) return false;
    WeaveSecurityKdfState* kdf = &job->work.kdf;
    uint32_t w[64]; pads(kdf->inner, kdf->outer, password, password_size, w);
    weave_security_wipe(w, sizeof(w));
    if(salt_size) memcpy(kdf->salt, salt, salt_size);
    kdf->salt_size = (uint32_t)salt_size; job->out_size = (uint32_t)out_size;
    kdf->iterations = iterations; kdf->block = 1;
    job->total = (uint64_t)((out_size + 31U)/32U) * iterations;
    job->kind = WeaveSecurityJobPbkdf2; job->state = WeaveSecurityJobRunning;
    return true;
}
static bool hash_job_begin(WeaveSecurityJob* job, const uint8_t* data, size_t size,
                           const uint8_t* key, size_t key_size, bool hmac) {
    if(!job) return false;
    weave_security_wipe(job, sizeof(*job));
    if((!data && size) || (!key && key_size)) return false;
    WeaveSecurityHashJobState* h = &job->work.hash;
    if(hmac) {
        uint32_t inner[8], w[64]; pads(inner, h->outer, key, key_size, w);
        sha_init(&h->hash, inner, 64);
        weave_security_wipe(inner, sizeof(inner)); weave_security_wipe(w, sizeof(w));
    } else sha_init(&h->hash, iv256, 0);
    h->input = data; h->size = size; job->total = size; job->out_size = 32;
    job->kind = hmac ? WeaveSecurityJobHmacSha256 : WeaveSecurityJobSha256;
    job->state = WeaveSecurityJobRunning;
    return true;
}
bool weave_security_job_begin_sha256(WeaveSecurityJob* job, const uint8_t* data, size_t size) {
    return hash_job_begin(job, data, size, NULL, 0, false);
}
bool weave_security_job_begin_hmac_sha256(WeaveSecurityJob* job, const uint8_t* key,
    size_t key_size, const uint8_t* data, size_t size) {
    return hash_job_begin(job, data, size, key, key_size, true);
}
static void finish_job(WeaveSecurityJob* job) {
    /* Only the derived result and public progress counters survive. */
    weave_security_wipe(&job->work, sizeof(job->work));
    job->state = WeaveSecurityJobDone;
}
WeaveSecurityJobState weave_security_job_step(WeaveSecurityJob* job, uint32_t budget) {
    if(!job) return WeaveSecurityJobInvalid;
    if(job->state != WeaveSecurityJobRunning || !budget) return job->state;
    uint32_t w[64];
    if(job->kind == WeaveSecurityJobSha256 || job->kind == WeaveSecurityJobHmacSha256) {
        WeaveSecurityHashJobState* h = &job->work.hash;
        size_t n = h->size - h->processed; if(n > budget) n = budget;
        if(n) sha_update(&h->hash, h->input + h->processed, n, w);
        h->processed += n; job->completed = h->processed;
        if(h->processed == h->size) {
            uint32_t value[8]; sha_finish(&h->hash, value, w);
            if(job->kind == WeaveSecurityJobHmacSha256) fixed32(h->outer, value, w);
            words_out(job->result, value, 32); weave_security_wipe(value, sizeof(value));
            finish_job(job);
        }
    } else if(job->kind == WeaveSecurityJobPbkdf2) {
        WeaveSecurityKdfState* kdf = &job->work.kdf;
        while(budget-- && job->state == WeaveSecurityJobRunning) {
            if(!kdf->round) {
                uint8_t index[4]; store32(index, kdf->block);
                hmac_parts(kdf->inner, kdf->outer, kdf->salt, kdf->salt_size,
                           index, sizeof(index), NULL, 0, kdf->u, w);
                memcpy(kdf->acc, kdf->u, 32);
            } else {
                fixed32(kdf->inner, kdf->u, w); fixed32(kdf->outer, kdf->u, w);
                for(size_t i = 0; i < 8; ++i) kdf->acc[i] ^= kdf->u[i];
            }
            ++kdf->round;
            if(kdf->round == kdf->iterations) {
                size_t offset = (kdf->block - 1U) * 32U;
                size_t n = job->out_size - offset; if(n > 32) n = 32;
                words_out(job->result + offset, kdf->acc, n);
                if(offset + n == job->out_size) finish_job(job);
                else { ++kdf->block; kdf->round = 0; }
            }
        }
        /* Avoid a 64-bit increment on every round in the inner loop. */
        job->completed = job->state == WeaveSecurityJobDone ? job->total :
            (uint64_t)(kdf->block - 1U) * kdf->iterations + kdf->round;
    } else {
        weave_security_wipe(job, sizeof(*job));
    }
    weave_security_wipe(w, sizeof(w));
    return job->state;
}
bool weave_security_job_progress(const WeaveSecurityJob* job, uint64_t* done, uint64_t* total) {
    if(!job || !done || !total || (job->state != WeaveSecurityJobRunning &&
       job->state != WeaveSecurityJobDone)) return false;
    *done = job->completed; *total = job->total;
    return true;
}
void weave_security_job_cancel(WeaveSecurityJob* job) {
    if(job) { weave_security_wipe(job, sizeof(*job)); job->state = WeaveSecurityJobCancelled; }
}
bool weave_security_job_result(WeaveSecurityJob* job, uint8_t* out, size_t size) {
    if(!job || !out || job->state != WeaveSecurityJobDone || size != job->out_size) return false;
    memcpy(out, job->result, size); weave_security_wipe(job, sizeof(*job)); return true;
}
bool weave_security_pbkdf2_sha256(const uint8_t* password, size_t password_size,
    const uint8_t* salt, size_t salt_size, uint32_t iterations, uint8_t* out, size_t out_size) {
    if(!out) return false;
    WeaveSecurityJob job;
    if(!weave_security_job_begin_pbkdf2(&job, password, password_size, salt, salt_size,
                                        iterations, out_size)) return false;
    while(weave_security_job_step(&job, 256) == WeaveSecurityJobRunning) {}
    bool ok = weave_security_job_result(&job, out, out_size);
    if(!ok) { weave_security_job_cancel(&job); weave_security_wipe(out, out_size); }
    return ok;
}
