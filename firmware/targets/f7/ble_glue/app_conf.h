#pragma once

#include <ble/core/ble_defs.h>

#define CFG_TX_POWER (0x19) /* +0dBm */

#define CFG_IDENTITY_ADDRESS GAP_PUBLIC_ADDR

/* Authentification I/O */

#define CFG_ENCRYPTION_KEY_SIZE_MAX (16)
#define CFG_ENCRYPTION_KEY_SIZE_MIN (16)

/* Capacités I/O */

#define CFG_IO_CAPABILITY IO_CAP_DISPLAY_YES_NO

/* Support Secure Connections */

/* STM32WB SC_Support=2 is LE Secure Connections-only mode (ST AN5289).
 * Override WEAVE_ALLOW_LEGACY_BLE_PAIRING at build time only if an older
 * companion requires it; legacy pairing offers weaker passive protection. */
#ifndef WEAVE_ALLOW_LEGACY_BLE_PAIRING
#define WEAVE_ALLOW_LEGACY_BLE_PAIRING 0
#endif
#define CFG_SC_SUPPORT (WEAVE_ALLOW_LEGACY_BLE_PAIRING ? SC_PAIRING_OPTIONAL : 2)

/* Configuration PHY */

#define ALL_PHYS_PREFERENCE 0x00
#define RX_2M_PREFERRED     0x02
#define TX_2M_PREFERRED     0x02
#define TX_1M               0x01
#define TX_2M               0x02
#define RX_1M               0x01
#define RX_2M               0x02

/******************************************************************************
 * BLE Stack
 ******************************************************************************/
/* Connexions simultanées : 1..8 */

#define CFG_BLE_NUM_LINK 2

/* Services max : deux GAP/GATT automatiques + services utilisateur */

#define CFG_BLE_NUM_GATT_SERVICES 8

/* Attributs max : neuf automatiques + attributs utilisateur hors services */

#define CFG_BLE_NUM_GATT_ATTRIBUTES 68

/**
 * Maximum supported ATT_MTU size
 */
#define CFG_BLE_MAX_ATT_MTU (256 + 128 + 16 + 8 + 4 + 2)

/* Somme par attribut : taille valeur +5 UUID16 ou +19 UUID128 */
/* Ajouter2 server descriptor, 2*DTM_NUM_LINK client descriptor */
/* Ajouter2 extended properties si présentes, en octets */

#define CFG_BLE_ATT_VALUE_ARRAY_SIZE (1344)

/* Taille Write List en nombre packets */

#define CFG_BLE_PREPARE_WRITE_LIST_SIZE BLE_PREP_WRITE_X_ATT(CFG_BLE_MAX_ATT_MTU)

/* Nombre de blocs mémoire alloués */

#define CFG_BLE_MBLOCK_COUNT \
    (BLE_MBLOCKS_CALC(CFG_BLE_PREPARE_WRITE_LIST_SIZE, CFG_BLE_MAX_ATT_MTU, CFG_BLE_NUM_LINK))

/* Extended packet length : 0 ou 1 */

#define CFG_BLE_DATA_LENGTH_EXTENSION 1

/* Précision sleep clock en mode slave, ppm */

#define CFG_BLE_SLAVE_SCA 500

/* Précision sleep clock master en ppm : */
/* 0 : 251-500 ; 1 : 151-250 ; 2 : 101-150 ; 3 : 76-100 */
/* 4 : 51-75 ; 5 : 31-50 ; 6 : 21-30 ; 7 : 0-20 */

#define CFG_BLE_MASTER_SCA 0

/* Clock RF low speed : 1 HSE/32/32, 0 crystal low speed sans calibration */

#define CFG_BLE_LSE_SOURCE                                                        \
    SHCI_C2_BLE_INIT_CFG_BLE_LS_CLK_LSE | SHCI_C2_BLE_INIT_CFG_BLE_LS_OTHER_DEV | \
        SHCI_C2_BLE_INIT_CFG_BLE_LS_CALIB

/* Startup crystal16/32 MHz en unités 625/256 µs */

#define CFG_BLE_HSE_STARTUP_TIME 0x148

/* Durée event connexion slave en unités 625/256 µs */

#define CFG_BLE_MAX_CONN_EVENT_LENGTH (0xFFFFFFFF)

/**
 * Viterbi Mode
 * 1 : enabled
 * 0 : disabled
 */
#define CFG_BLE_VITERBI_MODE 1

/* Flags bit0 LL only/host ; bit1 retire/active service-change descriptor */
/* bit2 device name readonly/RW ; bit3 extended advertising non supporté */
/* bit4 CS algorithm2 ; bit7 power class1/2-3 */
/* Autres bits réservés, obligatoirement zéro */

#define CFG_BLE_OPTIONS                                                                 \
    (SHCI_C2_BLE_INIT_OPTIONS_LL_HOST | SHCI_C2_BLE_INIT_OPTIONS_WITH_SVC_CHANGE_DESC | \
     SHCI_C2_BLE_INIT_OPTIONS_DEVICE_NAME_RO | SHCI_C2_BLE_INIT_OPTIONS_EXT_ADV |       \
     SHCI_C2_BLE_INIT_OPTIONS_CS_ALGO2 | SHCI_C2_BLE_INIT_OPTIONS_POWER_CLASS_2_3)

/* Queue doit contenir events async entre commande HCI et réponse */
/* Payload27 : place pour trois events255 octets */
/* Queue saturée avant CC/CS : hang possible */
/* TL_BLE_HCI_ToNot signale absence réponse après30 s par défaut */

#define CFG_TLBLE_EVT_QUEUE_LENGTH 5
/* Payload HCI <=255 octets ; conserver255 avec firmware wireless actuel */

#define CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE \
    255 /* 255 avec memory manager et mailbox */

#define TL_BLE_EVENT_FRAME_SIZE (TL_EVT_HDR_SIZE + CFG_TLBLE_MOST_EVENT_PAYLOAD_SIZE)

/* Defines pour compatibilité -Wundef */
#define CFG_DEBUG_TRACE_FULL  0
#define CFG_DEBUG_TRACE_LIGHT 0
