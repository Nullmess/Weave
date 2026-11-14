/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   weave_cli.c                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Nullmess <noreply@github.com>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/13 16:06:00 by Nullmess          #+#    #+#             */
/*   Updated: 2026/10/13 16:06:00 by Nullmess         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "weave_i.h"
#include "weave_ringtone.h"
#include "weave_factory_defaults.h"

#include <cli/cli.h>
#include <stdio.h>
#include <furi_hal.h>
#include <storage/storage.h>
#include <stdlib.h>
#include <string.h>

#define TAG "Weave"

/* API interne au firmware, non exposée aux FAP. */
extern void furi_hal_version_set_custom_name(const char* name);

static const char	*weave_transport_name(WeaveTransport transport)
{
	if (transport == WeaveTransportUsb)
		return ("USB");
	if (transport == WeaveTransportBle)
		return ("BLE");
	return ("offline");
}

static const char	*weave_request_name(bool pending)
{
	if (pending)
		return ("pending");
	return ("idle");
}

static void	weave_cli(PipeSide *pipe, FuriString *args, void *context)
{
	Weave			*weave;
	WeaveTransport	transport;
	bool			pending;

	weave = context;
	UNUSED(pipe);
	if (furi_string_size(args) && furi_string_cmp_str(args, "status"))
	{
		puts("Usage: weave [status]");
		return ;
	}
	furi_mutex_acquire(weave->mutex, FuriWaitForever);
	transport = weave->transport;
	pending = weave->pending;
	furi_mutex_release(weave->mutex);
	printf("Weave 1.0.0\r\nGateway: %s\r\n",
		weave_transport_name(transport));
	printf("Request: %s\r\n", weave_request_name(pending));
}

static void weave_hostname_restore(void) {
    /* Custom names reside on SD and survive normal firmware flashing.
     * Without a saved name, the hardware OTP name (or FZ) remains active. */
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);
    if(storage_file_open(file, EXT_PATH(".weave/name"), FSAM_READ, FSOM_OPEN_EXISTING)) {
        char hostname[9] = {0};
        const uint16_t size = storage_file_read(file, hostname, sizeof(hostname));
        bool valid = size > 0 && size <= 8;
        for(uint16_t i = 0; valid && i < size; i++) {
            /* Reject corrupt binary names without replacing the factory name. */
            if((uint8_t)hostname[i] < 0x20 || (uint8_t)hostname[i] > 0x7E)
                valid = false;
        }
        if(valid) {
            hostname[size] = '\0';
            furi_hal_version_set_custom_name(hostname);
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

/* Folders are recreated after an SD format / factory reset. The category
   icons are provided by the Apps UI, not stored as icon files on SD. */
static void weave_categories_ensure(void) {
    static const char* const names[] = {
        "USB", "GPIO", "Tools", "Bluetooth", "NFC", "RFID",
        "Sub-GHz", "Infrared", "iButton", "Games", "Other",
    };
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(storage_sd_status(storage) == FSE_OK) {
        storage_simply_mkdir(storage, EXT_PATH("apps"));
        for(size_t i = 0; i < COUNT_OF(names); ++i) {
            FuriString* path = furi_string_alloc_printf(
                EXT_PATH("apps/%s"), names[i]);
            storage_simply_mkdir(storage, furi_string_get_cstr(path));
            furi_string_free(path);
        }
    }
    furi_record_close(RECORD_STORAGE);
}

/* Seules les FAP livrées avec le firmware peuvent être restaurées. */
static void weave_factory_restore_faps(void) {
    static const char* const defaults[] = {
        "USB/bad_usb.fap", "GPIO/gpio.fap", "iButton/ibutton.fap",
        "Infrared/infrared.fap", "RFID/lfrfid.fap", "NFC/nfc.fap",
        "Sub-GHz/subghz.fap", "USB/u2f.fap", "USB/hid_usb.fap",
        "Bluetooth/hid_ble.fap", "Games/snake_game.fap",
    };
    Storage* storage = furi_record_open(RECORD_STORAGE);
    if(storage_sd_status(storage) != FSE_OK ||
       !storage_file_exists(storage, WEAVE_FACTORY_RESTORE_MARKER)) {
        furi_record_close(RECORD_STORAGE);
        return;
    }

    bool complete = true;
    FuriString* source = furi_string_alloc();
    FuriString* dest = furi_string_alloc();
    FuriString* tmp = furi_string_alloc();
    for(size_t i = 0; i < COUNT_OF(defaults); ++i) {
        furi_string_printf(dest, EXT_PATH("apps/%s"), defaults[i]);
        if(storage_file_exists(storage, furi_string_get_cstr(dest))) continue;
        furi_string_printf(source, EXT_PATH(".weave/default-faps/apps/%s"), defaults[i]);
        if(!storage_file_exists(storage, furi_string_get_cstr(source))) {
            FURI_LOG_W(TAG, "Default FAP backup missing: %s", defaults[i]);
            complete = false;
            continue;
        }
        furi_string_printf(tmp, "%s.weave-part", furi_string_get_cstr(dest));
        if(storage_common_copy(storage, furi_string_get_cstr(source),
                               furi_string_get_cstr(tmp)) == FSE_OK &&
           storage_common_rename(storage, furi_string_get_cstr(tmp),
                                 furi_string_get_cstr(dest)) == FSE_OK) {
            FURI_LOG_I(TAG, "Restored FAP: %s", defaults[i]);
        } else {
            storage_simply_remove(storage, furi_string_get_cstr(tmp));
            FURI_LOG_E(TAG, "Failed to restore FAP: %s", defaults[i]);
            complete = false;
        }
    }
    if(complete) storage_simply_remove(storage, WEAVE_FACTORY_RESTORE_MARKER);
    furi_string_free(tmp);
    furi_string_free(dest);
    furi_string_free(source);
    furi_record_close(RECORD_STORAGE);
}

#include "weave_ringtone.h"

extern void weave_ble_cli_init(void);
extern void weave_network_mailbox_init(Weave* weave);

int32_t	weave_on_system_start(void *context)
{
	Weave		*weave;
	CliRegistry	*cli;

	UNUSED(context);
    weave_hostname_restore();
    weave_categories_ensure();
    weave_factory_restore_faps();
	weave = malloc(sizeof(Weave));
	furi_check(weave);
	memset(weave, 0, sizeof(Weave));
	weave->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
	furi_check(weave->mutex);
	furi_record_create(RECORD_WEAVE, weave);
	cli = furi_record_open(RECORD_CLI);
	cli_registry_add_command(cli, "weave", CliCommandFlagParallelSafe,
		weave_cli, weave);
	furi_record_close(RECORD_CLI);
	/* The ringtone worker also serves the alarm and Find my Flipper RPC. */
	weave_ringtone_init();
	weave_ble_cli_init();
	weave_network_mailbox_init(weave);
	FURI_LOG_I(TAG, "Service ready");
	return (0);
}
