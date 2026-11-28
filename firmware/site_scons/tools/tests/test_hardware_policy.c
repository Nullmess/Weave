#include "weave_hardware_policy.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    WeaveTrustAnchor anchor;
    bool available, authentic, conflict, no_write, lose_read, lose_ack;
    unsigned calls;
} Mock;
static bool read_anchor(void* ctx, const uint8_t scope[32], WeaveTrustAnchor* out) {
    Mock* m=ctx;
    if(!m->available || scope[0]!=42) return false;
    *out=m->anchor;return true;
}
static bool authenticate(void* ctx,const uint8_t scope[32],const WeaveTrustAnchor* candidate) {
    Mock* m=ctx; return m->authentic && scope[0]==42 && candidate->digest[0]!=0;
}
static bool advance(void* ctx,const uint8_t scope[32],const WeaveTrustAnchor* old,const WeaveTrustAnchor* next) {
    Mock* m=ctx; ++m->calls;
    if(m->conflict || scope[0]!=42 || old->generation!=m->anchor.generation ||
       memcmp(old->digest,m->anchor.digest,32)) return false;
    if(!m->no_write) m->anchor=*next;
    if(m->lose_read) m->available=false;
    return !m->lose_ack;
}
int main(void) {
    uint8_t scope[32]={42}, wrong[32]={43};
    WeaveTrustAnchor a={7,{1}}, b={8,{2}}, c={9,{3}}, forged={8,{4}};
    Mock m={.anchor=a,.available=true,.authentic=true};
    WeaveTrustBackend backend={&m,read_anchor,authenticate,advance};
    assert(weave_trust_verify(NULL,scope,&a)==WeaveTrustUnavailable);
    assert(weave_trust_commit(NULL,scope,&b)==WeaveTrustUnavailable);
    assert(weave_trust_verify(&backend,wrong,&a)==WeaveTrustUnavailable);
    assert(weave_trust_verify(&backend,scope,&a)==WeaveTrustOk);
    assert(weave_trust_verify(&backend,scope,&b)==WeaveTrustRejected);
    assert(weave_trust_commit(&backend,scope,&c)==WeaveTrustRejected);
    m.anchor.digest[0]=0; /* invalid current state must never be advanced */
    assert(weave_trust_commit(&backend,scope,&b)==WeaveTrustRejected && m.calls==0);
    m.anchor=a;
    m.authentic=false;
    assert(weave_trust_commit(&backend,scope,&b)==WeaveTrustRejected);
    assert(weave_trust_verify(&backend,scope,&a)==WeaveTrustRejected);
    assert(m.calls==0);m.authentic=true;
    m.conflict=true;
    assert(weave_trust_commit(&backend,scope,&b)==WeaveTrustConflict);
    m.conflict=false;m.no_write=true;
    assert(weave_trust_commit(&backend,scope,&b)==WeaveTrustConflict);
    m.no_write=false;
    assert(weave_trust_commit(&backend,scope,&b)==WeaveTrustOk);
    assert(weave_trust_verify(&backend,scope,&a)==WeaveTrustRejected);
    assert(weave_trust_verify(&backend,scope,&forged)==WeaveTrustRejected);
    assert(weave_trust_commit(&backend,scope,&forged)==WeaveTrustRejected);
    assert(weave_trust_commit(&backend,scope,&a)==WeaveTrustRejected);
    assert(weave_trust_commit(&backend,scope,&b)==WeaveTrustOk);
    assert(m.calls==3);
    m.lose_ack=true;
    assert(weave_trust_commit(&backend,scope,&c)==WeaveTrustConflict);
    assert(weave_trust_verify(&backend,scope,&c)==WeaveTrustOk);
    assert(weave_trust_commit(&backend,scope,&c)==WeaveTrustOk);
    m.lose_ack=false;m.lose_read=true;
    WeaveTrustAnchor d={10,{5}};
    assert(weave_trust_commit(&backend,scope,&d)==WeaveTrustUnavailable);
    m.available=true;m.lose_read=false;
    assert(weave_trust_verify(&backend,scope,&d)==WeaveTrustOk);
    m.anchor.generation=UINT64_MAX;
    assert(weave_trust_commit(&backend,scope,&a)==WeaveTrustRejected);
    puts("Trusted anchor policy: missing backend, authentication, rollback, conflict, overflow and interrupted commit PASS");
    WeaveMpuRange arena={0x20000000,0x10000};
    WeaveMpuLayout l={{0x20000000,4096},{0x20001000,1024},{0x20002000,2048},{0x20003000,1024}};
    assert(weave_mpu_layout_valid(&l,arena));
    assert(weave_mpu_buffer_valid(&l,0x20002000,2048,true));
    assert(weave_mpu_buffer_valid(&l,0x200033ff,1,true));
    assert(weave_mpu_buffer_valid(&l,0x20001000,1024,false));
    assert(!weave_mpu_buffer_valid(&l,0x20001000,1,true));
    assert(!weave_mpu_buffer_valid(&l,0x20000000,1,false));
    assert(!weave_mpu_buffer_valid(&l,0x40000000,1,true));
    assert(!weave_mpu_buffer_valid(&l,0x200027ff,2,true));
    assert(!weave_mpu_buffer_valid(&l,0x20002000,0,true));
    assert(!weave_mpu_buffer_valid(&l,UINT32_MAX,2,true));
    assert(!weave_mpu_buffer_valid(&l,0x20002000,SIZE_MAX,true));
    WeaveMpuLayout bad=l;bad.stack.base=bad.data.base;
    assert(!weave_mpu_layout_valid(&bad,arena));
    bad=l;bad.stack.base+=1;assert(!weave_mpu_layout_valid(&bad,arena));
    bad=l;bad.stack.size=96;assert(!weave_mpu_layout_valid(&bad,arena));
    bad=l;bad.stack.size=16;assert(!weave_mpu_layout_valid(&bad,arena));
    bad=l;bad.stack.base=0x20010000;assert(!weave_mpu_layout_valid(&bad,arena));
    assert(!weave_mpu_layout_valid(&l,(WeaveMpuRange){UINT32_MAX,2}));
    puts("MPU layout and syscall buffer policy: alignment, bounds, overlap, permissions and overflow PASS");
    WeaveMpuRegionRegisters regs[4];
    assert(weave_mpu_encode_layout(&l,arena,4,8,0x0f,regs));
    assert(regs[0].rbar == 0x20000014U && regs[0].rasr == 0x06070017U);
    assert(regs[1].rbar == 0x20001015U && regs[1].rasr == 0x16070013U);
    assert(regs[2].rbar == 0x20002016U && regs[2].rasr == 0x13070015U);
    assert(regs[3].rbar == 0x20003017U && regs[3].rasr == 0x13070013U);
    assert(!weave_mpu_encode_layout(&l,arena,4,8,0x10,regs));
    for(size_t i=0;i<4;i++) assert(!regs[i].rbar && !regs[i].rasr);
    assert(!weave_mpu_encode_layout(&l,arena,5,8,0,regs));
    assert(!weave_mpu_encode_layout(&l,arena,UINT32_MAX,8,0,regs));
    assert(!weave_mpu_encode_layout(&l,arena,0,3,0,regs));
    assert(!weave_mpu_encode_layout(&l,arena,0,17,0,regs));
    assert(!weave_mpu_encode_layout(&bad,arena,4,8,0,regs));
    puts("MPU descriptors: RX/R-XN/RW-XN, size encoding, reserved regions and region budget PASS");

}
