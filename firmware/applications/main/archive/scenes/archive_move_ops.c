/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   archive_move_ops.c                                 :+:      :+:    :+:   */
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
#include <string.h>

void archive_move_result(ArchiveApp *app, bool result)
{
    archive_notify(result);
	widget_reset(app->widget);
	widget_add_string_element(app->widget, 64, 28, AlignCenter,
		AlignCenter, FontPrimary, result ? "Moved" : "Move failed");
	view_dispatcher_switch_to_view(app->view_dispatcher, ArchiveViewWidget);
}

bool archive_move_item(ArchiveApp *app, const char *root)
{
    ArchiveFile_t *item;
    Storage *storage;
    FuriString *name;
    FuriString *dest;
    FileInfo info;
    uint64_t total;
    uint64_t free;
    uint64_t size;
    bool result;

    item = archive_get_current_file(app->browser);
    if(!item)
        return false;
    storage = furi_record_open(RECORD_STORAGE);
    name = furi_string_alloc();
    dest = furi_string_alloc();
    path_extract_filename(item->path, name, false);
    furi_string_printf(dest, "%s/%s", root, furi_string_get_cstr(name));
    result = storage_common_stat(storage,
        furi_string_get_cstr(item->path), &info) == FSE_OK;
    result = result && storage_common_fs_info(storage,
        root, &total, &free) == FSE_OK;
    size = result ? info.size : 0;
    if(result && file_info_is_dir(&info))
        result = archive_folder_total(storage,
            furi_string_get_cstr(item->path), &size);
    result = result && size <= free && !storage_common_exists(storage,
        furi_string_get_cstr(dest));
    if(result)
        result = storage_common_copy(storage,
            furi_string_get_cstr(item->path), furi_string_get_cstr(dest))
            == FSE_OK;
    if(result)
        result = storage_simply_remove_recursive(storage,
            furi_string_get_cstr(item->path));
    furi_string_free(dest);
    furi_string_free(name);
    furi_record_close(RECORD_STORAGE);
    return result;
}
