#pragma once
/* Internal policy building blocks. No native FAP isolation or hardware counter
 * is enabled by these functions. Do not export as an SDK security guarantee. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WEAVE_TRUST_DIGEST_SIZE 32U

typedef struct {
    uint64_t generation;
    uint8_t digest[WEAVE_TRUST_DIGEST_SIZE];
} WeaveTrustAnchor;

typedef enum {
    WeaveTrustOk,
    WeaveTrustUnavailable,
    WeaveTrustRejected,
    WeaveTrustConflict,
    WeaveTrustInvalid,
} WeaveTrustResult;

/* A future privileged backend must bind each scope to a distinct, provisioned
 * anchor, protect it against reset/replay, and atomically compare+advance both
 * generation and digest. No fallback to the SD card or ordinary flash.
 * read() must fail on an unprovisioned anchor. authenticate() must verify the
 * exact candidate digest/generation/scope against a trusted signature or MAC.
 * A successful advance must be durable before returning. */
typedef struct {
    void* context;
    bool (*read)(void*, const uint8_t scope[32], WeaveTrustAnchor*);
    bool (*authenticate)(void*, const uint8_t scope[32], const WeaveTrustAnchor*);
    bool (*advance)(void*, const uint8_t scope[32], const WeaveTrustAnchor* expected,
                    const WeaveTrustAnchor* next);
} WeaveTrustBackend;

/* Reads must match the committed state exactly. Future generations are not
 * accepted until committed. Caller must use the exact bytes it authenticated. */
WeaveTrustResult weave_trust_verify(const WeaveTrustBackend*, const uint8_t scope[32],
                                  const WeaveTrustAnchor* candidate);
/* Commit only AFTER the candidate bytes are durable and independently verified.
 * Must be called by an authorized privileged transaction coordinator. On a
 * conflict or I/O failure re-read the protected anchor before recovering. */
WeaveTrustResult weave_trust_commit(const WeaveTrustBackend*, const uint8_t scope[32],
                                  const WeaveTrustAnchor* candidate);

/* Exact ARMv7-M MPU region candidates. All four allocations must be owned
 * exclusively by the isolated task; code is RX, the other three regions XN.
 * This validates layout only: it does not allocate, switch privilege or program
 * the MPU. No DMA or syscall isolation follows from this check. */
typedef struct { uint32_t base; uint32_t size; } WeaveMpuRange;
typedef struct {
    WeaveMpuRange code, readonly, data, stack;
} WeaveMpuLayout;
/* arena is a trusted, exclusively reserved RAM range, not an FAP input. */
bool weave_mpu_layout_valid(const WeaveMpuLayout*, WeaveMpuRange arena);
/* Syscall buffer check: reject wraparound, zero length and cross-region access.
 * The privileged caller still must copy inputs once and prevent TOCTOU. */
bool weave_mpu_buffer_valid(const WeaveMpuLayout*, uint32_t address, size_t length, bool write);

/* ARMv7-M RBAR/RASR values for a future trusted context switch. This only
 * encodes four regions, it neither programs registers nor changes privilege.
 * occupied is the trusted mask of all static/reserved regions; no overlap in
 * register indices is allowed. The caller must also prove there are no other
 * enabled regions granting wider unprivileged access and mediate DMA. */
typedef struct { uint32_t rbar, rasr; } WeaveMpuRegionRegisters;
bool weave_mpu_encode_layout(const WeaveMpuLayout*, WeaveMpuRange arena,
    unsigned first, unsigned region_count, uint32_t occupied,
    WeaveMpuRegionRegisters out[4]);
