#include "came_atomo.h"
#include <lib/toolbox/manchester_decoder.h>
#include "../blocks/const.h"
#include "../blocks/decoder.h"
#include "../blocks/encoder.h"
#include "../blocks/generic.h"
#include "../blocks/math.h"

#define TAG "SubGhzProtocoCameAtomo"

#define SUBGHZ_NO_CAME_ATOMO_RAINBOW_TABLE 0xFFFFFFFFFFFFFFFF

static const SubGhzBlockConst subghz_protocol_came_atomo_const = {
    .te_short = 600,
    .te_long = 1200,
    .te_delta = 250,
    .min_count_bit_for_found = 62,
};

struct SubGhzProtocolDecoderCameAtomo {
    SubGhzProtocolDecoderBase base;

    SubGhzBlockDecoder decoder;
    SubGhzBlockGeneric generic;

    ManchesterState manchester_saved_state;
    const char* came_atomo_rainbow_table_file_name;
};

struct SubGhzProtocolEncoderCameAtomo {
    SubGhzProtocolEncoderBase base;

    SubGhzProtocolBlockEncoder encoder;
    SubGhzBlockGeneric generic;
};

typedef enum {
    CameAtomoDecoderStepReset = 0,
    CameAtomoDecoderStepDecoderData,
} CameAtomoDecoderStep;

const SubGhzProtocolDecoder subghz_protocol_came_atomo_decoder = {
    .alloc = subghz_protocol_decoder_came_atomo_alloc,
    .free = subghz_protocol_decoder_came_atomo_free,

    .feed = subghz_protocol_decoder_came_atomo_feed,
    .reset = subghz_protocol_decoder_came_atomo_reset,

    .get_hash_data = subghz_protocol_decoder_came_atomo_get_hash_data,
    .serialize = subghz_protocol_decoder_came_atomo_serialize,
    .deserialize = subghz_protocol_decoder_came_atomo_deserialize,
    .get_string = subghz_protocol_decoder_came_atomo_get_string,
};

const SubGhzProtocolEncoder subghz_protocol_came_atomo_encoder = {
    .alloc = NULL,
    .free = NULL,

    .deserialize = NULL,
    .stop = NULL,
    .yield = NULL,
};

const SubGhzProtocol subghz_protocol_came_atomo = {
    .name = SUBGHZ_PROTOCOL_CAME_ATOMO_NAME,
    .type = SubGhzProtocolTypeDynamic,
    .flag = SubGhzProtocolFlag_433 | SubGhzProtocolFlag_AM | SubGhzProtocolFlag_Decodable,

    .decoder = &subghz_protocol_came_atomo_decoder,
    .encoder = &subghz_protocol_came_atomo_encoder,
};

void* subghz_protocol_decoder_came_atomo_alloc(SubGhzEnvironment* environment) {
    SubGhzProtocolDecoderCameAtomo* instance = malloc(sizeof(SubGhzProtocolDecoderCameAtomo));
    instance->base.protocol = &subghz_protocol_came_atomo;
    instance->generic.protocol_name = instance->base.protocol->name;
    instance->came_atomo_rainbow_table_file_name =
        subghz_environment_get_came_atomo_rainbow_table_file_name(environment);
    if(instance->came_atomo_rainbow_table_file_name) {
        FURI_LOG_I(
            TAG, "Loading rainbow table from %s", instance->came_atomo_rainbow_table_file_name);
    }
    return instance;
}

void subghz_protocol_decoder_came_atomo_free(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;
    instance->came_atomo_rainbow_table_file_name = NULL;
    free(instance);
}

void subghz_protocol_decoder_came_atomo_reset(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;
    instance->decoder.parser_step = CameAtomoDecoderStepReset;
    manchester_advance(
        instance->manchester_saved_state,
        ManchesterEventReset,
        &instance->manchester_saved_state,
        NULL);
}

