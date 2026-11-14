#include "somfy_keytis.h"
#include <lib/toolbox/manchester_decoder.h>

#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define TAG "SubGhzProtocolSomfyKeytis"

static const SubGhzBlockConst subghz_protocol_somfy_keytis_const = {
    .te_short = 640,
    .te_long = 1280,
    .te_delta = 250,
    .min_count_bit_for_found = 80,
};

struct SubGhzProtocolDecoderSomfyKeytis {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    uint16_t header_count;
    ManchesterState manchester_saved_state;
    uint32_t press_duration_counter;
};

struct SubGhzProtocolEncoderSomfyKeytis {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    SomfyKeytisDecoderStepReset = 0,
    SomfyKeytisDecoderStepCheckPreambula,
    SomfyKeytisDecoderStepFoundPreambula,
    SomfyKeytisDecoderStepStartDecode,
    SomfyKeytisDecoderStepDecoderData,
} SomfyKeytisDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_somfy_keytis_decoder = {
    .alloc = subghz_protocol_decoder_somfy_keytis_alloc,
    .free = subghz_protocol_decoder_somfy_keytis_free,

    .feed = subghz_protocol_decoder_somfy_keytis_feed,
    .reset = subghz_protocol_decoder_somfy_keytis_reset,

    .get_hash_data = subghz_protocol_decoder_somfy_keytis_get_hash_data,
    .serialize = subghz_protocol_decoder_somfy_keytis_serialize,
    .deserialize = subghz_protocol_decoder_somfy_keytis_deserialize,
    .get_string = subghz_protocol_decoder_somfy_keytis_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_somfy_keytis_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol subghz_protocol_somfy_keytis = {
    .name = SUBGHZ_PROTOCOL_SOMFY_KEYTIS_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_868 | SubGhzProtocolFlag_AM |
            SubGhzProtocolFlag_Decodable,

    .decoder = &subghz_protocol_somfy_keytis_decoder,
    .encoder = &subghz_protocol_somfy_keytis_encoder,
};

void* subghz_protocol_decoder_somfy_keytis_alloc(SubGhzEnvironment* environment) {
    UNUSED(environment);
    SubGhzProtocolDecoderSomfyKeytis* instance = malloc(sizeof(SubGhzProtocolDecoderSomfyKeytis));
    instance->base.protocol = &subghz_protocol_somfy_keytis;
    instance->generic.protocol_name = instance->base.protocol->name;

    return instance;
}

void subghz_protocol_decoder_somfy_keytis_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;
    free(instance);
}

void subghz_protocol_decoder_somfy_keytis_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;
    instance->decoder.parser_step = SomfyKeytisDecoderStepReset;
    manchester_advance(
        instance->manchester_saved_state,
        ManchesterEventReset,
        &instance->manchester_saved_state,
        NULL);
}

static uint8_t subghz_protocol_somfy_keytis_crc(uint64_t data) {
    uint8_t crc = 0;
    data &= 0xFFF0FFFFFFFFFF;
    for(uint8_t i = 0; i < 56; i += 8) {
        crc = crc ^ data >> i ^ (data >> (i + 4));
    }
    return crc & 0xf;
}

void subghz_protocol_decoder_somfy_keytis_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;

    ManchesterEvent event = ManchesterEventReset;
    switch(instance->decoder.parser_step) {
    case SomfyKeytisDecoderStepReset:
        if((level) && DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_short * 4) <
                          subghz_protocol_somfy_keytis_const.te_delta * 4) {
            instance->decoder.parser_step = SomfyKeytisDecoderStepFoundPreambula;
            instance->header_count++;
        }
        break;
    case SomfyKeytisDecoderStepFoundPreambula:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_short * 4) <
                        subghz_protocol_somfy_keytis_const.te_delta * 4)) {
            instance->decoder.parser_step = SomfyKeytisDecoderStepCheckPreambula;
        } else {
            instance->header_count = 0;
            instance->decoder.parser_step = SomfyKeytisDecoderStepReset;
        }
        break;
    case SomfyKeytisDecoderStepCheckPreambula:
        if(level) {
            if(DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_short * 4) <
               subghz_protocol_somfy_keytis_const.te_delta * 4) {
                instance->decoder.parser_step = SomfyKeytisDecoderStepFoundPreambula;
                instance->header_count++;
            } else if(
                (instance->header_count > 1) &&
                (DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_short * 7) <
                 subghz_protocol_somfy_keytis_const.te_delta * 4)) {
                instance->decoder.parser_step = SomfyKeytisDecoderStepDecoderData;
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                instance->press_duration_counter = 0;
                manchester_advance(
                    instance->manchester_saved_state,
                    ManchesterEventReset,
                    &instance->manchester_saved_state,
                    NULL);
                manchester_advance(
                    instance->manchester_saved_state,
                    ManchesterEventLongHigh,
                    &instance->manchester_saved_state,
                    NULL);
            }
        }

        break;

    case SomfyKeytisDecoderStepDecoderData:
        if(!level) {
            if(DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_short) <
               subghz_protocol_somfy_keytis_const.te_delta) {
                event = ManchesterEventShortLow;
            } else if(
                DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_long) <
                subghz_protocol_somfy_keytis_const.te_delta) {
                event = ManchesterEventLongLow;
            } else if(
                duration >= (subghz_protocol_somfy_keytis_const.te_long +
                             subghz_protocol_somfy_keytis_const.te_delta)) {
                if(instance->decoder.decode_count_bit ==
                   subghz_protocol_somfy_keytis_const.min_count_bit_for_found) {
                    //check crc
                    uint64_t data_tmp = instance->generic.data ^ (instance->generic.data >> 8);
                    if(((data_tmp >> 40) & 0xF) == subghz_protocol_somfy_keytis_crc(data_tmp)) {
                        instance->generic.data = instance->decoder.decode_data;
                        instance->generic.data_count_bit = instance->decoder.decode_count_bit;

                        if(instance->base.callback)
                            instance->base.callback(&instance->base, instance->base.context);
                    }
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 0;
                manchester_advance(
                    instance->manchester_saved_state,
                    ManchesterEventReset,
                    &instance->manchester_saved_state,
                    NULL);
                manchester_advance(
                    instance->manchester_saved_state,
                    ManchesterEventLongHigh,
                    &instance->manchester_saved_state,
                    NULL);
                instance->decoder.parser_step = SomfyKeytisDecoderStepReset;
            } else {
                instance->decoder.parser_step = SomfyKeytisDecoderStepReset;
            }
        } else {
            if(DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_short) <
               subghz_protocol_somfy_keytis_const.te_delta) {
                event = ManchesterEventShortHigh;
            } else if(
                DURATION_DIFF(duration, subghz_protocol_somfy_keytis_const.te_long) <
                subghz_protocol_somfy_keytis_const.te_delta) {
                event = ManchesterEventLongHigh;
            } else {
                instance->decoder.parser_step = SomfyKeytisDecoderStepReset;
            }
        }
        if(event != ManchesterEventReset) {
            bool data;
            bool data_ok = manchester_advance(
                instance->manchester_saved_state, event, &instance->manchester_saved_state, &data);

            if(data_ok) {
                if(instance->decoder.decode_count_bit < 56) {
                    instance->decoder.decode_data = (instance->decoder.decode_data << 1) | data;
                } else {
                    instance->press_duration_counter = (instance->press_duration_counter << 1) |
                                                       data;
                }

                instance->decoder.decode_count_bit++;
            }
        }
        break;
    }
}

