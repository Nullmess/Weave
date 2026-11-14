/* Support protocole : protocol_name.h/c et protocol_name_render.h/c */
/* Features combinables par OR ; scènes implémentées selon features */
/* Scène absente : placeholder nfc_protocol_support_gui_common obligatoire */
/* Inclure header dans nfc_protocol_support_defs.c */
/* Enregistrer dans nfc_protocol_support[] à index protocole */
/* Scènes protocolaires séparées admises si abstraction insuffisante */

#pragma once

#include <gui/scene_manager.h>
#include <lib/nfc/protocols/nfc_protocol.h>

#include "nfc_protocol_support_common.h"

void nfc_protocol_support_on_enter(NfcProtocolSupportScene scene, void* context);

/* true si event consommé */

bool nfc_protocol_support_on_event(
    NfcProtocolSupportScene scene,
    void* context,
    SceneManagerEvent event);

void nfc_protocol_support_on_exit(NfcProtocolSupportScene scene, void* context);

bool nfc_protocol_support_has_feature(NfcProtocol protocol, NfcProtocolFeature feature);
