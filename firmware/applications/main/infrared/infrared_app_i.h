

#pragma once

#include <furi_hal_infrared.h>

#include <gui/gui.h>
#include <gui/view.h>
#include <assets_icons.h>
#include <gui/view_stack.h>
#include <gui/view_dispatcher.h>
#include <gui/scene_manager.h>

#include <gui/modules/popup.h>
#include <gui/modules/loading.h>
#include <gui/modules/submenu.h>
#include <gui/modules/dialog_ex.h>
#include <gui/modules/text_input.h>
#include <gui/modules/button_menu.h>
#include <gui/modules/button_panel.h>
#include <gui/modules/variable_item_list.h>

#include <rpc/rpc_app.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>

#include <notification/notification_messages.h>
#include <infrared/worker/infrared_worker.h>

#include "infrared_app.h"
#include "infrared_remote.h"
#include <lib/infrared/signal/infrared_brute_force.h>
#include "infrared_custom_event.h"

#include "scenes/infrared_scene.h"
#include "views/infrared_progress_view.h"
#include "views/infrared_debug_view.h"
#include "views/infrared_move_view.h"

#define INFRARED_FILE_NAME_SIZE  100
#define INFRARED_TEXT_STORE_NUM  2
#define INFRARED_TEXT_STORE_SIZE 128

#define INFRARED_MAX_BUTTON_NAME_LENGTH 23
#define INFRARED_MAX_REMOTE_NAME_LENGTH 23

#define INFRARED_APP_FOLDER    EXT_PATH("infrared")
#define INFRARED_APP_EXTENSION ".ir"

#define INFRARED_DEFAULT_REMOTE_NAME "Remote"
#define INFRARED_LOG_TAG             "InfraredApp"

typedef enum {
    InfraredButtonIndexNone = -1,
} InfraredButtonIndex;

typedef enum {
    InfraredEditTargetNone,
    InfraredEditTargetRemote,
    InfraredEditTargetButton,
} InfraredEditTarget;

typedef enum {
    InfraredEditModeNone,
    InfraredEditModeRename,
    InfraredEditModeDelete,
} InfraredEditMode;

typedef struct {
    bool is_learning_new_remote; /* Apprentissage nouveau remote ou ajout à l’existant */
    bool is_debug_enabled;
    bool is_transmitting;
    bool is_otg_enabled; /* OTG externe 5V activé */
    bool decode_only; /* Ignore unknown RAW samples while learning */
    InfraredEditTarget edit_target : 8;
    InfraredEditMode edit_mode     : 8;
    int32_t current_button_index; /* Index destination déplacement */
    int32_t prev_button_index; /* Index source déplacement */
    uint32_t last_transmit_time; /* Dernière transmission signal */
    FuriHalInfraredTxPin tx_pin;
} InfraredAppState;

struct InfraredApp {
    SceneManager* scene_manager;
    ViewDispatcher* view_dispatcher;

    Gui* gui;
    Storage* storage;
    DialogsApp* dialogs;
    NotificationApp* notifications;
    InfraredWorker* worker;
    InfraredRemote* remote;
    InfraredSignal* current_signal;
    InfraredBruteForce* brute_force;

    Submenu* submenu;
    TextInput* text_input;
    DialogEx* dialog_ex;
    ButtonMenu* button_menu;
    Popup* popup;
    VariableItemList* var_item_list;

    ViewStack* view_stack;
    InfraredDebugView* debug_view;
    InfraredMoveView* move_view;

    ButtonPanel* button_panel;
    Loading* loading;
    InfraredProgressView* progress;

    FuriThread* task_thread;
    FuriString* file_path; /* Chemin complet fichier chargé */
    FuriString* button_name; /* Nom bouton demandé en mode RPC */

    char text_store[INFRARED_TEXT_STORE_NUM][INFRARED_TEXT_STORE_SIZE + 1];
    InfraredAppState app_state;

    void* rpc_ctx;
};

typedef enum {
    InfraredViewSubmenu,
    InfraredViewTextInput,
    InfraredViewDialogEx,
    InfraredViewButtonMenu,
    InfraredViewPopup,
    InfraredViewVariableList,
    InfraredViewStack,
    InfraredViewDebugView,
    InfraredViewMove,
    InfraredViewLoading,
} InfraredView;

typedef enum {
    InfraredNotificationMessageSuccess,
    InfraredNotificationMessageGreenOn,
    InfraredNotificationMessageGreenOff,
    InfraredNotificationMessageYellowOn,
    InfraredNotificationMessageYellowOff,
    InfraredNotificationMessageBlinkStartRead,
    InfraredNotificationMessageBlinkStartSend,
    InfraredNotificationMessageBlinkStop,
    InfraredNotificationMessageCount,
} InfraredNotificationMessage;

/* Nom fichier généré selon noms/nombre fichiers infrared existants */

InfraredErrorCode infrared_add_remote_with_button(
    const InfraredApp* infrared,
    const char* name,
    const InfraredSignal* signal);

InfraredErrorCode
    infrared_rename_current_remote(const InfraredApp* infrared, const char* new_name);

/* Répétition indéfinie jusqu’à arrêt explicite */

void infrared_tx_start(InfraredApp* infrared);

/* Répétition indéfinie jusqu’à arrêt explicite */

InfraredErrorCode infrared_tx_start_button_index(InfraredApp* infrared, size_t button_index);

void infrared_tx_stop(InfraredApp* infrared);

void infrared_tx_send_once(InfraredApp* infrared);

InfraredErrorCode infrared_tx_send_once_button_index(InfraredApp* infrared, size_t button_index);

/* Thread séparé ; vue busy affichée, inputs ignorés */

void infrared_blocking_task_start(InfraredApp* infrared, FuriThreadCallback callback);

/* Vue busy conservée après attente ; appelant doit changer vue */

InfraredErrorCode infrared_blocking_task_finalize(InfraredApp* infrared);

/* Bank texte : 0 ou 1 */

void infrared_text_store_set(InfraredApp* infrared, uint32_t bank, const char* fmt, ...)
    _ATTRIBUTE((__format__(__printf__, 3, 4)));

/* Bank texte : 0 ou 1 */

void infrared_text_store_clear(InfraredApp* infrared, uint32_t bank);

void infrared_play_notification_message(
    const InfraredApp* infrared,
    InfraredNotificationMessage message);

void infrared_show_error_message(const InfraredApp* infrared, const char* fmt, ...)
    _ATTRIBUTE((__format__(__printf__, 2, 3)));

/* InfraredTxPinInternal utilise les LEDs intégrées */

void infrared_set_tx_pin(InfraredApp* infrared, FuriHalInfraredTxPin tx_pin);

/* GPIO pin 1 : OTG 5V */

void infrared_enable_otg(InfraredApp* infrared, bool enable);

void infrared_save_settings(InfraredApp* infrared);

/* Callback après réception signal complet */

void infrared_signal_received_callback(void* context, InfraredWorkerSignal* received_signal);

/* Callback après validation input */

void infrared_text_input_callback(void* context);

/* Callback après fermeture utilisateur ou timeout */

void infrared_popup_closed_callback(void* context);