/* Décodage des données reçues */
static void subghz_protocol_somfy_keytis_check_remote_controller(SubGhzBlockGeneric* instance) {
    //https://pushstack.wordpress.com/somfy-rts-protocol/
    /* Somfy80 : quantum604 µs ; hardware sync2416, software4550 */
    /* Payload56 bits + PDC24 ; 72 parcelles par maintien */
    /* PDC counter4 1..15 puis15 ; counter8 1..72 */
    /* PDC : champs constants 11/00/0000 ; CRC XOR nibbles4..23 */
    /* Encryption key : MSB souvent A, LSB compteur ; MSB variable selon remote */
    /* Layout bouton4/CRC4/counter16 big endian/serial24 little endian */
    /* Décryptage uint64 : frame XOR(frame>>8) */
    /* CRC : XOR octets et leurs nibbles hauts, masque F */
    /* Exemple A453537C4B9855 C40019→keyA4/btnF/crc7/cnt002F/SN37D3CD */

    uint64_t data = instance->data ^ (instance->data >> 8);
    instance->btn = (data >> 48) & 0xF;
    instance->cnt = (data >> 24) & 0xFFFF;
    instance->serial = data & 0xFFFFFF;
}

/* Nom du bouton sur 4 bits */
static const char* subghz_protocol_somfy_keytis_get_name_button(uint8_t btn) {
    const char* name_btn[0x10] = {
        "Unknown",
        "0x01",
        "0x02",
        "Prog",
        "Key_1",
        "0x05",
        "0x06",
        "0x07",
        "0x08",
        "0x09",
        "0x0A",
        "0x0B",
        "0x0C",
        "0x0D",
        "0x0E",
        "0x0F"};
    return btn <= 0xf ? name_btn[btn] : name_btn[0];
}

uint8_t subghz_protocol_decoder_somfy_keytis_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_somfy_keytis_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;
    SubGhzProtocolStatus ret =
        subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
    if((ret == SubGhzProtocolStatusOk) &&
       !flipper_format_write_uint32(
           flipper_format, "Duration_Counter", &instance->press_duration_counter, 1)) {
        FURI_LOG_E(TAG, "Unable to add Duration_Counter");
        ret = SubGhzProtocolStatusErrorParserOthers;
    }
    return ret;
}

SubGhzProtocolStatus
    subghz_protocol_decoder_somfy_keytis_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;
    SubGhzProtocolStatus ret = SubGhzProtocolStatusError;
    do {
        ret = subghz_block_generic_deserialize_check_count_bit(
            &instance->generic,
            flipper_format,
            subghz_protocol_somfy_keytis_const.min_count_bit_for_found);
        if(ret != SubGhzProtocolStatusOk) {
            break;
        }
        if(!flipper_format_rewind(flipper_format)) {
            FURI_LOG_E(TAG, "Rewind error");
            ret = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
        if(!flipper_format_read_uint32(
               flipper_format,
               "Duration_Counter",
               (uint32_t*)&instance->press_duration_counter,
               1)) {
            FURI_LOG_E(TAG, "Missing Duration_Counter");
            ret = SubGhzProtocolStatusErrorParserOthers;
            break;
        }
    } while(false);

    return ret;
}

void subghz_protocol_decoder_somfy_keytis_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoderSomfyKeytis* instance = context;

    subghz_protocol_somfy_keytis_check_remote_controller(&instance->generic);

    furi_string_cat_printf(
        output,
        "%s %db\r\n"
        "%lX%08lX%06lX\r\n"
        "Sn:0x%06lX \r\n"
        "Cnt:0x%04lX\r\n"
        "Btn:%s\r\n",

        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        (uint32_t)(instance->generic.data >> 32),
        (uint32_t)instance->generic.data,
        instance->press_duration_counter,
        instance->generic.serial,
        instance->generic.cnt,
        subghz_protocol_somfy_keytis_get_name_button(instance->generic.btn));
}
