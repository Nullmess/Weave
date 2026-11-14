#pragma once
#include "subghz_types.h"
#include "subghz_txrx.h"

SubGhzProtocolStatus subghz_txrx_gen_data_protocol(
    void* context,
    const char* preset_name,
    uint32_t frequency,
    const char* protocol_name,
    uint64_t key,
    uint32_t bit);

/* Génération données protocole et TE ; fréquence Hz */

SubGhzProtocolStatus subghz_txrx_gen_data_protocol_and_te(
    SubGhzTxRx* instance,
    const char* preset_name,
    uint32_t frequency,
    const char* protocol_name,
    uint64_t key,
    uint32_t bit,
    uint32_t te);

SubGhzProtocolStatus subghz_txrx_gen_keeloq_protocol(
    SubGhzTxRx* instance,
    const char* name_preset,
    uint32_t frequency,
    const char* name_sysmem,
    uint32_t serial,
    uint8_t btn,
    uint16_t cnt);

SubGhzProtocolStatus subghz_txrx_gen_secplus_v2_protocol(
    SubGhzTxRx* instance,
    const char* name_preset,
    uint32_t frequency,
    uint32_t serial,
    uint8_t btn,
    uint32_t cnt);

SubGhzProtocolStatus subghz_txrx_gen_secplus_v1_protocol(
    SubGhzTxRx* instance,
    const char* name_preset,
    uint32_t frequency);
