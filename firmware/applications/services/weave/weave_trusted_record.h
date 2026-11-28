#pragma once
#include "weave_hardware_policy.h"

/* Internal privileged coordinator, not an SDK API. Each scope owns two distinct
 * durable slots. Slot generation parity is part of the layout. A provisioned
 * anchor must already point at a valid record. No legacy fallback or enrollment.
 * Calls for a scope MUST be serialized across readers/writers (including the
 * backend). The trusted caller owns the buffers and prevents concurrent edits.
 * hash() computes SHA-256 over the exact record; authenticate() in the anchor
 * backend binds the digest, generation and scope to an authorized record. */
typedef struct {
    void* context;
    bool (*read)(void*, const uint8_t scope[32], unsigned slot,
                 uint8_t* out, size_t capacity, size_t* size);
    bool (*write_durable)(void*, const uint8_t scope[32], unsigned slot,
                          const uint8_t* data, size_t size);
    bool (*hash)(const uint8_t* data, size_t size, uint8_t digest[32]);
} WeaveTrustedSlots;

/* On failure, output is wiped and size is zero. A protected anchor that points
 * to lost or corrupt storage is an error, not permission to use the other slot. */
WeaveTrustResult weave_trusted_record_read(const WeaveTrustBackend*, const WeaveTrustedSlots*,
    const uint8_t scope[32], uint8_t* out, size_t capacity, size_t* size);

/* scratch must not overlap candidate data, scope, backend, or metadata. It must
 * hold both current and candidate records (individually). Always wiped. After
 * a lost commit acknowledgement the caller can retry the exact candidate or
 * read; never infer the current state from a storage slot alone. */
WeaveTrustResult weave_trusted_record_commit(const WeaveTrustBackend*, const WeaveTrustedSlots*,
    const uint8_t scope[32], const WeaveTrustAnchor* candidate, const uint8_t* data, size_t size,
    uint8_t* scratch, size_t capacity);