void subghz_protocol_decoder_came_atomo_feed(void* context, bool level, uint32_t duration) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;

    ManchesterEvent event = ManchesterEventReset;
    switch(instance->decoder.parser_step) {
    case CameAtomoDecoderStepReset:
        if((!level) && (DURATION_DIFF(duration, subghz_protocol_came_atomo_const.te_long * 60) <
                        subghz_protocol_came_atomo_const.te_delta * 40)) {
            //Found header CAME
            instance->decoder.parser_step = CameAtomoDecoderStepDecoderData;
            instance->decoder.decode_data = 0;
            instance->decoder.decode_count_bit = 1;
            manchester_advance(
                instance->manchester_saved_state,
                ManchesterEventReset,
                &instance->manchester_saved_state,
                NULL);
            manchester_advance(
                instance->manchester_saved_state,
                ManchesterEventShortLow,
                &instance->manchester_saved_state,
                NULL);
        }
        break;
    case CameAtomoDecoderStepDecoderData:
        if(!level) {
            if(DURATION_DIFF(duration, subghz_protocol_came_atomo_const.te_short) <
               subghz_protocol_came_atomo_const.te_delta) {
                event = ManchesterEventShortLow;
            } else if(
                DURATION_DIFF(duration, subghz_protocol_came_atomo_const.te_long) <
                subghz_protocol_came_atomo_const.te_delta) {
                event = ManchesterEventLongLow;
            } else if(
                duration >= ((uint32_t)subghz_protocol_came_atomo_const.te_long * 2 +
                             subghz_protocol_came_atomo_const.te_delta)) {
                if(instance->decoder.decode_count_bit ==
                   subghz_protocol_came_atomo_const.min_count_bit_for_found) {
                    instance->generic.data = instance->decoder.decode_data;
                    instance->generic.data_count_bit = instance->decoder.decode_count_bit;
                    if(instance->base.callback)
                        instance->base.callback(&instance->base, instance->base.context);
                }
                instance->decoder.decode_data = 0;
                instance->decoder.decode_count_bit = 1;
                manchester_advance(
                    instance->manchester_saved_state,
                    ManchesterEventReset,
                    &instance->manchester_saved_state,
                    NULL);
                manchester_advance(
                    instance->manchester_saved_state,
                    ManchesterEventShortLow,
                    &instance->manchester_saved_state,
                    NULL);
            } else {
                instance->decoder.parser_step = CameAtomoDecoderStepReset;
            }
        } else {
            if(DURATION_DIFF(duration, subghz_protocol_came_atomo_const.te_short) <
               subghz_protocol_came_atomo_const.te_delta) {
                event = ManchesterEventShortHigh;
            } else if(
                DURATION_DIFF(duration, subghz_protocol_came_atomo_const.te_long) <
                subghz_protocol_came_atomo_const.te_delta) {
                event = ManchesterEventLongHigh;
            } else {
                instance->decoder.parser_step = CameAtomoDecoderStepReset;
            }
        }
        if(event != ManchesterEventReset) {
            bool data;
            bool data_ok = manchester_advance(
                instance->manchester_saved_state, event, &instance->manchester_saved_state, &data);

            if(data_ok) {
                instance->decoder.decode_data = (instance->decoder.decode_data << 1) | !data;
                instance->decoder.decode_count_bit++;
            }
        }
        break;
    }
}

/* Rainbow table : index et chemin fichier complet */
static uint64_t subghz_protocol_came_atomo_get_magic_xor_in_file(
    const char* file_name,
    uint8_t number_atomo_magic_xor) {
    if(!strcmp(file_name, "")) return SUBGHZ_NO_CAME_ATOMO_RAINBOW_TABLE;

    uint8_t buffer[sizeof(uint64_t)] = {0};
    uint32_t address = number_atomo_magic_xor * sizeof(uint64_t);
    uint64_t atomo_magic_xor = 0;

    if(subghz_keystore_raw_get_data(file_name, address, buffer, sizeof(uint64_t))) {
        for(size_t i = 0; i < sizeof(uint64_t); i++) {
            atomo_magic_xor = (atomo_magic_xor << 8) | buffer[i];
        }
    } else {
        atomo_magic_xor = SUBGHZ_NO_CAME_ATOMO_RAINBOW_TABLE;
    }
    return atomo_magic_xor;
}

