/* Header interne exclu des API publiques */

#pragma once

#include "nfc_device.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief NfcDevice structure definition.
 */
struct NfcDevice {
    NfcProtocol protocol;
    NfcDeviceData* protocol_data;

    NfcLoadingCallback
        loading_callback; /* Callback de fin de chargement */
    void* loading_callback_context;
};

/* Comme get_data, mais données modifiables */

NfcDeviceData* nfc_device_get_data_ptr(const NfcDevice* instance, NfcProtocol protocol);

#ifdef __cplusplus
}
#endif
