/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   archive_scene_move.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: Nullmess <noreply@github.com>               +#+  +:+       +#+       */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/10/14 12:00:00 by Nullmess          #+#    #+#             */
/*   Updated: 2026/10/14 12:00:00 by Nullmess         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../archive_i.h"
#include "../helpers/archive_browser.h"
#include <toolbox/path.h>
#include <storage/storage.h>
#include <dialogs/dialogs.h>

void archive_move_result(ArchiveApp *app, bool result);
bool archive_move_item(ArchiveApp *app, const char *root);

static void archive_move_select(void *context, uint32_t index)
{
	ArchiveApp		*app;
	DialogsApp		*dialogs;
	DialogMessage	*message;
	bool			confirmed;

	app = context;
	if (index != 0)
		return ;
	dialogs = furi_record_open(RECORD_DIALOGS);
	message = dialog_message_alloc();
	dialog_message_set_header(message, "Confirm move", 64, 4,
		AlignCenter, AlignTop);
	dialog_message_set_text(message, "Move to SD Card?",
		64, 27, AlignCenter, AlignCenter);
	dialog_message_set_buttons(message, "Cancel", NULL, "Move");
	confirmed = dialog_message_show(dialogs, message)
		== DialogMessageButtonRight;
	dialog_message_free(message);
	furi_record_close(RECORD_DIALOGS);
	if (confirmed)
		archive_move_result(app, archive_move_item(app,
			STORAGE_EXT_PATH_PREFIX));
}

void archive_scene_move_on_enter(void *context)
{
	ArchiveApp	*app;

	app = context;
	submenu_reset(app->roots);
	submenu_set_header(app->roots, "Move to root");
	submenu_add_item(app->roots, "SD Card", 0, archive_move_select, app);
	view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewRoots);
}

bool archive_scene_move_on_event(void *context, SceneManagerEvent event)
{
	UNUSED(context);
	UNUSED(event);
	return (false);
}

void archive_scene_move_on_exit(void *context)
{
	ArchiveApp	*app;

	app = context;
	submenu_reset(app->roots);
	widget_reset(app->widget);
	archive_refresh_dir(app->browser);
}
