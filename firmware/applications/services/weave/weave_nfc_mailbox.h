#pragma once
#include "weave_nfc.h"
typedef struct {
    WeaveNfcRequest request;
    uint32_t sequence;
    uint8_t buffer[WEAVE_NFC_PACKET_MAX];
    size_t response_size;
    bool active, delivered, ready;
} WeaveNfcMailbox;
/* Pure state machine. Caller serializes access and controls deadline. */
void weave_nfc_mailbox_clear(WeaveNfcMailbox* box);
bool weave_nfc_mailbox_begin(WeaveNfcMailbox* box, const uint8_t* data, size_t size, size_t response_max);
WeaveNfcResult weave_nfc_mailbox_receive(WeaveNfcMailbox* box, WeaveNfcRequest* request, uint8_t* data, size_t capacity);
WeaveNfcResult weave_nfc_mailbox_respond(WeaveNfcMailbox* box, uint32_t id, const uint8_t* data, size_t size);
size_t weave_nfc_mailbox_take(WeaveNfcMailbox* box, uint8_t* data, size_t capacity);
