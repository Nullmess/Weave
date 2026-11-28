#include "weave_isodep.h"
#include <string.h>
static void wipe(void* p, size_t n) { volatile uint8_t* v=p; while(n--) *v++=0; }
void weave_isodep_reset(WeaveIsoDep* s) { wipe(s,sizeof(*s)); }
static WeaveIsoAction one(WeaveIsoDep* s, uint8_t pcb) {
    wipe(s->frame,sizeof(s->frame)); s->frame[0]=pcb;s->frame_size=1;return WeaveIsoSend;
}
static WeaveIsoAction next_response(WeaveIsoDep* s) {
    const size_t left=s->response_size-s->response_offset;
    const size_t n=left<s->payload_max?left:s->payload_max;
    if(!n) return WeaveIsoAbort;
    const bool more=n<left;
    one(s,(uint8_t)(0x02U|s->tx_number|(more?0x10U:0U)));
    memcpy(s->frame+1,s->response+s->response_offset,n);s->frame_size=n+1;
    s->response_offset+=n;
    if(more) { s->phase=WeaveIsoAck; s->replay_ack=true; }
    else {
        wipe(s->response,sizeof(s->response));s->response_size=s->response_offset=0;
        s->expected=s->tx_number^1U;s->phase=WeaveIsoReceiving;
    }
    return WeaveIsoSend;
}
WeaveIsoAction weave_isodep_input(WeaveIsoDep* s,const uint8_t* f,size_t n,size_t peer) {
    if(!f||!n||n>WEAVE_ISODEP_FRAME_MAX||peer<16||peer>256) return WeaveIsoIgnore;
    const uint8_t pcb=f[0];
    if(pcb==0xc2 && n==1) { weave_isodep_reset(s);one(s,0xc2);return WeaveIsoHalt; }
    /* Optional PPS at the advertised default bitrate only. */
    if(!s->started && pcb==0xd0 && ((n==2&&f[1]==0x01)||(n==3&&f[1]==0x11&&f[2]==0))) {
        return one(s,0xd0);
    }
    if(pcb==0xf2 && n==2 && s->phase==WeaveIsoApp && s->wtx_pending && f[1]==WEAVE_ISODEP_WTX_MULTIPLIER) {
        s->wtx_pending=false;return WeaveIsoWait;
    }
    if((pcb==0xa2||pcb==0xa3||pcb==0xb2||pcb==0xb3) && n==1) {
        const uint8_t number=pcb&1U;
        if(s->phase==WeaveIsoAck) {
            if(number==s->tx_number) return WeaveIsoSend; /* R(NAK) or duplicate ACK */
            if(!(pcb&0x10U) && number==(s->tx_number^1U)) {s->tx_number^=1U;return next_response(s);}
        } else if(s->phase==WeaveIsoReceiving && s->frame_size &&
                  ((pcb&0x10U)||s->replay_ack) && number==(s->expected^1U)) return WeaveIsoSend;
        return WeaveIsoIgnore;
    }
    /* I-block, fixed bits, no CID/NAD. Empty INF is not a Weave message. */
    if((pcb&0xeeU)!=0x02U||n<2) return WeaveIsoIgnore;
    const uint8_t number=pcb&1U;
    if(s->phase!=WeaveIsoReceiving) {
        /* A retry cannot trigger the application a second time. */
        if(s->phase==WeaveIsoApp && number==(s->expected^1U) && s->wtx_pending)
            return WeaveIsoSend;
        return WeaveIsoIgnore;
    }
    if(number!=s->expected) return s->started&&s->frame_size?WeaveIsoSend:WeaveIsoIgnore;
    s->started=true;s->replay_ack=false;
    s->payload_max=peer-3U; /* PCB and CRC. */
    if(n-1U>sizeof(s->request)-s->request_size) return WeaveIsoAbort;
    memcpy(s->request+s->request_size,f+1,n-1U);s->request_size+=n-1U;
    s->expected^=1U;
    if(pcb&0x10U) return one(s,(uint8_t)(0xa2U|s->expected));
    s->tx_number=number;s->phase=WeaveIsoApp;s->frame_size=0;
    wipe(s->frame,sizeof(s->frame));return WeaveIsoRequest;
}
WeaveIsoAction weave_isodep_reply(WeaveIsoDep* s,const uint8_t* response,size_t n) {
    if(s->phase!=WeaveIsoApp||s->wtx_pending||!response||!n||n>sizeof(s->response)||!s->payload_max)
        return WeaveIsoAbort;
    wipe(s->request,sizeof(s->request));s->request_size=0;
    memcpy(s->response,response,n);s->response_size=n;s->response_offset=0;
    return next_response(s);
}
WeaveIsoAction weave_isodep_wtx(WeaveIsoDep* s) {
    if(s->phase!=WeaveIsoApp||s->wtx_pending)return WeaveIsoAbort;
    one(s,0xf2);s->frame[1]=WEAVE_ISODEP_WTX_MULTIPLIER;s->frame_size=2;s->wtx_pending=true;
    return WeaveIsoSend;
}
