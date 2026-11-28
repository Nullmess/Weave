#include "weave_hardware_policy.h"

static bool equal_anchor(const WeaveTrustAnchor* a, const WeaveTrustAnchor* b) {
    unsigned diff = 0;
    for(size_t i = 0; i < WEAVE_TRUST_DIGEST_SIZE; ++i) diff |= a->digest[i] ^ b->digest[i];
    return a->generation == b->generation && diff == 0;
}
static bool backend_valid(const WeaveTrustBackend* b) {
    return b && b->read && b->authenticate && b->advance;
}
WeaveTrustResult weave_trust_verify(const WeaveTrustBackend* b, const uint8_t scope[32],
                                  const WeaveTrustAnchor* candidate) {
    if(!scope || !candidate) return WeaveTrustInvalid;
    if(!backend_valid(b)) return WeaveTrustUnavailable;
    WeaveTrustAnchor current = {0};
    if(!b->read(b->context, scope, &current)) return WeaveTrustUnavailable;
    if(!equal_anchor(&current, candidate) || !b->authenticate(b->context, scope, candidate))
        return WeaveTrustRejected;
    return WeaveTrustOk;
}
WeaveTrustResult weave_trust_commit(const WeaveTrustBackend* b, const uint8_t scope[32],
                                  const WeaveTrustAnchor* candidate) {
    if(!scope || !candidate) return WeaveTrustInvalid;
    if(!backend_valid(b)) return WeaveTrustUnavailable;
    WeaveTrustAnchor current = {0};
    if(!b->read(b->context, scope, &current)) return WeaveTrustUnavailable;
    if(!b->authenticate(b->context, scope, &current) ||
       !b->authenticate(b->context, scope, candidate)) return WeaveTrustRejected;
    if(equal_anchor(&current, candidate)) return WeaveTrustOk; /* idempotent recovery */
    if(current.generation == UINT64_MAX || candidate->generation != current.generation + 1U)
        return WeaveTrustRejected;
    if(!b->advance(b->context, scope, &current, candidate)) return WeaveTrustConflict;
    WeaveTrustAnchor confirmed = {0};
    if(!b->read(b->context, scope, &confirmed)) return WeaveTrustUnavailable;
    return equal_anchor(&confirmed, candidate) ? WeaveTrustOk : WeaveTrustConflict;
}

static uint64_t end(WeaveMpuRange r) { return (uint64_t)r.base + r.size; }
static bool valid_range(WeaveMpuRange r) {
    return r.size && end(r) <= (UINT64_C(1) << 32);
}
static bool contained(WeaveMpuRange outer, WeaveMpuRange inner) {
    return valid_range(outer) && valid_range(inner) && inner.base >= outer.base &&
           end(inner) <= end(outer);
}
static bool aligned_region(WeaveMpuRange r) {
    return valid_range(r) && r.size >= 32U && !(r.size & (r.size - 1U)) &&
           !(r.base & (r.size - 1U));
}
static bool overlap(WeaveMpuRange a, WeaveMpuRange b) {
    return (uint64_t)a.base < end(b) && (uint64_t)b.base < end(a);
}
bool weave_mpu_layout_valid(const WeaveMpuLayout* layout, WeaveMpuRange arena) {
    if(!layout || !valid_range(arena)) return false;
    const WeaveMpuRange ranges[] = {layout->code, layout->readonly, layout->data, layout->stack};
    for(size_t i = 0; i < 4; ++i) {
        if(!aligned_region(ranges[i]) || !contained(arena, ranges[i])) return false;
        for(size_t j = 0; j < i; ++j) if(overlap(ranges[i], ranges[j])) return false;
    }
    return true;
}
bool weave_mpu_buffer_valid(const WeaveMpuLayout* l, uint32_t addr, size_t len, bool write) {
    if(!l || !len || len > UINT32_MAX) return false;
    const WeaveMpuRange buffer = {addr, (uint32_t)len};
    /* Only call with a previously validated, immutable layout. No code pointers. */
    return contained(l->data, buffer) || contained(l->stack, buffer) ||
           (!write && contained(l->readonly, buffer));
}

bool weave_mpu_encode_layout(const WeaveMpuLayout* l, WeaveMpuRange arena,
    unsigned first, unsigned region_count, uint32_t occupied,
    WeaveMpuRegionRegisters out[4]) {
    if(!out) return false;
    for(size_t i = 0; i < 4; ++i) out[i] = (WeaveMpuRegionRegisters){0, 0};
    /* ARMv7-M RBAR region selector has four bits. Check without overflow. */
    if(region_count < 4U || region_count > 16U || first > region_count - 4U ||
       (occupied & (UINT32_C(15) << first)) || !weave_mpu_layout_valid(l, arena)) return false;
    const WeaveMpuRange ranges[] = {l->code, l->readonly, l->data, l->stack};
    for(unsigned i = 0; i < 4; ++i) {
        unsigned exponent = 0;
        for(uint32_t size = ranges[i].size; size > 1U; size >>= 1) ++exponent;
        /* RBAR.VALID=1; normal SRAM, TEX=0/C=1/B=1/S=1, no subregion
         * disabling. AP=0b110 read-only or 0b011 read-write, XN except code. */
        out[i].rbar = ranges[i].base | UINT32_C(0x10) | (first + i);
        out[i].rasr = UINT32_C(1) | ((exponent - 1U) << 1) |
                      (UINT32_C(7) << 16) | ((i < 2U ? UINT32_C(6) : UINT32_C(3)) << 24) |
                      (i == 0U ? 0U : UINT32_C(1) << 28);
    }
    return true;
}
