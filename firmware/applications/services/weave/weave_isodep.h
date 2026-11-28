#pragma once
#include "weave_nfc.h"
/* CID/NAD-free ISO-DEP PICC state machine, used only by the Weave broker.
 * Input/output frames exclude CRC, which the native 3A listener handles. */
#define WEAVE_ISODEP_FRAME_MAX 254U
#define WEAVE_ISODEP_WTX_MULTIPLIER 8U
typedef enum { WeaveIsoIgnore, WeaveIsoSend, WeaveIsoRequest, WeaveIsoWait,
               WeaveIsoHalt, WeaveIsoAbort } WeaveIsoAction;
typedef enum { WeaveIsoReceiving, WeaveIsoApp, WeaveIsoAck } WeaveIsoPhase;
typedef struct {
    uint8_t request[WEAVE_NFC_PACKET_MAX];
    uint8_t response[WEAVE_NFC_PACKET_MAX];
    uint8_t frame[WEAVE_ISODEP_FRAME_MAX];
    size_t request_size, response_size, response_offset, frame_size, payload_max;
    uint8_t expected, tx_number;
    bool wtx_pending, started, replay_ack;
    WeaveIsoPhase phase;
} WeaveIsoDep;
void weave_isodep_reset(WeaveIsoDep* state);
WeaveIsoAction weave_isodep_input(WeaveIsoDep* state, const uint8_t* frame, size_t size, size_t peer_frame_size);
WeaveIsoAction weave_isodep_reply(WeaveIsoDep* state, const uint8_t* response, size_t size);
WeaveIsoAction weave_isodep_wtx(WeaveIsoDep* state);