/* Décodage via rainbow table au chemin complet */
static void subghz_protocol_came_atomo_remote_controller(
    SubGhzBlockGeneric* instance,
    const char* file_name) {
    /* Maintien bouton : 99 parcelles à compteur croissant */
    /* Nouvelle press : compteur chiffré incrémenté */
    /* 0x1FAF compteur parcelle ; F0A press ; ED0F7D9E serial ; F bouton */
    /* Index table : ((counter XOR 0x185F)>>4)+1 modulo32 */
    /* Nibble compteur inférieur incrémenté tous les huit samples */
    /* Exemple 1CF6 XOR185F=04A9 ; index=(74+1)%32=11 */
    /* Payload 931DFB16C0B1 XOR atomo_magic_xor[11]=EF3ED0F7D9EF */
    /* Résultat : EF3 compteur, ED0F7D9E serial, F bouton */

    uint16_t parcel_counter = instance->data >> 48;
    parcel_counter = parcel_counter ^ 0x185F;
    parcel_counter >>= 4;
    uint8_t ind = (parcel_counter + 1) % 32;
    uint64_t temp_data = instance->data & 0x0000FFFFFFFFFFFF;
    uint64_t atomo_magic_xor = subghz_protocol_came_atomo_get_magic_xor_in_file(file_name, ind);

    if(atomo_magic_xor != SUBGHZ_NO_CAME_ATOMO_RAINBOW_TABLE) {
        temp_data = temp_data ^ atomo_magic_xor;
        instance->cnt = temp_data >> 36;
        instance->serial = (temp_data >> 4) & 0x000FFFFFFFF;
        instance->btn = temp_data & 0xF;
    } else {
        instance->cnt = 0;
        instance->serial = 0;
        instance->btn = 0;
    }
}

uint8_t subghz_protocol_decoder_came_atomo_get_hash_data(void* context) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;
    return subghz_protocol_blocks_get_hash_data(
        &instance->decoder, (instance->decoder.decode_count_bit / 8) + 1);
}

SubGhzProtocolStatus subghz_protocol_decoder_came_atomo_serialize(
    void* context,
    FlipperFormat* flipper_format,
    SubGhzRadioPreset* preset) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;
    return subghz_block_generic_serialize(&instance->generic, flipper_format, preset);
}

SubGhzProtocolStatus
    subghz_protocol_decoder_came_atomo_deserialize(void* context, FlipperFormat* flipper_format) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;
    return subghz_block_generic_deserialize_check_count_bit(
        &instance->generic,
        flipper_format,
        subghz_protocol_came_atomo_const.min_count_bit_for_found);
}

void subghz_protocol_decoder_came_atomo_get_string(void* context, FuriString* output) {
    furi_assert(context);
    SubGhzProtocolDecoderCameAtomo* instance = context;
    subghz_protocol_came_atomo_remote_controller(
        &instance->generic, instance->came_atomo_rainbow_table_file_name);
    uint32_t code_found_hi = instance->generic.data >> 32;
    uint32_t code_found_lo = instance->generic.data & 0x00000000ffffffff;

    furi_string_cat_printf(
        output,
        "%s %db\r\n"
        "Key:0x%lX%08lX\r\n"
        "Sn:0x%08lX  Btn:0x%01X\r\n"
        "Cnt:0x%03lX\r\n",

        instance->generic.protocol_name,
        instance->generic.data_count_bit,
        code_found_hi,
        code_found_lo,
        instance->generic.serial,
        instance->generic.btn,
        instance->generic.cnt);
}
