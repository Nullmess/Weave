/**
 * @file nfc_common.h
 * @brief Various common NFC-related macros.
 */
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/* Version ATQA obsolète */
#define NFC_LSB_ATQA_FORMAT_VERSION          (2)
/* Version conservée pour rétrocompatibilité */
#define NFC_MINIMUM_SUPPORTED_FORMAT_VERSION NFC_LSB_ATQA_FORMAT_VERSION
/* Version introduisant chargement unifié */
#define NFC_UNIFIED_FORMAT_VERSION           (4)
/* Current NFC file format version. */
#define NFC_CURRENT_FORMAT_VERSION           NFC_UNIFIED_FORMAT_VERSION

#ifdef __cplusplus
}
#endif
