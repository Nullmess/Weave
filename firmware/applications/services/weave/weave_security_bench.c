/* On-device CLI benchmark. Fixed public test vectors, never real secrets. */
#include "weave_security.h"
#include <furi.h>
#include <stdio.h>
#include <string.h>

static void report(const char* name, bool ok, uint32_t start, size_t heap_before) {
    uint32_t elapsed = furi_get_tick() - start;
    printf("%s: %s, ticks=%lu, hz=%lu, heap_before=%lu, heap_after=%lu, "
           "max_free_block=%lu, stack_min_free=%lu\r\n", name, ok ? "PASS" : "FAIL",
           (unsigned long)elapsed, (unsigned long)furi_kernel_get_tick_frequency(),
           (unsigned long)heap_before, (unsigned long)memmgr_get_free_heap(),
           (unsigned long)memmgr_heap_get_max_free_block(),
           (unsigned long)furi_thread_get_stack_space(furi_thread_get_current_id()));
}

void weave_security_bench(bool full) {
    static const uint8_t expected_short[32] = {
        0x5e,0xc0,0x2b,0x91,0xa4,0xb5,0x9c,0x6f,0x59,0xdd,0x5f,0xbe,0x4c,0xa6,0x49,0xec,
        0xe4,0xfa,0x85,0x68,0xcd,0xb8,0xba,0x36,0xcf,0x41,0x42,0x6e,0x88,0x05,0x52,0x2b,
    };
    static const uint8_t expected_long[32] = {
        0x95,0xc4,0x43,0xd0,0x4a,0x2b,0x5e,0x20,0x4a,0x2e,0x7f,0x3d,0x5b,0xd4,0xf9,0xa8,
        0x06,0x78,0x1e,0x0f,0x1d,0xc6,0x7d,0x82,0x35,0xfa,0xba,0xf9,0xbb,0xe0,0xb4,0xc9,
    };
    const uint32_t rounds = full ? 150000U : 10000U;
    const uint8_t* expected = full ? expected_long : expected_short;
    uint8_t result[32];
    printf("Weave Security API %lu; job bytes=%lu; PBKDF2 rounds=%lu\r\n",
           (unsigned long)weave_security_api_version(), (unsigned long)sizeof(WeaveSecurityJob),
           (unsigned long)rounds);
    size_t heap = memmgr_get_free_heap(); uint32_t start = furi_get_tick();
    bool ok = weave_security_pbkdf2_sha256((const uint8_t*)"password", 8,
        (const uint8_t*)"salt", 4, rounds, result, sizeof(result)) &&
        weave_security_equal(result, expected, sizeof(result));
    report("PBKDF2-SHA256", ok, start, heap);
    weave_security_wipe(result, sizeof(result));
    /* Compare caller-scheduled jobs with the same public KAT. */
    WeaveSecurityJob job;
    heap = memmgr_get_free_heap(); start = furi_get_tick();
    bool job_ok = weave_security_job_begin_pbkdf2(&job, (const uint8_t*)"password", 8,
        (const uint8_t*)"salt", 4, rounds, sizeof(result));
    uint64_t done = 0, total = 0, previous = 0;
    uint32_t max_step = 0;
    WeaveSecurityJobState state = WeaveSecurityJobInvalid;
    while(job_ok) {
        uint32_t step_start = furi_get_tick();
        state = weave_security_job_step(&job, 256);
        uint32_t step_ticks = furi_get_tick() - step_start;
        if(step_ticks > max_step) max_step = step_ticks;
        job_ok = weave_security_job_progress(&job, &done, &total) && total == rounds &&
                 done > previous && done - previous <= 256;
        previous = done;
        if(state != WeaveSecurityJobRunning) break;
        furi_delay_tick(1);
    }
    job_ok = job_ok && state == WeaveSecurityJobDone && done == rounds &&
        weave_security_job_result(&job, result, sizeof(result)) &&
        weave_security_equal(result, expected, sizeof(result));
    weave_security_job_cancel(&job);
    report("PBKDF2 cooperative", job_ok, start, heap);
    printf("PBKDF2 max slice ticks=%lu (budget=256)\r\n", (unsigned long)max_step);
    weave_security_wipe(result, sizeof(result));
    heap = memmgr_get_free_heap(); start = furi_get_tick();
    ok = weave_security_aead_hw_selftest();
    report("AES-GCM hardware KAT", ok, start, heap);
    if(!ok) return;
    /* Nonzero inputs and AAD, software/hardware interoperability. */
    uint8_t key[32], nonce[12], aad[16], plain[128], a[128], b[128], ta[16], tb[16];
    memset(key, 0x42, sizeof(key)); memset(nonce, 0x23, sizeof(nonce));
    memset(aad, 0x17, sizeof(aad)); memset(plain, 0x91, sizeof(plain));
    heap = memmgr_get_free_heap(); start = furi_get_tick();
    ok = weave_security_aead_encrypt(key, nonce, aad, sizeof(aad), plain, sizeof(plain), a, ta);
    report("AES-GCM software 128B", ok, start, heap);
    if(!ok) return;
    heap = memmgr_get_free_heap(); start = furi_get_tick();
    ok = weave_security_aead_encrypt_hw(key, nonce, aad, sizeof(aad), plain, sizeof(plain), b, tb) &&
        weave_security_equal(a, b, sizeof(a)) && weave_security_equal(ta, tb, sizeof(ta));
    report("AES-GCM hardware 128B", ok, start, heap);
    if(ok) {
        memset(b, 0xa5, sizeof(b));
        ok = weave_security_aead_decrypt_hw(key, nonce, aad, sizeof(aad), a, sizeof(a), ta, b) &&
             weave_security_equal(b, plain, sizeof(plain));
        printf("AES-GCM interoperability: %s\r\n", ok ? "PASS" : "FAIL");
    }
    weave_security_wipe(b, sizeof(b));
}
