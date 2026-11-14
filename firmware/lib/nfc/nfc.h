/* Transport NFC : config/timings ; création automatique via protocol stack */
#pragma once

#include <toolbox/bit_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Nfc Nfc;

/* Événements disponibles variables selon technologie/hardware */
typedef enum {
    NfcEventTypeUserAbort,
    NfcEventTypeFieldOn,
    NfcEventTypeFieldOff,
    NfcEventTypeTxStart,
    NfcEventTypeTxEnd,
    NfcEventTypeRxStart,
    NfcEventTypeRxEnd,

    NfcEventTypeListenerActivated,
    NfcEventTypePollerReady,
} NfcEventType;

typedef struct {
    BitBuffer* buffer; /* Pointeur buffer des données reçues */
} NfcEventData;

/* Structure événement transmise au callback */
typedef struct {
    NfcEventType type;
    NfcEventData data;
} NfcEvent;

/* Retour callback : commande déterminant l’action suivante */
typedef enum {
    NfcCommandContinue,
    NfcCommandReset,
    NfcCommandStop,
    NfcCommandSleep,
} NfcCommand;

/* Event par valeur ; retour commande à exécuter */
typedef NfcCommand (*NfcEventCallback)(NfcEvent event, void* context);

/* Mode listener non supporté par toutes les technologies */
typedef enum {
    NfcModePoller,
    NfcModeListener,

    NfcModeNum, /* Nombre modes, usage interne */
} NfcMode;

typedef enum {
    NfcTechIso14443a,
    NfcTechIso14443b,
    NfcTechIso15693,
    NfcTechFelica,

    NfcTechNum, /* Nombre technologies, usage interne */
} NfcTech;

typedef enum {
    NfcErrorNone,
    NfcErrorInternal, /* Erreur inconnue du niveau inférieur */
    NfcErrorTimeout, /* Timeout opération, notamment absence réponse carte */
    NfcErrorIncompleteFrame, /* Trame de données incomplète */
    NfcErrorDataFormat, /* Format données incorrect ou inconnu */
} NfcError;

/* Allocation verrouille exclusivement NFC HAL jusqu’à libération */
Nfc* nfc_alloc(void);

/* Libération release le verrou NFC HAL */
void nfc_free(Nfc* instance);

/* Mode listener non supporté par toutes les technologies */
void nfc_config(Nfc* instance, NfcMode mode, NfcTech tech);

/* Délai poller frame en cycles carrier */
void nfc_set_fdt_poll_fc(Nfc* instance, uint32_t fdt_poll_fc);

/* Délai listener frame en cycles carrier */
void nfc_set_fdt_listen_fc(Nfc* instance, uint32_t fdt_listen_fc);

/* Mask RX time en cycles carrier */
void nfc_set_mask_receive_time_fc(Nfc* instance, uint32_t mask_rx_time_fc);

/* Délai minimal entre poll frames consécutives en µs */
void nfc_set_fdt_poll_poll_us(Nfc* instance, uint32_t fdt_poll_poll_us);

/* Guard time en µs */
void nfc_set_guard_time_us(Nfc* instance, uint32_t guard_time_us);

/* Nfc_config du mode/technologie requis avant start */
void nfc_start(Nfc* instance, NfcEventCallback callback, void* context);

/* Arrêt autorisé uniquement si instance active */
void nfc_stop(Nfc* instance);

/* Usage uniquement dans le callback ; fwt en cycles carrier */
/* Trames bit/byte sans start/stop spécifiques, ajoutés par HAL */
NfcError
    nfc_poller_trx(Nfc* instance, const BitBuffer* tx_buffer, BitBuffer* rx_buffer, uint32_t fwt);

/* Usage uniquement dans callback ; start/stop spécifiques gérés par HAL */
NfcError nfc_listener_tx(Nfc* instance, const BitBuffer* tx_buffer);

/* APIs spécifiques : protocoles partiellement implémentés par hardware */

typedef enum {
    NfcIso14443aShortFrameSensReq,
    NfcIso14443aShortFrameAllReqa,
} NfcIso14443aShortFrame;

/* Timeout fwt en cycles carrier */
NfcError nfc_iso14443a_poller_trx_short_frame(
    Nfc* instance,
    NfcIso14443aShortFrame frame,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Timeout fwt en cycles carrier */
NfcError nfc_iso14443a_poller_trx_sdd_frame(
    Nfc* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Parité fournie par le caller ; fwt en cycles carrier */
NfcError nfc_iso14443a_poller_trx_custom_parity(
    Nfc* instance,
    const BitBuffer* tx_buffer,
    BitBuffer* rx_buffer,
    uint32_t fwt);

/* Parité fournie par le caller */
NfcError nfc_iso14443a_listener_tx_custom_parity(Nfc* instance, const BitBuffer* tx_buffer);

/* UID en bytes ; longueur compatible protocole requise */
NfcError nfc_iso14443a_listener_set_col_res_data(
    Nfc* instance,
    uint8_t* uid,
    uint8_t uid_len,
    uint8_t* atqa,
    uint8_t sak);

/* IDm/PMm en bytes ; sys_code issu de SYS_C */
NfcError nfc_felica_listener_set_sensf_res_data(
    Nfc* instance,
    const uint8_t* idm,
    const uint8_t idm_len,
    const uint8_t* pmm,
    const uint8_t pmm_len,
    const uint16_t sys_code);

NfcError nfc_iso15693_listener_tx_sof(Nfc* instance);

/* Collision FeliCa : bloque TX jusqu’au slot [0,15], lancer immédiatement */
void nfc_felica_listener_timer_anticol_start(Nfc* instance, uint8_t target_time_slot);

void nfc_felica_listener_timer_anticol_stop(Nfc* instance);

#ifdef __cplusplus
}
#endif
