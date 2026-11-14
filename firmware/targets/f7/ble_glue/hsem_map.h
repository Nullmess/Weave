#pragma once

/******************************************************************************
 * Semaphores
 * THIS SHALL NO BE CHANGED AS THESE SEMAPHORES ARE USED AS WELL ON THE CM0+
 *****************************************************************************/
/* Sémaphore standby pris par chaque CPU jusqu’à restauration éléments */

#define CFG_HW_PWR_STANDBY_SEMID     10
/* SRAM2 Thread persistent data : CPU2 peut écrire à tout moment */
/* CPU1 prend CFG_HW_THREAD_NVM_SRAM_SEMID, lit puis relâche */
/* Aucune limite durée rétention sémaphore ; stockage selon SHCI_C2_Config */

#define CFG_HW_THREAD_NVM_SRAM_SEMID 9

/* SRAM2 BLE persistent data : CPU2 peut écrire à tout moment */
/* CPU1 prend CFG_HW_BLE_NVM_SRAM_SEMID, lit puis relâche */
/* Aucune limite durée rétention sémaphore ; stockage selon SHCI_C2_Config */

#define CFG_HW_BLE_NVM_SRAM_SEMID 8

/* CPU1 prend sémaphore avant write64 ou erase secteur, relâche juste après */
/* Attendre au moins1 µs avant réacquisition pour laisser CPU2 intervenir */
/* CPU2 <=v1.4 utilise PES, sémaphore inutilisé */
/* Par défaut PES ; SHCI_C2_SetFlashActivityControl sélectionne sémaphore */

#define CFG_HW_BLOCK_FLASH_REQ_BY_CPU2_SEMID 7

/* Sémaphore CPU1 empêche write/erase CPU2, qui stall les deux CPU */
/* Ne pas utiliser PES : risque stall CPU2 */

#define CFG_HW_BLOCK_FLASH_REQ_BY_CPU1_SEMID 6

/* USB : prendre sémaphore avant config CLK48, relâcher après clock off */
/* RNG : CFG_HW_RNG_SEMID suffit pour CLK48 ; voir AN5289 */

#define CFG_HW_CLK48_CONFIG_SEMID 5

/* Index sémaphore entrée Stop Mode */
#define CFG_HW_ENTRY_STOP_MODE_SEMID 4

/* Index sémaphore RCC */
#define CFG_HW_RCC_SEMID 3

/* Index sémaphore FLASH */
#define CFG_HW_FLASH_SEMID 2

/* Index sémaphore PKA */
#define CFG_HW_PKA_SEMID 1

/* Index sémaphore RNG */
#define CFG_HW_RNG_SEMID 0
