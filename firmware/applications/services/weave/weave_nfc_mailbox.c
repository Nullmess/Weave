#include "weave_nfc_mailbox.h"
#include <string.h>
void weave_nfc_mailbox_clear(WeaveNfcMailbox* box) {
    volatile uint8_t* p = (volatile uint8_t*)box->buffer;
    for(size_t n=sizeof(box->buffer);n;n--) *p++=0;
    box->request = (WeaveNfcRequest){0};
    box->response_size = 0;
    box->active = box->delivered = box->ready = false;
}
bool weave_nfc_mailbox_begin(WeaveNfcMailbox* box, const uint8_t* data, size_t size, size_t response_max) {
    if(box->active || !data || !size || size > WEAVE_NFC_PACKET_MAX ||
       !response_max || response_max > WEAVE_NFC_PACKET_MAX) return false;
    weave_nfc_mailbox_clear(box);
    if(++box->sequence == 0) ++box->sequence;
    box->request=(WeaveNfcRequest){.id=box->sequence,.size=size,.response_max=response_max};
    memcpy(box->buffer,data,size);
    box->active=true;
    return true;
}
WeaveNfcResult weave_nfc_mailbox_receive(WeaveNfcMailbox* box, WeaveNfcRequest* request, uint8_t* data, size_t capacity) {
    if(!request || !data) return WeaveNfcInvalid;
    *request=(WeaveNfcRequest){0};
    if(!box->active || box->delivered) return WeaveNfcNoData;
    if(capacity<box->request.size) return WeaveNfcInvalid;
    *request=box->request;memcpy(data,box->buffer,request->size);box->delivered=true;
    return WeaveNfcOk;
}
WeaveNfcResult weave_nfc_mailbox_respond(WeaveNfcMailbox* box, uint32_t id, const uint8_t* data, size_t size) {
    if(!id || !data || !size || size > WEAVE_NFC_PACKET_MAX) return WeaveNfcInvalid;
    if(!box->active || box->request.id!=id || box->ready) return WeaveNfcExpired;
    if(!box->delivered || size>box->request.response_max) return WeaveNfcInvalid;
    volatile uint8_t* p=(volatile uint8_t*)box->buffer;
    for(size_t n=sizeof(box->buffer);n;n--) *p++=0;
    memcpy(box->buffer,data,size);box->response_size=size;box->ready=true;
    return WeaveNfcOk;
}
size_t weave_nfc_mailbox_take(WeaveNfcMailbox* box, uint8_t* data, size_t capacity) {
    if(!box->active || !box->ready || !data || capacity<box->response_size) return 0;
    const size_t size=box->response_size;memcpy(data,box->buffer,size);weave_nfc_mailbox_clear(box);return size;
}
