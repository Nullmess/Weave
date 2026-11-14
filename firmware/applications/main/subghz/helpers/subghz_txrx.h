#pragma once

#include "subghz_types.h"

#include <lib/subghz/subghz_worker.h>
#include <lib/subghz/subghz_setting.h>
#include <lib/subghz/receiver.h>
#include <lib/subghz/transmitter.h>
#include <lib/subghz/protocols/raw.h>
#include <lib/subghz/devices/devices.h>

typedef struct SubGhzTxRx SubGhzTxRx;

typedef void (*SubGhzTxRxNeedSaveCallback)(void* context);

typedef enum {
    SubGhzTxRxStartTxStateOk,
    SubGhzTxRxStartTxStateErrorOnlyRx,
    SubGhzTxRxStartTxStateErrorParserOthers,
} SubGhzTxRxStartTxState;

SubGhzTxRx* subghz_txrx_alloc(void);

void subghz_txrx_free(SubGhzTxRx* instance);

bool subghz_txrx_is_database_loaded(SubGhzTxRx* instance);

/* Fréquence en Hz */

void subghz_txrx_set_preset(
    SubGhzTxRx* instance,
    const char* preset_name,
    uint32_t frequency,
    uint8_t* preset_data,
    size_t preset_data_size);

const char* subghz_txrx_get_preset_name(SubGhzTxRx* instance, const char* preset);

SubGhzRadioPreset subghz_txrx_get_preset(SubGhzTxRx* instance);

void subghz_txrx_get_frequency_and_modulation(
    SubGhzTxRx* instance,
    FuriString* frequency,
    FuriString* modulation);

SubGhzTxRxStartTxState subghz_txrx_tx_start(SubGhzTxRx* instance, FlipperFormat* flipper_format);

void subghz_txrx_rx_start(SubGhzTxRx* instance);

void subghz_txrx_stop(SubGhzTxRx* instance);

void subghz_txrx_sleep(SubGhzTxRx* instance);

void subghz_txrx_hopper_update(SubGhzTxRx* instance);

SubGhzHopperState subghz_txrx_hopper_get_state(SubGhzTxRx* instance);

void subghz_txrx_hopper_set_state(SubGhzTxRx* instance, SubGhzHopperState state);

void subghz_txrx_hopper_unpause(SubGhzTxRx* instance);

void subghz_txrx_hopper_pause(SubGhzTxRx* instance);

void subghz_txrx_speaker_on(SubGhzTxRx* instance);

void subghz_txrx_speaker_off(SubGhzTxRx* instance);

void subghz_txrx_speaker_mute(SubGhzTxRx* instance);

void subghz_txrx_speaker_unmute(SubGhzTxRx* instance);

void subghz_txrx_speaker_set_state(SubGhzTxRx* instance, SubGhzSpeakerState state);

SubGhzSpeakerState subghz_txrx_speaker_get_state(SubGhzTxRx* instance);

bool subghz_txrx_load_decoder_by_name_protocol(SubGhzTxRx* instance, const char* name_protocol);

SubGhzProtocolDecoderBase* subghz_txrx_get_decoder(SubGhzTxRx* instance);

void subghz_txrx_set_need_save_callback(
    SubGhzTxRx* instance,
    SubGhzTxRxNeedSaveCallback callback,
    void* context);

FlipperFormat* subghz_txrx_get_fff_data(SubGhzTxRx* instance);

SubGhzSetting* subghz_txrx_get_setting(SubGhzTxRx* instance);

bool subghz_txrx_protocol_is_serializable(SubGhzTxRx* instance);

bool subghz_txrx_protocol_is_transmittable(SubGhzTxRx* instance, bool check_type);

void subghz_txrx_receiver_set_filter(SubGhzTxRx* instance, SubGhzProtocolFlag filter);

void subghz_txrx_set_rx_calback(
    SubGhzTxRx* instance,
    SubGhzReceiverCallback callback,
    void* context);

/* Callback Raw en fin de transfert */

void subghz_txrx_set_raw_file_encoder_worker_callback_end(
    SubGhzTxRx* instance,
    SubGhzProtocolEncoderRAWCallbackEnd callback,
    void* context);

bool subghz_txrx_radio_device_is_external_connected(SubGhzTxRx* instance, const char* name);

SubGhzRadioDeviceType
    subghz_txrx_radio_device_set(SubGhzTxRx* instance, SubGhzRadioDeviceType radio_device_type);

SubGhzRadioDeviceType subghz_txrx_radio_device_get(SubGhzTxRx* instance);

float subghz_txrx_radio_device_get_rssi(SubGhzTxRx* instance);

const char* subghz_txrx_radio_device_get_name(SubGhzTxRx* instance);

bool subghz_txrx_radio_device_is_frequecy_valid(SubGhzTxRx* instance, uint32_t frequency);
