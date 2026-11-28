/** Weave permissions: explicit on-device consent, deny-by-default.
 * Grants are RAM-only, per Furi application ID and per capability.
 * This is cooperative API access control, not an OS memory sandbox: native
 * FAPs share an address space and may call other firmware interfaces.
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define WEAVE_PERMISSIONS_API_VERSION 2U
typedef enum {
    WeavePermissionHttp = 0,
    WeavePermissionSocket,
    WeavePermissionUpload,
    WeavePermissionVault,
    WeavePermissionUsb,
    WeavePermissionKeystore,
    WeavePermissionBle,
    WeavePermissionNfc,
    WeavePermissionCount,
} WeavePermission;
/* Blocks for on-device consent if not yet granted. Returns false on Deny/Back,
 * invalid app identity, unavailable UI, or exhausted grant table.
 * Never invoke from a Weave callback or with a service mutex held. */
bool weave_permission_check(WeavePermission capability);
/* Query does not prompt. */
bool weave_permission_has(WeavePermission capability);
/* Revoke all RAM grants and attempt to release the calling app's USB, BLE and NFC leases.
 * No effect on others. Serialize with that app's acquire/check operations. */
void weave_permission_revoke_self(void);
#ifdef __cplusplus
}
#endif
