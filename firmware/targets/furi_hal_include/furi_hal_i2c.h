
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <furi_hal_i2c_config.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* START suivi de l’adresse */
    FuriHalI2cBeginStart,
    /* RESTART : transaction précédente terminée par EndAwaitRestart */
    FuriHalI2cBeginRestart,
    /* Reprise après EndPause ; même direction RX/TX requise */
    FuriHalI2cBeginResume,
} FuriHalI2cBegin;

typedef enum {
    /* Fin de transaction par STOP */
    FuriHalI2cEndStop,
    /* Clock stretching suivi obligatoirement de BeginRestart */
    FuriHalI2cEndAwaitRestart,
    /* Clock stretching suivi obligatoirement de BeginResume */
    FuriHalI2cEndPause,
} FuriHalI2cEnd;

void furi_hal_i2c_init_early(void);

void furi_hal_i2c_deinit_early(void);

void furi_hal_i2c_init(void);

void furi_hal_i2c_acquire(const FuriHalI2cBusHandle* handle);

/* Handle précédemment acquis via furi_hal_i2c_acquire */
void furi_hal_i2c_release(const FuriHalI2cBusHandle* handle);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_tx(
    const FuriHalI2cBusHandle* handle,
    uint8_t address,
    const uint8_t* data,
    size_t size,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_tx_ext(
    const FuriHalI2cBusHandle* handle,
    uint16_t address,
    bool ten_bit,
    const uint8_t* data,
    size_t size,
    FuriHalI2cBegin begin,
    FuriHalI2cEnd end,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_rx(
    const FuriHalI2cBusHandle* handle,
    uint8_t address,
    uint8_t* data,
    size_t size,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_rx_ext(
    const FuriHalI2cBusHandle* handle,
    uint16_t address,
    bool ten_bit,
    uint8_t* data,
    size_t size,
    FuriHalI2cBegin begin,
    FuriHalI2cEnd end,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_trx(
    const FuriHalI2cBusHandle* handle,
    uint8_t address,
    const uint8_t* tx_data,
    size_t tx_size,
    uint8_t* rx_data,
    size_t rx_size,
    uint32_t timeout);

/* Timeout en ms ; true si device présent et prêt */
bool furi_hal_i2c_is_device_ready(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_read_reg_8(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint8_t reg_addr,
    uint8_t* data,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_read_reg_16(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint8_t reg_addr,
    uint16_t* data,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_read_mem(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint8_t mem_addr,
    uint8_t* data,
    size_t len,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_write_reg_8(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint8_t reg_addr,
    uint8_t data,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_write_reg_16(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint8_t reg_addr,
    uint16_t data,
    uint32_t timeout);

/* Timeout en ms ; retour true si transfert réussi */
bool furi_hal_i2c_write_mem(
    const FuriHalI2cBusHandle* handle,
    uint8_t i2c_addr,
    uint8_t mem_addr,
    const uint8_t* data,
    size_t len,
    uint32_t timeout);

#ifdef __cplusplus
}
#endif
