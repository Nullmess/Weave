/**
 * @file nfc_protocol_support_common.h
 * @brief Common application-level protocol support definitions.
 */
#pragma once

typedef enum {
    NfcProtocolFeatureNone = 0,
    NfcProtocolFeatureEmulateUid = 1UL << 0,
    NfcProtocolFeatureEmulateFull = 1UL << 1,
    NfcProtocolFeatureEditUid = 1UL << 2,
    NfcProtocolFeatureMoreInfo = 1UL << 3,
    NfcProtocolFeatureWrite = 1UL << 4,
} NfcProtocolFeature;

typedef enum {
    NfcProtocolSupportSceneInfo, /**< Display general card information. */
    NfcProtocolSupportSceneMoreInfo, /**< Display more card information. */
    NfcProtocolSupportSceneRead,
    NfcProtocolSupportSceneReadMenu,
    NfcProtocolSupportSceneReadSuccess,
    NfcProtocolSupportSceneSavedMenu,
    NfcProtocolSupportSceneSaveName,
    NfcProtocolSupportSceneEmulate,
    NfcProtocolSupportSceneWrite,
    NfcProtocolSupportSceneRpc, /* Affiché en mode RPC */

    NfcProtocolSupportSceneCount, /* Nombre total de scenes ; usage interne */
} NfcProtocolSupportScene;
