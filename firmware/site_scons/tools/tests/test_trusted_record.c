#include "weave_trusted_record.h"
#include <assert.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>

#define CAP 64
static const uint8_t scope[32] = {42};
typedef struct {
    WeaveTrustAnchor a;
    uint8_t slot[2][CAP]; size_t n[2];
    bool available, authentic, lose_ack, corrupt, fail_write, fail_read;
    size_t cut; unsigned writes, advances;
} Store;
static bool hash(const uint8_t* p,size_t n,uint8_t d[32]) { return SHA256(p,n,d)!=NULL; }
static WeaveTrustAnchor anchor(uint64_t g, const uint8_t* p,size_t n) {
    WeaveTrustAnchor a={.generation=g}; assert(hash(p,n,a.digest)); return a;
}
static bool read_anchor(void* ctx,const uint8_t s[32],WeaveTrustAnchor* a) {
    Store* m=ctx; if(!m->available || memcmp(s,scope,32)) return false; *a=m->a; return true;
}
static bool authenticate(void* ctx,const uint8_t s[32],const WeaveTrustAnchor* a) {
    (void)a; return ((Store*)ctx)->authentic && !memcmp(s,scope,32);
}
static bool advance(void* ctx,const uint8_t s[32],const WeaveTrustAnchor* a,const WeaveTrustAnchor* b) {
    Store* m=ctx; ++m->advances;
    if(memcmp(s,scope,32) || a->generation!=m->a.generation || memcmp(a->digest,m->a.digest,32)) return false;
    m->a=*b; return !m->lose_ack;
}
static bool read_slot(void* ctx,const uint8_t s[32],unsigned slot,uint8_t* p,size_t cap,size_t* n) {
    Store* m=ctx; assert(slot<2); if(m->fail_read || memcmp(s,scope,32) || !m->n[slot] || m->n[slot]>cap) return false;
    *n=m->n[slot]; memcpy(p,m->slot[slot],*n); return true;
}
static bool write_slot(void* ctx,const uint8_t s[32],unsigned slot,const uint8_t* p,size_t n) {
    Store* m=ctx; assert(slot<2 && n<=CAP); if(memcmp(s,scope,32)) return false; ++m->writes;
    size_t count=m->cut<n?m->cut:n; memcpy(m->slot[slot],p,count); m->n[slot]=count;
    if(m->corrupt && count) m->slot[slot][0]^=1;
    return !m->fail_write && count==n;
}
static void zero(const uint8_t* p,size_t n) { for(size_t i=0;i<n;i++) assert(!p[i]); }
int main(void) {
    const uint8_t old[]={1,2,3}, next[]={4,5,6,7,8}, wrong[]={9,9,9,9,9};
    const WeaveTrustAnchor a=anchor(10,old,sizeof(old)), b=anchor(11,next,sizeof(next));
    Store initial={.a=a,.available=true,.authentic=true,.cut=CAP,.n={sizeof(old),0}};
    memcpy(initial.slot[0],old,sizeof(old));
    Store m=initial;
    WeaveTrustBackend backend={&m,read_anchor,authenticate,advance};
    WeaveTrustedSlots slots={&m,read_slot,write_slot,hash};
    uint8_t scratch[CAP],out[CAP]; size_t n=0;
#define COMMIT(p) weave_trusted_record_commit(&backend,&slots,scope,&b,p,sizeof(next),scratch,sizeof(scratch))
#define READ() weave_trusted_record_read(&backend,&slots,scope,out,sizeof(out),&n)
    for(size_t cut=0;cut<sizeof(next);cut++) {
        m=initial;m.cut=cut;memset(scratch,0xab,sizeof(scratch));
        assert(COMMIT(next)==WeaveTrustUnavailable);zero(scratch,sizeof(scratch));
        assert(m.advances==0 && m.a.generation==10);
        assert(READ()==WeaveTrustOk && n==sizeof(old) && !memcmp(out,old,n));zero(out+n,sizeof(out)-n);
        m.cut=CAP;assert(COMMIT(next)==WeaveTrustOk);
        assert(READ()==WeaveTrustOk && n==sizeof(next) && !memcmp(out,next,n));
    }
    m=initial;assert(COMMIT(wrong)==WeaveTrustRejected && !m.writes);
    m.authentic=false;assert(COMMIT(next)==WeaveTrustRejected && !m.writes);
    m=initial;m.corrupt=true;assert(COMMIT(next)==WeaveTrustRejected && !m.advances);
    assert(READ()==WeaveTrustOk && n==sizeof(old));
    m=initial;m.fail_write=true;assert(COMMIT(next)==WeaveTrustUnavailable && !m.advances);
    assert(READ()==WeaveTrustOk && n==sizeof(old));
    m=initial;m.lose_ack=true;assert(COMMIT(next)==WeaveTrustConflict);
    assert(READ()==WeaveTrustOk && n==sizeof(next));
    assert(COMMIT(next)==WeaveTrustOk && m.writes==1 && m.advances==1);
    m.slot[1][0]^=1;memset(out,0xab,sizeof(out));assert(READ()==WeaveTrustRejected && !n);zero(out,sizeof(out));
    assert(COMMIT(next)==WeaveTrustRejected && m.writes==1); /* no fallback */
    m=initial;m.available=false;assert(READ()==WeaveTrustUnavailable && !n);zero(out,sizeof(out));
    assert(COMMIT(next)==WeaveTrustUnavailable && !m.writes);
    m=initial;m.fail_read=true;assert(COMMIT(next)==WeaveTrustUnavailable && !m.writes);
    puts("Trusted record: torn writes, corrupt readback, lost acknowledgement, idempotence, exact digest and no rollback PASS");
}
