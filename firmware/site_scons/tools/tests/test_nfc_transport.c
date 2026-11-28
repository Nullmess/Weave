#include "weave_nfc_mailbox.h"
#include "weave_isodep.h"
#include "nfc/helpers/iso14443_4_layer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void test_chain(void) {
    WeaveIsoDep s;weave_isodep_reset(&s);
    uint8_t frame[254], message[WEAVE_NFC_PACKET_MAX], result[WEAVE_NFC_PACKET_MAX];
    for(size_t i=0;i<sizeof(message);i++) message[i]=(uint8_t)i;
    size_t pos=0;uint8_t number=0;
    while(pos<sizeof(message)) {
        size_t n=sizeof(message)-pos;if(n>253)n=253;
        bool more=pos+n<sizeof(message);
        frame[0]=(uint8_t)(0x02|number|(more?0x10:0));memcpy(frame+1,message+pos,n);
        assert(weave_isodep_input(&s,frame,n+1,32)==(more?WeaveIsoSend:WeaveIsoRequest));
        pos+=n;number^=1;
        if(more) {
            assert(s.frame_size==1&&s.frame[0]==(0xa2|number));
            assert(weave_isodep_input(&s,frame,n+1,32)==WeaveIsoSend&&s.request_size==pos);
        }
    }
    assert(s.request_size==sizeof(message)&&!memcmp(s.request,message,sizeof(message)));
    assert(weave_isodep_wtx(&s)==WeaveIsoSend&&s.frame[0]==0xf2&&s.frame[1]==8);
    assert(weave_isodep_input(&s,(uint8_t[]){0xf2,7},2,32)==WeaveIsoIgnore);
    assert(weave_isodep_input(&s,frame,13,32)==WeaveIsoSend&&s.frame[0]==0xf2);
    assert(weave_isodep_input(&s,(uint8_t[]){0xf2,8},2,32)==WeaveIsoWait);
    assert(weave_isodep_wtx(&s)==WeaveIsoSend);
    assert(weave_isodep_input(&s,(uint8_t[]){0xf2,8},2,32)==WeaveIsoWait);
    assert(weave_isodep_reply(&s,message,sizeof(message))==WeaveIsoSend);
    pos=0;uint8_t pcb;
    for(;;) {
        pcb=s.frame[0];const size_t n=s.frame_size-1;
        assert(s.frame_size<=30&&pos+n<=sizeof(result));
        memcpy(result+pos,s.frame+1,n);pos+=n;
        if(!(pcb&0x10))break;
        assert(weave_isodep_input(&s,(uint8_t[]){(uint8_t)(0xb2|(pcb&1))},1,32)==WeaveIsoSend&&s.frame[0]==pcb);
        assert(weave_isodep_input(&s,(uint8_t[]){(uint8_t)(0xa2|((pcb&1)^1))},1,32)==WeaveIsoSend);
    }
    assert(pos==sizeof(message)&&!memcmp(result,message,sizeof(message)));
    assert(weave_isodep_input(&s,(uint8_t[]){(uint8_t)(0xa2|(pcb&1))},1,32)==WeaveIsoSend&&s.frame[0]==pcb);
    assert(weave_isodep_input(&s,(uint8_t[]){(uint8_t)(0x02|((pcb&1)^1)),0xaa},2,32)==WeaveIsoRequest);
    assert(s.request_size==1&&s.request[0]==0xaa);
    assert(weave_isodep_input(&s,(uint8_t[]){0xc2},1,32)==WeaveIsoHalt);
    assert(!s.request_size&&!s.response_size&&s.frame[0]==0xc2);
    weave_isodep_reset(&s);
    for(size_t i=0;i<5;i++) { frame[0]=(uint8_t)(0x12|(i&1));assert(weave_isodep_input(&s,frame,sizeof(frame),256)==(i<4?WeaveIsoSend:WeaveIsoAbort)); }
    weave_isodep_reset(&s);
    for(size_t i=0;i<sizeof(s);i++) assert(((uint8_t*)&s)[i]==0);
    assert(weave_isodep_input(&s,(uint8_t[]){0x03,0xaa},2,32)==WeaveIsoIgnore);
    assert(weave_isodep_input(&s,(uint8_t[]){0x0a,0xaa},2,32)==WeaveIsoIgnore);
    puts("ISO-DEP: 1024-byte request/response chaining, R-ACK/NAK, retry, WTX, overflow, wipe PASS");
}
int main(void) {
    WeaveNfcMailbox b={0}; WeaveNfcRequest r;
    uint8_t in[WEAVE_NFC_PACKET_MAX],out[WEAVE_NFC_PACKET_MAX];memset(in,0x71,sizeof(in));
    assert(!weave_nfc_mailbox_begin(&b,in,WEAVE_NFC_PACKET_MAX+1,240));
    assert(weave_nfc_mailbox_begin(&b,in,240,13));
    assert(!weave_nfc_mailbox_begin(&b,in,1,13));
    assert(weave_nfc_mailbox_respond(&b,b.request.id,in,1)==WeaveNfcInvalid);
    assert(weave_nfc_mailbox_receive(&b,&r,out,239)==WeaveNfcInvalid);
    assert(weave_nfc_mailbox_receive(&b,&r,out,240)==WeaveNfcOk && !memcmp(in,out,240));
    uint32_t id=r.id;
    assert(weave_nfc_mailbox_receive(&b,&r,out,240)==WeaveNfcNoData);
    assert(weave_nfc_mailbox_respond(&b,id,in,14)==WeaveNfcInvalid);
    assert(weave_nfc_mailbox_respond(&b,id,in,13)==WeaveNfcOk);
    assert(weave_nfc_mailbox_respond(&b,id,in,13)==WeaveNfcExpired);
    assert(weave_nfc_mailbox_take(&b,out,12)==0);
    assert(weave_nfc_mailbox_take(&b,out,240)==13 && !memcmp(in,out,13));
    for(size_t i=0;i<sizeof(b.buffer);i++)assert(b.buffer[i]==0);
    assert(weave_nfc_mailbox_begin(&b,in,1,1));
    assert(b.request.id!=id && weave_nfc_mailbox_respond(&b,id,in,1)==WeaveNfcExpired);
    weave_nfc_mailbox_clear(&b);
    assert(weave_nfc_mailbox_receive(&b,&r,out,240)==WeaveNfcNoData);
    b.sequence=UINT32_MAX;assert(weave_nfc_mailbox_begin(&b,in,1,1) && b.request.id==1);
    Iso14443_4Layer* l=iso14443_4_layer_alloc();
    BitBuffer* a=bit_buffer_alloc(256);BitBuffer* d=bit_buffer_alloc(256);
    const uint8_t bad[][3]={{0xd0,0,0},{0xd0,0x10,0},{0x02,0,0},{0x0a,0,0},{0x06,0,0},{0x12,0xaa,0}};
    const size_t sizes[]={1,2,1,1,1,2};
    assert(iso14443_4_layer_decode_command(l,a,d)==Iso14443_4LayerResultSkip);
    for(size_t i=0;i<sizeof(sizes)/sizeof(sizes[0]);i++) {
        iso14443_4_layer_reset(l);bit_buffer_copy_bytes(a,bad[i],sizes[i]);
        assert(iso14443_4_layer_decode_command(l,a,d)==Iso14443_4LayerResultSkip);
    }
    bit_buffer_copy_bits(a,in,5);assert(iso14443_4_layer_decode_command(l,a,d)==Iso14443_4LayerResultSkip);
    iso14443_4_layer_reset(l);const uint8_t frame[]={0x02,0x00,0xa4,0x04,0x00};
    bit_buffer_copy_bytes(a,frame,sizeof(frame));
    assert(iso14443_4_layer_decode_command(l,a,d)==Iso14443_4LayerResultData);
    assert(bit_buffer_get_size_bytes(d)==4 && !memcmp(bit_buffer_get_data(d),frame+1,4));
    bit_buffer_reset(a);bit_buffer_append_byte(a,0x90);bit_buffer_append_byte(a,0);bit_buffer_reset(d);
    assert(iso14443_4_layer_encode_response(l,a,d));
    assert(bit_buffer_get_size_bytes(d)==3 && bit_buffer_get_byte(d,1)==0x90);
    bit_buffer_free(a);bit_buffer_free(d);iso14443_4_layer_free(l);
    test_chain();
    puts("NFC mailbox + native ISO-DEP parser: bounds, one-shot delivery, stale IDs, malformed frames, valid exchange PASS");
}
