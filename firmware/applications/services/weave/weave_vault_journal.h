#pragma once
#include "weave_security.h"
#include <storage/storage.h>
/* Internal two-slot storage envelope. Digest detects torn writes, not attackers.
 * AAD/GCM validation remains the responsibility of the vault. */
#define WEAVE_VAULT_RECORD_MAX (52U + WEAVE_SECRET_MAX_SIZE)
WeaveSecurityResult weave_vault_read(Storage* storage, const char* path,
    uint8_t* record, size_t* size);
WeaveSecurityResult weave_vault_write(Storage* storage, const char* path,
    const uint8_t* record, size_t size, bool create_only);
WeaveSecurityResult weave_vault_remove(Storage* storage, const char* path);
