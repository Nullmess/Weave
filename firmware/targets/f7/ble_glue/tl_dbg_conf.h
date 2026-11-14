#pragma once

#include "app_conf.h" /* Configurations dbg_trace définies dans cet include */
#include "dbg_trace.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Traces raw au format paquet HCI binaire BT */

#define TL_SHCI_CMD_DBG_EN 1 /* Trace commandes/réponses système CPU2 */
#define TL_SHCI_CMD_DBG_RAW_EN \
    0 /* Trace raw commandes/réponses système CPU2 */
#define TL_SHCI_EVT_DBG_EN 1 /* Trace events système asynchrones CPU2 */
#define TL_SHCI_EVT_DBG_RAW_EN \
    0 /* Trace raw events système asynchrones CPU2 */

#define TL_HCI_CMD_DBG_EN 1 /* Trace commandes/réponses BLE CPU2 */
#define TL_HCI_CMD_DBG_RAW_EN \
    0 /* Trace raw commandes/réponses BLE CPU2 */
#define TL_HCI_EVT_DBG_EN     1 /* Trace events BLE asynchrones CPU2 */
#define TL_HCI_EVT_DBG_RAW_EN 0 /* Trace raw events BLE asynchrones CPU2 */

#define TL_MM_DBG_EN 1 /* Trace buffer rendu à CPU2 */

/**
 * System Transport Layer
 */
#if(TL_SHCI_CMD_DBG_EN != 0)
#define TL_SHCI_CMD_DBG_MSG PRINT_MESG_DBG
#define TL_SHCI_CMD_DBG_BUF PRINT_LOG_BUFF_DBG
#else
#define TL_SHCI_CMD_DBG_MSG(...)
#define TL_SHCI_CMD_DBG_BUF(...)
#endif

#if(TL_SHCI_CMD_DBG_RAW_EN != 0)
#define TL_SHCI_CMD_DBG_RAW(_PDATA_, _SIZE_) furi_log_tx(_PDATA_, _SIZE_)
#else
#define TL_SHCI_CMD_DBG_RAW(...)
#endif

#if(TL_SHCI_EVT_DBG_EN != 0)
#define TL_SHCI_EVT_DBG_MSG PRINT_MESG_DBG
#define TL_SHCI_EVT_DBG_BUF PRINT_LOG_BUFF_DBG
#else
#define TL_SHCI_EVT_DBG_MSG(...)
#define TL_SHCI_EVT_DBG_BUF(...)
#endif

#if(TL_SHCI_EVT_DBG_RAW_EN != 0)
#define TL_SHCI_EVT_DBG_RAW(_PDATA_, _SIZE_) furi_log_tx(_PDATA_, _SIZE_)
#else
#define TL_SHCI_EVT_DBG_RAW(...)
#endif

/**
 * BLE Transport Layer
 */
#if(TL_HCI_CMD_DBG_EN != 0)
#define TL_HCI_CMD_DBG_MSG PRINT_MESG_DBG
#define TL_HCI_CMD_DBG_BUF PRINT_LOG_BUFF_DBG
#else
#define TL_HCI_CMD_DBG_MSG(...)
#define TL_HCI_CMD_DBG_BUF(...)
#endif

#if(TL_HCI_CMD_DBG_RAW_EN != 0)
#define TL_HCI_CMD_DBG_RAW(_PDATA_, _SIZE_) furi_log_tx(_PDATA_, _SIZE_)
#else
#define TL_HCI_CMD_DBG_RAW(...)
#endif

#if(TL_HCI_EVT_DBG_EN != 0)
#define TL_HCI_EVT_DBG_MSG PRINT_MESG_DBG
#define TL_HCI_EVT_DBG_BUF PRINT_LOG_BUFF_DBG
#else
#define TL_HCI_EVT_DBG_MSG(...)
#define TL_HCI_EVT_DBG_BUF(...)
#endif

#if(TL_HCI_EVT_DBG_RAW_EN != 0)
#define TL_HCI_EVT_DBG_RAW(_PDATA_, _SIZE_) furi_log_tx(_PDATA_, _SIZE_)
#else
#define TL_HCI_EVT_DBG_RAW(...)
#endif

/**
 * Memory Manager - Released buffer tracing
 */
#if(TL_MM_DBG_EN != 0)
#define TL_MM_DBG_MSG PRINT_MESG_DBG
#else
#define TL_MM_DBG_MSG(...)
#endif

#ifdef __cplusplus
}
#endif
