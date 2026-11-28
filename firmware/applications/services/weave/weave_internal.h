#pragma once
#include <stdbool.h>
#include "weave.h"
bool weave_network_release_owner(Weave* weave, const char* appid, uint32_t timeout_ms);
void weave_network_finish_owner(Weave* weave, const char* appid);
/* Internal lifecycle hooks. Not FAP imports. */
const char* weave_caller_appid(void);
bool weave_usb_release_app(const char* appid);
void weave_usb_reap_orphan(void);
void weave_ble_broker_init(void);
bool weave_ble_release_app(const char* appid);
void weave_ble_reap_orphan(void);
void weave_nfc_broker_init(void);
bool weave_nfc_release_app(const char* appid);

#include "weave_security.h"
WeaveSecurityResult weave_vault_store_internal(bool keystore, bool create_only,
    const char* ns, const char* name, const uint8_t* password, size_t password_size,
    const uint8_t* value, size_t value_size);
WeaveSecurityResult weave_vault_load_internal(bool keystore, const char* ns, const char* name,
    const uint8_t* password, size_t password_size, uint8_t* output, size_t capacity,
    size_t* output_size);
WeaveSecurityResult weave_vault_delete_internal(bool keystore, const char* ns, const char* name);
