/** Generic platform feature discovery, independent of application protocols.
 * Capability bits describe implemented interfaces, not hardware availability.
 * State bits are a point-in-time snapshot; acquisition can still return BUSY.
 */
#pragma once
#include "weave.h"
#ifdef __cplusplus
extern "C" {
#endif
#define WEAVE_PLATFORM_API_VERSION 1U
#define WEAVE_CAP_NETWORK (1UL << 0)
#define WEAVE_CAP_USB_HID (1UL << 1) /* USB v2 */
#define WEAVE_CAP_USB_CCID (1UL << 2)
#define WEAVE_CAP_USB_COMPOSITE (1UL << 3)
#define WEAVE_CAP_SECURITY_V4 (1UL << 4)
#define WEAVE_CAP_PERMISSIONS_V2 (1UL << 5)
#define WEAVE_CAP_KEYSTORE (1UL << 6)
#define WEAVE_CAP_VAULT_RECOVERY (1UL << 7)
#define WEAVE_CAP_BLE_GATT (1UL << 8)
/* Strong security features are reserved, not advertised by this firmware. */
#define WEAVE_CAP_HARDWARE_ANTI_ROLLBACK (1UL << 9)
#define WEAVE_CAP_MEMORY_ISOLATION (1UL << 10)
#define WEAVE_CAP_NFC_ISODEP (1UL << 11)
#define WEAVE_STATE_GATEWAY_USB (1UL << 0)
#define WEAVE_STATE_GATEWAY_BLE (1UL << 1)
#define WEAVE_STATE_USB_CDC_AVAILABLE (1UL << 2)
uint32_t weave_platform_capabilities(void);
uint32_t weave_platform_state(Weave* weave);
#ifdef __cplusplus
}
#endif
