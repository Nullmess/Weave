#include "weave_trusted_record.h"

static void wipe(uint8_t* p, size_t n) {
    volatile uint8_t* v = p;
    while(n--) *v++ = 0;
}
static bool digest_matches(const WeaveTrustedSlots* s, const uint8_t* p, size_t n,
                           const WeaveTrustAnchor* a) {
    uint8_t digest[32] = {0};
    if(!s->hash(p, n, digest)) return false;
    unsigned diff = 0;
    for(size_t i = 0; i < sizeof(digest); ++i) diff |= digest[i] ^ a->digest[i];
    return diff == 0;
}
static bool same_anchor(const WeaveTrustAnchor* a, const WeaveTrustAnchor* b) {
    unsigned diff = 0;
    for(size_t i = 0; i < 32; ++i) diff |= a->digest[i] ^ b->digest[i];
    return a->generation == b->generation && diff == 0;
}
static bool available(const WeaveTrustBackend* b, const WeaveTrustedSlots* s) {
    return b && b->read && b->authenticate && b->advance && s && s->read &&
           s->write_durable && s->hash;
}
static WeaveTrustResult load_exact(const WeaveTrustBackend* b, const WeaveTrustedSlots* s,
    const uint8_t scope[32], const WeaveTrustAnchor* a, uint8_t* out, size_t cap, size_t* n) {
    *n = 0;
    if(!b->authenticate(b->context, scope, a)) return WeaveTrustRejected;
    if(!s->read(s->context, scope, (unsigned)(a->generation & 1U), out, cap, n))
        return WeaveTrustUnavailable;
    if(!*n || *n > cap || !digest_matches(s, out, *n, a)) return WeaveTrustRejected;
    return WeaveTrustOk;
}
WeaveTrustResult weave_trusted_record_read(const WeaveTrustBackend* b, const WeaveTrustedSlots* s,
    const uint8_t scope[32], uint8_t* out, size_t cap, size_t* size) {
    if(size) *size = 0;
    if(!out || !cap || !size || !scope) {
        if(out && cap) wipe(out, cap);
        return WeaveTrustInvalid;
    }
    WeaveTrustResult r = WeaveTrustUnavailable;
    WeaveTrustAnchor a = {0};
    size_t n = 0;
    if(available(b, s) && b->read(b->context, scope, &a)) {
        r = load_exact(b, s, scope, &a, out, cap, &n);
        if(r == WeaveTrustOk) r = weave_trust_verify(b, scope, &a);
    }
    if(r == WeaveTrustOk) {
        *size = n;
        wipe(out + n, cap - n);
    } else wipe(out, cap);
    return r;
}
WeaveTrustResult weave_trusted_record_commit(const WeaveTrustBackend* b, const WeaveTrustedSlots* s,
    const uint8_t scope[32], const WeaveTrustAnchor* candidate, const uint8_t* data, size_t size,
    uint8_t* scratch, size_t cap) {
    if(!scratch || !cap) return WeaveTrustInvalid;
    WeaveTrustResult r = WeaveTrustInvalid;
    WeaveTrustAnchor current = {0};
    size_t n = 0;
    if(!scope || !candidate || !data || !size || size > cap) goto done;
    r = WeaveTrustUnavailable;
    if(!available(b,s) || !b->read(b->context, scope, &current)) goto done;
    r = WeaveTrustRejected;
    if(!digest_matches(s, data, size, candidate) ||
       !b->authenticate(b->context, scope, candidate)) goto done;
    /* No writes if the trusted current record cannot be recovered. */
    r = load_exact(b, s, scope, &current, scratch, cap, &n);
    if(r != WeaveTrustOk) goto done;
    r = weave_trust_verify(b, scope, &current);
    if(r != WeaveTrustOk) goto done;
    if(same_anchor(&current, candidate)) goto done; /* already durable */
    r = WeaveTrustRejected;
    if(current.generation == UINT64_MAX || candidate->generation != current.generation + 1U)
        goto done;
    r = WeaveTrustUnavailable;
    if(!s->write_durable(s->context, scope, (unsigned)(candidate->generation & 1U), data, size))
        goto done;
    r = load_exact(b, s, scope, candidate, scratch, cap, &n);
    if(r != WeaveTrustOk) goto done;
    if(n != size) { r = WeaveTrustRejected; goto done; }
    /* Recheck current anchor before calling the atomic compare-and-advance. */
    r = weave_trust_verify(b, scope, &current);
    if(r == WeaveTrustOk) r = weave_trust_commit(b, scope, candidate);
done:
    wipe(scratch, cap);
    return r;
}
