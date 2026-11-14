

#pragma once

#include <core/string.h>

#include "../../nfc_app.h"
#include "../../nfc_app_i.h"

typedef void (*NfcProtocolSupportOnEnter)(NfcApp* instance);

/* true si event traité */

typedef bool (*NfcProtocolSupportOnEvent)(NfcApp* instance, SceneManagerEvent event);

/* on_exit omis car redondant */

typedef struct {
    NfcProtocolSupportOnEnter on_enter;
    NfcProtocolSupportOnEvent on_event;
} NfcProtocolSupportSceneBase;

/**
 * @brief Abstract protocol support interface.
 */
typedef struct {
    const uint32_t features;

    NfcProtocolSupportSceneBase scene_info;

    /* Infos étendues peuvent ouvrir scènes protocolaires spécifiques */

    NfcProtocolSupportSceneBase scene_more_info;

    /* Scène read crée poller et gère ses events */

    NfcProtocolSupportSceneBase scene_read;

    NfcProtocolSupportSceneBase scene_read_menu;

    NfcProtocolSupportSceneBase scene_read_success;

    NfcProtocolSupportSceneBase scene_saved_menu;

    NfcProtocolSupportSceneBase scene_save_name;

    /* Scène emulate crée listener et gère ses events */

    NfcProtocolSupportSceneBase scene_emulate;

    /* Scène write crée poller, gère events et captions */

    NfcProtocolSupportSceneBase scene_write;
} NfcProtocolSupportBase;
