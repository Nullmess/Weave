#pragma once

#include <furi.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint64_t capacity; /* Capacité totale en bytes */
    uint32_t block_size;
    uint32_t logical_block_count; /* Capacité logique en blocs */
    uint32_t logical_block_size; /* Taille bloc logique en bytes */

    uint8_t manufacturer_id;
    char oem_id[3]; /* OEM ID : 2 caractères puis NUL */
    char product_name[6]; /* Nom produit : 5 caractères puis NUL */
    uint8_t product_revision_major;
    uint8_t product_revision_minor;
    uint32_t product_serial_number;
    uint8_t manufacturing_month;
    uint16_t manufacturing_year;
} FuriHalSdInfo;

void furi_hal_sd_presence_init(void);

bool furi_hal_sd_is_present(void);

uint8_t furi_hal_sd_max_mount_retry_count(void);

/* Power_reset réinitialise l’alimentation SD */
FuriStatus furi_hal_sd_init(bool power_reset);

FuriStatus furi_hal_sd_read_blocks(uint32_t* buff, uint32_t sector, uint32_t count);

FuriStatus furi_hal_sd_write_blocks(const uint32_t* buff, uint32_t sector, uint32_t count);

FuriStatus furi_hal_sd_info(FuriHalSdInfo* info);

FuriStatus furi_hal_sd_get_card_state(void);

#ifdef __cplusplus
}
#endif
