/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   archive_scene_create.c                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Nullmess <noreply@github.com>               +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 13:00:00 by Nullmess          #+#    #+#             */
/*   Updated: 2026/10/14 13:00:00 by Nullmess         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../archive_i.h"
#include "../helpers/archive_browser.h"
#include <storage/storage.h>
#include <dialogs/dialogs.h>
#include <string.h>

#define CREATE_EVENT 0UL

static void	archive_create_callback(void *context)
{
	ArchiveApp	*app;

	app = context;
	view_dispatcher_send_custom_event(app->view_dispatcher, CREATE_EVENT);
}

void	archive_scene_create_on_enter(void *context)
{
	ArchiveApp	*app;

	app = context;
	app->text_store[0] = '\0';
	text_input_set_header_text(app->text_input, "Create folder:");
	text_input_set_result_callback(app->text_input,
		archive_create_callback, app, app->text_store, 32, false);
	view_dispatcher_switch_to_view(app->view_dispatcher,
		ArchiveViewTextInput);
}

static bool	archive_create_folder(ArchiveApp *app)
{
	Storage		*storage;
	FuriString	*dest;
	bool		valid;

	valid = app->text_store[0] && strcmp(app->text_store, ".")
		&& strcmp(app->text_store, "..")
		&& !strchr(app->text_store, '/');
	if (!valid)
		return (false);
	storage = furi_record_open(RECORD_STORAGE);
	dest = furi_string_alloc_printf("%s/%s",
		furi_string_get_cstr(app->browser->path), app->text_store);
	valid = !storage_common_exists(storage, furi_string_get_cstr(dest));
	valid = valid && storage_common_mkdir(storage,
			furi_string_get_cstr(dest)) == FSE_OK;
	furi_string_free(dest);
	furi_record_close(RECORD_STORAGE);
	return (valid);
}

bool	archive_scene_create_on_event(void *context, SceneManagerEvent event)
{
	ArchiveApp	*app;
	bool		result;

	app = context;
	if (event.type != SceneManagerEventTypeCustom
		|| event.event != CREATE_EVENT)
		return (false);
	result = archive_create_folder(app);
	archive_notify(result);
	scene_manager_previous_scene(app->scene_manager);
	if (result)
		archive_refresh_dir(app->browser);
	return (true);
}

void	archive_scene_create_on_exit(void *context)
{
	ArchiveApp	*app;

	app = context;
	text_input_reset(app->text_input);
}
