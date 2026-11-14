#include "weave_packages.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <furi_hal_rtc.h>
#include <assets_icons.h>
#include <gui/weave_ui.h>
#include <gui/elements.h>

/* A Weave companion syncs a bounded subset of the *real* public catalog.
 * A native device has no independent internet link; all installs are requested
 * through the connected companion and confirmed by RPC before reporting success. */
/* Keep all private state out of the public /ext/apps FAP directory.
 * Only the catalog cache and one compact version database are persistent.
 * Request/result/ack files are transaction-scoped and cleaned on completion. */
#define SHOP_DIR EXT_PATH(".weave")
#define SHOP_CACHE EXT_PATH(".weave/shop.catalog")
#define SHOP_REQUEST EXT_PATH(".weave/shop.request")
#define SHOP_RESULT EXT_PATH(".weave/shop.result")
#define SHOP_VERSIONS EXT_PATH(".weave/shop.versions")
#define SHOP_VERSIONS_TEMP EXT_PATH(".weave/shop.versions.tmp")
#define SHOP_VERSIONS_BACKUP EXT_PATH(".weave/shop.versions.bak")
#define SHOP_REMOTE_REQUEST EXT_PATH(".weave/shop.remote.request")
#define SHOP_REMOTE_READY EXT_PATH(".weave/shop.remote.ready")
#define SHOP_REMOTE_HOME EXT_PATH(".weave/shop.remote.home")
#define SHOP_LEGACY_VERSION_PREFIX ".weave-installed-"
#define SHOP_LINE 320
#define SHOP_WINDOW_SIZE 12
#define SHOP_VISIBLE 3
#define SHOP_ROWS 4
#define SHOP_ICON_BYTES 20
#define SHOP_DETAIL_COLUMNS 19
#define SHOP_SEARCH (WEAVE_FAP_ENTRY_LIMIT + 4)

/* Search is local: never sends partial queries to the companion. */
static bool weave_shop_matches(const char* text, const char* query) {
    if(!query[0]) return true;
    for(const char* hay = text; *hay; hay++) {
        const char* a = hay;
        const char* b = query;
        while(*a && *b) {
            char ac = (*a >= 'A' && *a <= 'Z') ? *a + ('a' - 'A') : *a;
            char bc = (*b >= 'A' && *b <= 'Z') ? *b + ('a' - 'A') : *b;
            if(ac != bc) break;
            a++;
            b++;
        }
        if(!*b) return true;
    }
    return false;
}


/* The public catalog uses categories beyond the fixed Flipper filesystem
 * folders. Normalize them here and in both companions; never create arbitrary
 * category directories that Installed FAPs cannot browse. */
static const char* weave_shop_folder(const char* category) {
    if(weave_shop_matches(category, "usb")) return "USB";
    if(weave_shop_matches(category, "nfc")) return "NFC";
    if(weave_shop_matches(category, "infrared") || !strcmp(category, "IR")) return "Infrared";
    if(weave_shop_matches(category, "sub-ghz") || weave_shop_matches(category, "subghz") ||
       weave_shop_matches(category, "radio")) return "Sub-GHz";
    if(weave_shop_matches(category, "rfid") || weave_shop_matches(category, "125khz")) return "RFID";
    if(weave_shop_matches(category, "bluetooth") || !strcmp(category, "BLE")) return "Bluetooth";
    if(weave_shop_matches(category, "gpio")) return "GPIO";
    if(weave_shop_matches(category, "ibutton") || weave_shop_matches(category, "1-wire")) return "iButton";
    if(weave_shop_matches(category, "game")) return "Games";
    if(weave_shop_matches(category, "tool") || weave_shop_matches(category, "utilit")) return "Tools";
    return "Other";
}

static bool weave_shop_valid_id(const char* id) {
    size_t n = strlen(id);
    if(n < 2 || n > 80) return false;
    for(const char* p = id; *p; ++p)
        if(!((*p >= 'A' && *p <= 'Z') || (*p >= 'a' && *p <= 'z') ||
             (*p >= '0' && *p <= '9') || *p == '_' || *p == '-')) return false;
    return true;
}

static bool weave_shop_installed_path(
    WeavePackageApp* app, const char* id, const char* category, char* out, size_t size) {
    if(!weave_shop_valid_id(id)) return false;
    const char* folders[WEAVE_FAP_CATEGORY_COUNT + 2];
    folders[0] = weave_shop_folder(category);
    for(size_t i = 0; i < WEAVE_FAP_CATEGORY_COUNT; ++i)
        folders[i + 1] = weave_fap_category_names[i];
    folders[WEAVE_FAP_CATEGORY_COUNT + 1] = "Others";
    for(size_t i = 0; i < WEAVE_FAP_CATEGORY_COUNT + 2; ++i) {
        int n = snprintf(out, size, "/ext/apps/%s/%s.fap", folders[i], id);
        if(n > 0 && (size_t)n < size && storage_common_exists(app->storage, out)) return true;
    }
    /* Pre-existing installations from the old Android installer could have
     * arbitrary noncanonical directories. Search one level for that filename. */
    File* dir = storage_file_alloc(app->storage);
    if(storage_dir_open(dir, EXT_PATH("apps"))) {
        FileInfo info;
        char name[96];
        uint32_t checked = 0;
        while(checked++ < 96 && storage_dir_read(dir, &info, name, sizeof(name))) {
            if(!file_info_is_dir(&info) || !strcmp(name, ".") || !strcmp(name, "..") ||
               strchr(name, '/') || strchr(name, '\\')) continue;
            int n = snprintf(out, size, "/ext/apps/%s/%s.fap", name, id);
            if(n > 0 && (size_t)n < size && storage_common_exists(app->storage, out)) {
                storage_dir_close(dir);
                storage_file_free(dir);
                return true;
            }
        }
        storage_dir_close(dir);
    }
    storage_file_free(dir);
    out[0] = 0;
    return false;
}

/* One shared database replaces per-FAP .weave-installed-*.txt files.
 * Database: id|catalog-version|exact-fap-path\n (not user-visible content).
 * Small bounded streaming buffers avoid allocating the database in Flipper RAM. */
static bool weave_shop_version_line_matches(const char* line, const char* id) {
    size_t n = strlen(id);
    return !strncmp(line, id, n) && line[n] == '|';
}

static bool weave_shop_version_fields_valid(const char* id, const char* version, const char* path) {
    if(!weave_shop_valid_id(id) || !version || !path || !*version ||
       strlen(version) > 32 || strlen(path) > 191 ||
       strncmp(path, "/ext/apps/", 10) ||
       strcmp(path + strlen(path) - 4, ".fap")) return false;
    for(const char* p = version; *p; p++)
        if(*p == '|' || *p == '\n' || *p == '\r') return false;
    for(const char* p = path; *p; p++)
        if(*p == '|' || *p == '\n' || *p == '\r') return false;
    return !strstr(path, "..");
}

/* Rewrite via a temporary file and a recoverable backup, never in-place. */
static bool weave_shop_versions_rewrite(
    WeavePackageApp* app, const char* id, const char* version, const char* path) {
    if(!weave_shop_valid_id(id) ||
       (version && !weave_shop_version_fields_valid(id, version, path))) return false;
    storage_simply_mkdir(app->storage, SHOP_DIR);
    File* source = storage_file_alloc(app->storage);
    File* target = storage_file_alloc(app->storage);
    storage_common_remove(app->storage, SHOP_VERSIONS_TEMP);
    bool writing = storage_file_open(target, SHOP_VERSIONS_TEMP, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    bool reading = storage_file_open(source, SHOP_VERSIONS, FSAM_READ, FSOM_OPEN_EXISTING);
    bool ok = writing;
    char line[328];
    size_t used = 0;
    bool overflow = false;
    uint32_t scanned = 0;
    uint8_t chunk[128];
    size_t count;
    while(ok && reading && (count = storage_file_read(source, chunk, sizeof(chunk))) != 0) {
        for(size_t j = 0; j < count && ok; ++j) {
            if(chunk[j] != '\n') {
                if(used < sizeof(line) - 1) line[used++] = (char)chunk[j];
                else overflow = true;
                continue;
            }
            if(++scanned > 512) { ok = false; break; }
            line[used] = 0;
            if(!overflow && used > 0 && !weave_shop_version_line_matches(line, id)) {
                if(storage_file_write(target, line, used) != used ||
                   storage_file_write(target, "\n", 1) != 1) ok = false;
            }
            used = 0;
            overflow = false;
        }
    }
    line[used] = 0;
    if(ok && reading && used && !overflow && !weave_shop_version_line_matches(line, id)) {
        if(storage_file_write(target, line, used) != used ||
           storage_file_write(target, "\n", 1) != 1) ok = false;
    }
    if(ok && version) {
        char record[328];
        int length = snprintf(record, sizeof(record), "%s|%s|%s\n", id, version, path);
        ok = length > 0 && (size_t)length < sizeof(record) &&
             storage_file_write(target, record, (size_t)length) == (size_t)length;
    }
    if(reading) storage_file_close(source);
    if(writing) storage_file_close(target);
    storage_file_free(source);
    storage_file_free(target);
    if(!ok) { storage_common_remove(app->storage, SHOP_VERSIONS_TEMP); return false; }
    /* Restore an interrupted previous commit before replacing the DB. */
    if(storage_common_exists(app->storage, SHOP_VERSIONS)) {
        storage_common_remove(app->storage, SHOP_VERSIONS_BACKUP);
        if(storage_common_rename(app->storage, SHOP_VERSIONS, SHOP_VERSIONS_BACKUP) != FSE_OK)
            return false;
    }
    if(storage_common_rename(app->storage, SHOP_VERSIONS_TEMP, SHOP_VERSIONS) != FSE_OK) {
        if(storage_common_exists(app->storage, SHOP_VERSIONS_BACKUP))
            storage_common_rename(app->storage, SHOP_VERSIONS_BACKUP, SHOP_VERSIONS);
        return false;
    }
    storage_common_remove(app->storage, SHOP_VERSIONS_BACKUP);
    return true;
}

static void weave_shop_remember_version(
    WeavePackageApp* app, const char* id, const char* version, const char* path) {
    if(!version || !*version || !path || !*path) return;
    if(!weave_shop_versions_rewrite(app, id, version, path))
        FURI_LOG_W("WeaveApps", "Unable to save version metadata");
}

static bool weave_shop_stored_version(
    WeavePackageApp* app, const char* id, const char* path, char* output, size_t size) {
    if(!weave_shop_valid_id(id)) return false;
    File* file = storage_file_alloc(app->storage);
    if(!storage_file_open(file, SHOP_VERSIONS, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        return false;
    }
    bool found = false;
    char line[328];
    size_t used = 0;
    bool overflow = false;
    uint8_t chunk[128];
    size_t count;
    while(!found && (count = storage_file_read(file, chunk, sizeof(chunk))) != 0) {
        for(size_t j = 0; j < count; ++j) {
            if(chunk[j] != '\n') {
                if(used < sizeof(line) - 1) line[used++] = (char)chunk[j];
                else overflow = true;
                continue;
            }
            line[used] = 0;
            if(!overflow && weave_shop_version_line_matches(line, id)) {
                char* version = strchr(line, '|') + 1;
                char* saved = strchr(version, '|');
                if(saved) {
                    *saved++ = 0;
                    if(!strcmp(saved, path) && strlen(version) < size && strlen(version) <= 32) {
                        strlcpy(output, version, size);
                        found = true;
                        break;
                    }
                }
            }
            used = 0;
            overflow = false;
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    return found;
}

void weave_packages_shop_forget_path(WeavePackageApp* app, const char* path) {
    const char* name = strrchr(path, '/');
    if(!name || !strstr(name, ".fap")) return;
    char id[81];
    size_t n = strlen(name + 1);
    if(n < 5 || n - 4 >= sizeof(id) || strcmp(name + 1 + n - 4, ".fap")) return;
    memcpy(id, name + 1, n - 4);
    id[n - 4] = 0;
    char saved[33];
    if(weave_shop_stored_version(app, id, path, saved, sizeof(saved)))
        weave_shop_versions_rewrite(app, id, NULL, NULL);
}

void weave_packages_shop_moved_path(
    WeavePackageApp* app, const char* old_path, const char* new_path) {
    if(strncmp(old_path, "/ext/apps/", 10) || strncmp(new_path, "/ext/apps/", 10)) return;
    const char* filename = strrchr(old_path, '/');
    if(!filename) return;
    size_t n = strlen(filename + 1);
    if(n < 5 || n > 84 || strcmp(filename + 1 + n - 4, ".fap")) return;
    char id[81];
    memcpy(id, filename + 1, n - 4);
    id[n - 4] = 0;
    char version[33];
    if(weave_shop_stored_version(app, id, old_path, version, sizeof(version)))
        weave_shop_remember_version(app, id, version, new_path);
}

/* One-time migration: preserve valid installed-version metadata and the cached
 * Shop, remove only Weave's own obsolete top-level temporary files. Never touch
 * user applications, FAP partials/backups, or unrelated .txt files. */
void weave_packages_catalog_migrate_storage(WeavePackageApp* app) {
    storage_simply_mkdir(app->storage, SHOP_DIR);
    if(storage_common_exists(app->storage, SHOP_VERSIONS_BACKUP)) {
        if(!storage_common_exists(app->storage, SHOP_VERSIONS))
            storage_common_rename(app->storage, SHOP_VERSIONS_BACKUP, SHOP_VERSIONS);
        else storage_common_remove(app->storage, SHOP_VERSIONS_BACKUP);
    }
    const char* old_catalog = EXT_PATH("apps/.weave-catalog.txt");
    if(storage_common_exists(app->storage, old_catalog)) {
        if(!storage_common_exists(app->storage, SHOP_CACHE))
            storage_common_rename(app->storage, old_catalog, SHOP_CACHE);
        else storage_common_remove(app->storage, old_catalog);
    }
    File* dir = storage_file_alloc(app->storage);
    if(storage_dir_open(dir, EXT_PATH("apps"))) {
        FileInfo info;
        char name[128];
        uint32_t checked = 0;
        while(checked++ < 512 && storage_dir_read(dir, &info, name, sizeof(name))) {
            if(file_info_is_dir(&info) || strncmp(name, SHOP_LEGACY_VERSION_PREFIX,
                                                strlen(SHOP_LEGACY_VERSION_PREFIX))) continue;
            size_t len = strlen(name), prefix = strlen(SHOP_LEGACY_VERSION_PREFIX);
            if(len <= prefix + 4 || strcmp(name + len - 4, ".txt")) continue;
            char id[81];
            if(len - prefix - 4 >= sizeof(id)) continue;
            memcpy(id, name + prefix, len - prefix - 4);
            id[len - prefix - 4] = 0;
            if(!weave_shop_valid_id(id)) continue;
            char legacy[256];
            snprintf(legacy, sizeof(legacy), "/ext/apps/%s", name);
            File* file = storage_file_alloc(app->storage);
            char buffer[280] = {0};
            bool ok = false;
            if(storage_file_open(file, legacy, FSAM_READ, FSOM_OPEN_EXISTING)) {
                size_t n = storage_file_read(file, buffer, sizeof(buffer) - 1);
                buffer[n] = 0;
                storage_file_close(file);
                char* newline = strchr(buffer, '\n');
                if(newline) {
                    *newline++ = 0;
                    char* end = strchr(newline, '\n');
                    if(end) *end = 0;
                    if(weave_shop_version_fields_valid(id, buffer, newline) &&
                       storage_common_exists(app->storage, newline))
                        ok = weave_shop_versions_rewrite(app, id, buffer, newline);
                    else ok = true; /* obsolete metadata for missing/invalid FAP */
                }
            }
            storage_file_free(file);
            if(ok) storage_common_remove(app->storage, legacy);
        }
        storage_dir_close(dir);
    }
    storage_file_free(dir);
    const char* temporary[] = {
        EXT_PATH("apps/.weave-install-request.txt"),
        EXT_PATH("apps/.weave-install-result.txt"),
        EXT_PATH("apps/.weave-remote-install.txt"),
        EXT_PATH("apps/.weave-remote-ready.txt"),
        EXT_PATH("apps/.weave-remote-home.txt"),
    };
    for(size_t i = 0; i < COUNT_OF(temporary); ++i)
        storage_common_remove(app->storage, temporary[i]);
}

/* Numeric version parts can be compared without guessing at opaque tags.
 * 1.2 and 1.2.0 compare equal; an unknown version never triggers Update. */
static bool weave_shop_parse_version(const char* text, uint32_t parts[4], bool* prerelease) {
    if(!text || !*text) return false;
    if(*text == 'v' || *text == 'V') ++text;
    memset(parts, 0, sizeof(uint32_t) * 4);
    *prerelease = false;
    for(size_t i = 0; i < 4; ++i) {
        if(*text < '0' || *text > '9') return false;
        while(*text >= '0' && *text <= '9') {
            if(parts[i] > (UINT32_MAX - 9) / 10) return false;
            parts[i] = parts[i] * 10 + (*text++ - '0');
        }
        if(*text == '-') {
            *prerelease = true;
            return text[1] != 0;
        }
        if(!*text || *text == '+') return true;
        if(*text++ != '.' || i == 3) return false;
    }
    return false;
}

static bool weave_shop_version_newer(const char* remote, const char* installed) {
    uint32_t a[4], b[4];
    bool remote_preview, installed_preview;
    if(!weave_shop_parse_version(remote, a, &remote_preview) ||
       !weave_shop_parse_version(installed, b, &installed_preview)) return false;
    for(size_t i = 0; i < 4; ++i) {
        if(a[i] != b[i]) return a[i] > b[i];
    }
    return installed_preview && !remote_preview;
}

/* Called only for a selected catalog entry, never for every list row. */
static bool weave_shop_entry_installed(WeavePackageApp* app, uint32_t index,
                                      char* path, size_t size, bool* update) {
    const char* id = furi_string_get_cstr(app->entries[index]);
    const char* category = furi_string_get_cstr(app->catalog_descriptions[index]);
    if(!weave_shop_installed_path(app, id, category, path, size)) return false;
    char version[33];
    *update = weave_shop_stored_version(app, id, path, version, sizeof(version)) &&
              weave_shop_version_newer(
                furi_string_get_cstr(app->catalog_versions[index]), version);
    return true;
}

static void weave_shop_refresh_detail_state(WeavePackageApp* app) {
    app->catalog_detail_installed = false;
    app->catalog_detail_update = false;
    app->catalog_detail_installed_path[0] = 0;
    if(app->catalog_detail_index < app->entry_count &&
       app->entries[app->catalog_detail_index] &&
       app->catalog_descriptions[app->catalog_detail_index] &&
       app->catalog_versions[app->catalog_detail_index]) {
        app->catalog_detail_installed = weave_shop_entry_installed(
            app, app->catalog_detail_index, app->catalog_detail_installed_path,
            sizeof(app->catalog_detail_installed_path), &app->catalog_detail_update);
    }
}

static bool weave_shop_line(WeavePackageApp* app, char* line) {
    char* id = strtok(line, "\t");
    char* title = strtok(NULL, "\t");
    char* category = strtok(NULL, "\t");
    char* description = strtok(NULL, "\t");
    char* icon_hex = strtok(NULL, "\t");
    char* version = strtok(NULL, "\t");
    if(!id || !title || !category || !description || !*title || !*id) return false;
    if(strlen(id) > 80) return false;
    for(const char* c = id; *c; c++) {
        if(!((*c >= 'a' && *c <= 'z') || (*c >= 'A' && *c <= 'Z') ||
             (*c >= '0' && *c <= '9') || *c == '-' || *c == '_')) return false;
    }
    uint32_t i = app->entry_count;
    if(i >= WEAVE_FAP_ENTRY_LIMIT) return false;
    app->entries[i] = furi_string_alloc_set(id);
    app->labels[i] = furi_string_alloc_set(title);
    app->catalog_descriptions[i] = furi_string_alloc_printf("%s\n%s", category, description);
    app->catalog_versions[i] = furi_string_alloc_set(version && strcmp(version, "-") ? version : "");
    app->icons[i] = NULL;
    if(icon_hex && strlen(icon_hex) == SHOP_ICON_BYTES * 2) {
        uint8_t icon_data[SHOP_ICON_BYTES];
        bool good = true;
        for(size_t k = 0; k < SHOP_ICON_BYTES; k++) {
            char hi = icon_hex[k * 2];
            char lo = icon_hex[k * 2 + 1];
            int a = hi >= '0' && hi <= '9' ? hi - '0' :
                hi >= 'a' && hi <= 'f' ? hi - 'a' + 10 :
                hi >= 'A' && hi <= 'F' ? hi - 'A' + 10 : -1;
            int b = lo >= '0' && lo <= '9' ? lo - '0' :
                lo >= 'a' && lo <= 'f' ? lo - 'a' + 10 :
                lo >= 'A' && lo <= 'F' ? lo - 'A' + 10 : -1;
            if(a < 0 || b < 0) { good = false; break; }
            icon_data[k] = (a << 4) | b;
        }
        if(good) {
            app->icons[i] = malloc(SHOP_ICON_BYTES);
            if(app->icons[i]) memcpy(app->icons[i], icon_data, SHOP_ICON_BYTES);
        }
    }
    app->entry_count++;
    return true;
}

static bool weave_shop_load(WeavePackageApp* app, bool* more) {
    furi_check(furi_mutex_acquire(app->entries_mutex, FuriWaitForever) == FuriStatusOk);
    weave_packages_clear_entries(app);
    File* file = storage_file_alloc(app->storage);
    if(!storage_file_open(file, SHOP_CACHE, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_free(file);
        furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
        return false;
    }
    char line[SHOP_LINE];
    size_t used = 0;
    uint32_t valid = 0;
    *more = false;
    uint8_t buffer[128];
    size_t count;
    bool finished = false;
    bool overflow = false;
    while(!finished && (count = storage_file_read(file, buffer, sizeof(buffer))) > 0) {
        for(size_t j = 0; j < count; j++) {
            uint8_t c = buffer[j];
            if(c != '\n') {
                if(used < sizeof(line) - 1 && ((c >= 32 && c <= 126) || c == '\t'))
                    line[used++] = (char)c;
                else overflow = true;
                continue;
            }
            line[used] = 0;
            used = 0;
            if(overflow) { overflow = false; continue; }
            if(!weave_shop_matches(line, app->catalog_query)) continue;
            if(valid++ < app->catalog_offset) continue;
            if(app->entry_count >= SHOP_WINDOW_SIZE) {
                *more = true;
                finished = true;
                break;
            }
            weave_shop_line(app, line);
        }
    }
    storage_file_close(file);
    storage_file_free(file);
    furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
    return true;
}

/* One request per installation, with a fresh token. The Flipper's UI observes
 * the result file automatically instead of asking the user to check status. */
#define SHOP_POLL_TICKS 3U  /* 600 ms; keep Storage RPC traffic low */
#define SHOP_WAIT_TIMEOUT 450U /* 90 seconds without companion response */
#define SHOP_INSTALL_TIMEOUT 9000U /* 30-minute absolute safety cap */
#define SHOP_IDLE_TIMEOUT 1500U /* 5 minutes without meaningful progress */

static bool weave_shop_install_finished(const WeavePackageApp* app) {
    return app->catalog_install_state == WeaveInstallDone ||
           app->catalog_install_state == WeaveInstallFailed;
}

static void weave_shop_write_request(WeavePackageApp* app, const char* id) {
    app->catalog_install_state = WeaveInstallWaiting;
    app->catalog_install_ticks = 0;
    app->catalog_install_last_activity_tick = 0;
    app->catalog_install_spinner = 0;
    app->catalog_install_visual_percent = 0;
    app->catalog_install_done_ticks = 0;
    app->catalog_install_path[0] = 0;
    app->catalog_install_total_bytes = 0;
    app->catalog_install_last_bytes = 0;
    app->catalog_install_initial_bytes = 0;
    app->catalog_install_new_file = false;
    app->catalog_install_transfer_tick = 0;
    app->catalog_install_eta_seconds = 0;
    app->catalog_install_percent = 0;
    app->catalog_install_percent_valid = false;
    snprintf(app->catalog_install_request, sizeof(app->catalog_install_request), "%s|%lu-%lu", id,
             (unsigned long)furi_hal_rtc_get_timestamp(), (unsigned long)furi_get_tick());
    strlcpy(app->catalog_remote_completion_token, app->catalog_install_request,
            sizeof(app->catalog_remote_completion_token));
    strlcpy(app->catalog_install_id, id, sizeof(app->catalog_install_id));
    strlcpy(app->catalog_install_version,
        furi_string_get_cstr(app->catalog_versions[app->catalog_detail_index]),
        sizeof(app->catalog_install_version));
    strlcpy(app->catalog_install_title,
        furi_string_get_cstr(app->labels[app->catalog_detail_index]),
        sizeof(app->catalog_install_title));
    /* Comme Firmware Updater : backlight maintenu durant le transfert. */
    if(!app->catalog_install_backlight_locked) {
        app->catalog_install_backlight_locked = true;
        notification_message(app->notification, &sequence_display_backlight_enforce_on);
    }
    /* Enter the progress screen before writing: errors use the same UI. */
    app->page = WeavePackagePageCatalogInstall;
    view_dispatcher_switch_to_view(app->dispatcher, 6);

    File* file = storage_file_alloc(app->storage);
    bool ok = storage_file_open(file, SHOP_REQUEST, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        const size_t len = strlen(app->catalog_install_request);
        ok = storage_file_write(file, app->catalog_install_request, len) == len;
        storage_file_close(file);
    }
    storage_file_free(file);
    if(!ok) app->catalog_install_state = WeaveInstallFailed;
    view_commit_model(app->catalog_install, true);
}

/* WN shop status v2 (still reads v1): token|phase|percent|eta|total|path.
 * Percent is optional while the server does not report a Content-Length.
 * During Storage RPC upload the link is occupied: derive transfer progress
 * from the actual SD file size rather than reporting imaginary percentages. */
static bool weave_shop_uint(const char* value, uint32_t* output) {
    if(!value || !*value) return false;
    uint32_t result = 0;
    for(const char* ptr = value; *ptr; ptr++) {
        if(*ptr < '0' || *ptr > '9' || result > (UINT32_MAX - 9) / 10) return false;
        result = result * 10 + (*ptr - '0');
    }
    *output = result;
    return true;
}

/* Android and the native Shop share this exact updater-style screen.
 * Only an explicit RPC app-start argument enables the remote path: a stale
 * SD marker cannot hijack a normal Weave Apps launch. */
bool weave_packages_catalog_remote_install_init(WeavePackageApp* app) {
    char request[420] = {0};
    File* file = storage_file_alloc(app->storage);
    bool opened = storage_file_open(
        file, SHOP_REMOTE_REQUEST, FSAM_READ, FSOM_OPEN_EXISTING);
    if(opened) {
        size_t n = storage_file_read(file, request, sizeof(request) - 1);
        request[n] = 0;
        storage_file_close(file);
    }
    storage_file_free(file);
    if(!opened) return false;

    /* token|id|title|version|total-bytes|staging-path. All fields sanitized
     * by the companion and validated again before becoming UI state. */
    char* save = NULL;
    char* token = strtok_r(request, "|\r\n", &save);
    char* id = strtok_r(NULL, "|\r\n", &save);
    char* title = strtok_r(NULL, "|\r\n", &save);
    char* version = strtok_r(NULL, "|\r\n", &save);
    char* total = strtok_r(NULL, "|\r\n", &save);
    char* path = strtok_r(NULL, "|\r\n", &save);
    if(!token || !id || !title || !version || !total || !path ||
       !weave_shop_valid_id(id) || strlen(token) > 100 ||
       strlen(title) > 32 || strlen(version) > 32 ||
       strncmp(token, "phone-", 6) || !token[6] ||
       strlen(path) >= sizeof(app->catalog_install_path)) return false;
    const bool downloading = !strcmp(path, "-");
    if(!downloading && (strncmp(path, "/ext/apps/", 10) || strstr(path, "..") ||
                        !strstr(path, ".fap.weave-part"))) return false;
    for(const char* c = token; *c; ++c) {
        if(!((*c >= 'a' && *c <= 'z') || (*c >= '0' && *c <= '9') ||
             *c == '-')) return false;
    }
    uint32_t bytes = 0;
    if(!weave_shop_uint(total, &bytes) || bytes > 2000000 ||
       (downloading && bytes != 0) || (!downloading && bytes == 0)) return false;

    strlcpy(app->catalog_install_request, token, sizeof(app->catalog_install_request));
    strlcpy(app->catalog_remote_completion_token, token,
            sizeof(app->catalog_remote_completion_token));
    strlcpy(app->catalog_install_id, id, sizeof(app->catalog_install_id));
    strlcpy(app->catalog_install_title, title, sizeof(app->catalog_install_title));
    strlcpy(app->catalog_install_version, version, sizeof(app->catalog_install_version));
    strlcpy(app->catalog_install_path, path, sizeof(app->catalog_install_path));
    app->catalog_install_remote = true;
    app->catalog_remote_ready_sent = false;
    app->catalog_install_total_bytes = bytes;
    app->catalog_install_new_file = true;
    app->catalog_install_state = downloading ? WeaveInstallDownloading : WeaveInstallInstalling;
    app->catalog_install_ticks = 0;
    app->catalog_install_last_activity_tick = 0;
    app->catalog_install_spinner = 0;
    app->catalog_install_visual_percent = 0;
    app->catalog_install_done_ticks = 0;
    app->catalog_install_percent = 0;
    app->catalog_install_percent_valid = !downloading;
    app->catalog_install_backlight_locked = true;
    notification_message(app->notification, &sequence_display_backlight_enforce_on);
    app->page = WeavePackagePageCatalogInstall;
    view_dispatcher_switch_to_view(app->dispatcher, 6);
    view_commit_model(app->catalog_install, true);
    return true;
}

/* A successful App.Start RPC is not proof that a native view was opened.
 * The firmware acknowledges the exact phone token AFTER activating the
 * install view. Android waits for this marker before downloading the FAP. */
void weave_packages_catalog_remote_ready(WeavePackageApp* app) {
    if(!app->catalog_install_remote || !app->catalog_install_request[0]) return;
    File* file = storage_file_alloc(app->storage);
    bool ok = storage_file_open(
        file, SHOP_REMOTE_READY, FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        size_t len = strlen(app->catalog_install_request);
        ok = storage_file_write(file, app->catalog_install_request, len) == len;
        storage_file_close(file);
    }
    storage_file_free(file);
    if(ok) app->catalog_remote_ready_sent = true;
    else FURI_LOG_E("WeaveApps", "Unable to acknowledge native install screen");
}

/* One-shot completion token, written only AFTER the native GUI event loop
 * exits and its views are freed. Both successful and failed installations
 * must unblock the companion; a failed transfer must not strand its queue. */
void weave_packages_catalog_remote_complete(WeavePackageApp* app) {
    /* Once the native GUI has exited, no further status polling needs these.
     * The phone alone removes its HOME ack after consuming it. */
    if(weave_shop_install_finished(app)) {
        storage_common_remove(app->storage, SHOP_RESULT);
        storage_common_remove(app->storage, SHOP_REQUEST);
        storage_common_remove(app->storage, SHOP_REMOTE_REQUEST);
    }
    if((app->catalog_install_state != WeaveInstallDone &&
        app->catalog_install_state != WeaveInstallFailed) ||
       !app->catalog_remote_completion_token[0]) return;
    File* file = storage_file_alloc(app->storage);
    bool ok = storage_file_open(file, SHOP_REMOTE_HOME,
                                FSAM_WRITE, FSOM_CREATE_ALWAYS);
    if(ok) {
        const size_t len = strlen(app->catalog_remote_completion_token);
        ok = storage_file_write(file, app->catalog_remote_completion_token, len) == len;
        storage_file_close(file);
    }
    storage_file_free(file);
    if(!ok) FURI_LOG_E("WeaveApps", "Unable to acknowledge native install close");
}

/* If Weave Apps was already opened locally, Loader rejects another App.Start.
 * Watch for a NEW phone request and enter the very same native progress view.
 * Initial scan captures the old marker so a stale file cannot hijack a
 * normal user-initiated launch of Weave Apps. */
void weave_packages_catalog_remote_install_watch(WeavePackageApp* app, bool accept_new) {
    if(app->catalog_install_request[0]) return;
    File* file = storage_file_alloc(app->storage);
    char marker[420] = {0};
    if(storage_file_open(file, SHOP_REMOTE_REQUEST,
                         FSAM_READ, FSOM_OPEN_EXISTING)) {
        size_t n = storage_file_read(file, marker, sizeof(marker) - 1);
        marker[n] = 0;
        storage_file_close(file);
    }
    storage_file_free(file);
    char* end = strchr(marker, '|');
    if(!end || end - marker < 7 || end - marker >= (int)sizeof(app->catalog_remote_last_seen))
        return;
    *end = 0;
    if(strncmp(marker, "phone-", 6) ||
       !strcmp(marker, app->catalog_remote_last_seen)) return;
    strlcpy(app->catalog_remote_last_seen, marker, sizeof(app->catalog_remote_last_seen));
    if(accept_new) weave_packages_catalog_remote_install_init(app);
}

static void weave_shop_install_poll(WeavePackageApp* app) {
    if(!app->catalog_install_request[0]) return;
    File* file = storage_file_alloc(app->storage);
    char status[320] = {0};
    if(storage_file_open(file, SHOP_RESULT, FSAM_READ, FSOM_OPEN_EXISTING)) {
        const size_t len = storage_file_read(file, status, sizeof(status) - 1);
        status[len] = '\0';
        storage_file_close(file);
    }
    storage_file_free(file);
    const size_t token_len = strlen(app->catalog_install_request);
    if(strncmp(status, app->catalog_install_request, token_len) ||
       status[token_len] != '|') return;
    char* cursor = status + token_len + 1;
    char* phase = strtok(cursor, "|\r\n");
    if(!phase) return;
    if(strcmp(phase, "done") == 0) {
        app->catalog_install_last_activity_tick = app->catalog_install_ticks;
        char path[192] = {0};
        const char* installed_path = app->catalog_install_path;
        if(app->catalog_install_remote) {
            /* Companion supplied the precise path after its RPC inventory check.
             * Do not mistake a partial .weave-part upload for a completed FAP. */
            strtok(NULL, "|\r\n"); // percentage
            strtok(NULL, "|\r\n"); // ETA
            strtok(NULL, "|\r\n"); // total bytes
            const char* verified = strtok(NULL, "|\r\n");
            bool valid = verified && !strncmp(verified, "/ext/apps/", 10) &&
                !strstr(verified, "..") && !strchr(verified, '\\') &&
                strlen(verified) < sizeof(path);
            if(valid) {
                size_t len = strlen(verified);
                valid = len > 4 && !strcmp(verified + len - 4, ".fap") &&
                    storage_common_exists(app->storage, verified);
            }
            if(!valid) {
                app->catalog_install_state = WeaveInstallFailed;
                return;
            }
            weave_shop_remember_version(app, app->catalog_install_id,
                                        app->catalog_install_version, verified);
            app->catalog_install_state = WeaveInstallDone;
            app->catalog_install_percent = 100;
            app->catalog_install_percent_valid = true;
            app->catalog_install_eta_seconds = 0;
            return;
        }
        if(installed_path[0] && storage_common_exists(app->storage, installed_path)) {
            strlcpy(path, installed_path, sizeof(path));
        } else if(!weave_shop_installed_path(app, app->catalog_install_id, "", path, sizeof(path))) {
            app->catalog_install_state = WeaveInstallFailed;
            return;
        }
        weave_shop_remember_version(app, app->catalog_install_id,
                                    app->catalog_install_version, path);
        weave_shop_refresh_detail_state(app);
        app->catalog_install_state = WeaveInstallDone;
        app->catalog_install_percent = 100;
        app->catalog_install_percent_valid = true;
        app->catalog_install_eta_seconds = 0;
    } else if(strcmp(phase, "failed") == 0) {
        app->catalog_install_last_activity_tick = app->catalog_install_ticks;
        app->catalog_install_state = WeaveInstallFailed;
    } else if(strcmp(phase, "downloading") == 0 || strcmp(phase, "installing") == 0) {
        if(strcmp(phase, "installing") == 0) {
            if(app->catalog_install_state != WeaveInstallInstalling) {
                app->catalog_install_transfer_tick = app->catalog_install_ticks;
                app->catalog_install_visual_percent = 0;
                app->catalog_install_last_bytes = 0;
                app->catalog_install_percent_valid = false;
                app->catalog_install_initial_bytes = 0;
                app->catalog_install_new_file = false;
            }
            app->catalog_install_state = WeaveInstallInstalling;
            app->catalog_install_last_activity_tick = app->catalog_install_ticks;
        } else if(app->catalog_install_state != WeaveInstallInstalling) {
            app->catalog_install_state = WeaveInstallDownloading;
            app->catalog_install_last_activity_tick = app->catalog_install_ticks;
        } else return; /* Never regress from transfer to download. */
        uint32_t number;
        const char* pct = strtok(NULL, "|\r\n");
        const char* eta = strtok(NULL, "|\r\n");
        const char* total = strtok(NULL, "|\r\n");
        const char* path = strtok(NULL, "|\r\n");
        if(weave_shop_uint(pct, &number) && number <= 99 &&
           (!app->catalog_install_percent_valid || number >= app->catalog_install_percent)) {
            if(!app->catalog_install_percent_valid || number > app->catalog_install_percent)
                app->catalog_install_last_activity_tick = app->catalog_install_ticks;
            app->catalog_install_percent = number;
            app->catalog_install_percent_valid = true;
        }
        if(weave_shop_uint(eta, &number) && number < 36000)
            app->catalog_install_eta_seconds = number;
        if(weave_shop_uint(total, &number) && number <= 2000000)
            app->catalog_install_total_bytes = number;
        if(path && !strncmp(path, "/ext/apps/", 10) && strlen(path) < sizeof(app->catalog_install_path) &&
           !strstr(path, "..") && strstr(path, ".fap")) {
            if(strcmp(path, app->catalog_install_path)) {
                strlcpy(app->catalog_install_path, path, sizeof(app->catalog_install_path));
                FileInfo previous;
                if(storage_common_stat(app->storage, path, &previous) == FSE_OK) {
                    app->catalog_install_initial_bytes = previous.size > UINT32_MAX ?
                        UINT32_MAX : (uint32_t)previous.size;
                    app->catalog_install_new_file = false;
                } else {
                    app->catalog_install_initial_bytes = 0;
                    app->catalog_install_new_file = true;
                }
            }
        }
    }
}

/* Existing file size on SD is the verified source of truth while the RPC
 * transport is busy. Never display 100% before the companion confirms it. */
static void weave_shop_install_measure(WeavePackageApp* app) {
    if(app->catalog_install_state != WeaveInstallInstalling ||
       !app->catalog_install_total_bytes || !app->catalog_install_path[0]) return;
    FileInfo info;
    if(storage_common_stat(app->storage, app->catalog_install_path, &info) != FSE_OK ||
       info.size > app->catalog_install_total_bytes) return;
    uint32_t bytes = (uint32_t)info.size;
    /* On upgrades, ignore a pre-existing complete FAP until the RPC upload
     * has visibly truncated or overwritten it. Never count stale bytes. */
    if(!app->catalog_install_new_file) {
        if(bytes < app->catalog_install_initial_bytes ||
           bytes > app->catalog_install_initial_bytes)
            app->catalog_install_new_file = true;
        else return;
    }
    if(bytes > app->catalog_install_last_bytes) {
        app->catalog_install_last_bytes = bytes;
        app->catalog_install_last_activity_tick = app->catalog_install_ticks;
        app->catalog_install_percent = (uint8_t)((uint64_t)bytes * 99 /
                                                  app->catalog_install_total_bytes);
        app->catalog_install_percent_valid = true;
        uint32_t elapsed = (app->catalog_install_ticks - app->catalog_install_transfer_tick) / 5;
        if(elapsed > 1 && bytes > 0 && bytes < app->catalog_install_total_bytes)
            app->catalog_install_eta_seconds = (uint32_t)(((uint64_t)
                (app->catalog_install_total_bytes - bytes) * elapsed) / bytes);
        else app->catalog_install_eta_seconds = 0;
    }
}

/* Keep the infinite ring through the whole transfer and verification.
 * Only the verified DONE state starts the independent 0..100% animation.
 * The native app exits automatically after a brief 100% hold.
 */
void weave_packages_catalog_install_tick(WeavePackageApp* app) {
    if(!app->catalog_install_request[0]) return;
    app->catalog_install_ticks++;
    app->catalog_install_spinner++;
    if(!weave_shop_install_finished(app)) {
        if(app->catalog_install_ticks % SHOP_POLL_TICKS == 0) {
            weave_shop_install_poll(app);
            weave_shop_install_measure(app);
        }
        if(!weave_shop_install_finished(app) &&
           ((app->catalog_install_state == WeaveInstallWaiting &&
             app->catalog_install_ticks >= SHOP_WAIT_TIMEOUT) ||
            app->catalog_install_ticks >= SHOP_INSTALL_TIMEOUT ||
            (app->catalog_install_ticks - app->catalog_install_last_activity_tick >=
                 SHOP_IDLE_TIMEOUT)))
            app->catalog_install_state = WeaveInstallFailed;
    }
    if(app->catalog_install_state == WeaveInstallDone) {
        /* Real FAP verified. Start the cosmetic progress at 0%, never during
         * the upload. At 200 ms/tick, 8% per tick runs for ~2.6 seconds. */
        if(app->catalog_install_visual_percent < 100) {
            const uint8_t left = 100 - app->catalog_install_visual_percent;
            app->catalog_install_visual_percent += left > 8 ? 8 : left;
            app->catalog_install_done_ticks = 0;
        } else if(++app->catalog_install_done_ticks >= 4) {
            app->catalog_install_request[0] = 0;
            view_dispatcher_stop(app->dispatcher);
            return;
        }
    } else if(app->catalog_install_state == WeaveInstallFailed) {
        /* A failed queued job must not keep the viewer open forever and
         * prevent Android from launching the next app. Show the error first. */
        if(++app->catalog_install_done_ticks >= 15) {
            app->catalog_install_request[0] = 0;
            view_dispatcher_stop(app->dispatcher);
            return;
        }
    }
    if(app->page == WeavePackagePageCatalogInstall)
        view_commit_model(app->catalog_install, true);
    else if(app->page == WeavePackagePageCatalogDetail)
        view_commit_model(app->catalog_detail, true);
}

/* 16 points on a 12-pixel circle. A 5-segment rotating arc uses no floating
 * point/trigonometry and stays well inside the Flipper's 128x64 framebuffer. */
static void weave_shop_draw_spinner(Canvas* canvas, uint8_t frame) {
    static const int8_t xs[16] = {12,11,8,5,0,-5,-8,-11,-12,-11,-8,-5,0,5,8,11};
    static const int8_t ys[16] = {0,5,8,11,12,11,8,5,0,-5,-8,-11,-12,-11,-8,-5};
    const uint8_t first = frame & 15;
    for(uint8_t i = 0; i < 5; i++) {
        uint8_t a = (first + i) & 15;
        uint8_t b = (a + 1) & 15;
        canvas_draw_line(canvas, 64 + xs[a], 40 + ys[a],
                         64 + xs[b], 40 + ys[b]);
    }
}

void weave_packages_catalog_install_draw(Canvas* canvas, void* model) {
    const WeavePackageApp* app = *(WeavePackageApp**)model;
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);

    char title[sizeof(app->catalog_install_title)];
    snprintf(title, sizeof(title), "%s",
        app->catalog_install_title[0] ? app->catalog_install_title : "Application");
    canvas_set_font(canvas, FontPrimary);
    if(canvas_string_width(canvas, title) > 120) {
        size_t n = strlen(title);
        while(n > 4 && canvas_string_width(canvas, title) > 120) {
            n--;
            memcpy(title + n - 3, "...", 4);
        }
    }
    canvas_draw_str_aligned(canvas, 64, 15, AlignCenter, AlignCenter, title);

    if(app->catalog_install_state == WeaveInstallWaiting ||
       app->catalog_install_state == WeaveInstallDownloading ||
       app->catalog_install_state == WeaveInstallInstalling) {
        weave_shop_draw_spinner(canvas, app->catalog_install_spinner);
    } else if(app->catalog_install_state == WeaveInstallFailed) {
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str_aligned(canvas, 64, 43, AlignCenter, AlignCenter, "FAILED");
    } else {
        /* Bar appears only after the FAP has been verified as installed. */
        elements_progress_bar(canvas, 14, 31, 100,
            (float)app->catalog_install_visual_percent / 100.0f);
        char value[8];
        snprintf(value, sizeof(value), "%u%%",
            (unsigned)app->catalog_install_visual_percent);
        canvas_draw_str_aligned(canvas, 64, 53, AlignCenter, AlignCenter, value);
    }
}

bool weave_packages_catalog_install_input(InputEvent* event, void* context) {
    WeavePackageApp* app = context;
    /* Completion is automatic; prevent accidental early exit while animating. */
    if(app->catalog_install_state != WeaveInstallFailed) return true;
    if(event->type == InputTypeShort &&
       (event->key == InputKeyOk || event->key == InputKeyBack)) {
        app->catalog_install_request[0] = 0;
        view_dispatcher_stop(app->dispatcher);
    }
    return true;
}

static void weave_shop_search_done(void* context) {
    WeavePackageApp* app = context;
    /* Commit only when the user confirms; BACK keeps the previous filter. */
    strlcpy(app->catalog_query, app->catalog_query_draft, sizeof(app->catalog_query));
    app->catalog_offset = 0;
    app->catalog_selected = 0;
    app->catalog_scroll = 0;
    weave_packages_catalog(app);
}

void weave_packages_catalog_entry(void* context, uint32_t index) {
    WeavePackageApp* app = context;
    if(index == SHOP_SEARCH) {
        strlcpy(app->catalog_query_draft, app->catalog_query,
            sizeof(app->catalog_query_draft));
        text_input_reset(app->catalog_search_input);
        text_input_set_header_text(app->catalog_search_input, "Search catalog");
        text_input_set_result_callback(app->catalog_search_input, weave_shop_search_done, app,
            app->catalog_query_draft, sizeof(app->catalog_query_draft), false);
        app->page = WeavePackagePageCatalogSearch;
        view_dispatcher_switch_to_view(app->dispatcher, 4);
        return;
    }
    if(index >= app->entry_count) return;
    app->catalog_detail_index = index;
    app->catalog_detail_offset = 0;
    weave_shop_refresh_detail_state(app);
    app->page = WeavePackagePageCatalogDetail;
    view_commit_model(app->catalog_detail, true);
    view_dispatcher_switch_to_view(app->dispatcher, 3);
}

static void weave_shop_scroll_to_selection(WeavePackageApp* app);

void weave_packages_catalog(WeavePackageApp* app) {
    bool more = false;
    bool found = weave_shop_load(app, &more);
    app->page = WeavePackagePageCatalog;
    app->catalog_has_more = more;
    if(!found) {
        weave_packages_notice(app, "Open Weave companion\nto sync the Shop.");
        weave_packages_main(app);
        return;
    }
    if(!app->entry_count && !app->catalog_offset && !app->catalog_query[0]) {
        weave_packages_notice(app, "Catalog is empty.\nRefresh in companion.");
        weave_packages_main(app);
        return;
    }
    if(app->catalog_selected > app->entry_count) app->catalog_selected = 0;
    weave_shop_scroll_to_selection(app);
    view_commit_model(app->catalog_view, true);
    view_dispatcher_switch_to_view(app->dispatcher, 5);
}

/* Search stays at the top of a vertically scrolling list. There are no
 * visible list pages or left/right navigation; 12-entry windows are loaded
 * transparently as the selection reaches a cache boundary. */
static const Icon* weave_shop_list_icon(const WeavePackageApp* app, uint32_t index) {
    const char* description = furi_string_get_cstr(app->catalog_descriptions[index]);
    char category[24] = {0};
    size_t length = strcspn(description, "\n");
    if(length >= sizeof(category)) length = sizeof(category) - 1;
    memcpy(category, description, length);
    return weave_packages_category_icon(category);
}

void weave_packages_catalog_draw(Canvas* canvas, void* model) {
    WeavePackageApp* app = *(WeavePackageApp**)model;
    furi_check(furi_mutex_acquire(app->entries_mutex, FuriWaitForever) == FuriStatusOk);
    canvas_clear(canvas);
    weave_ui_header(canvas, app->catalog_query[0] ? "Shop results" : "Flipper Shop");
    canvas_set_font(canvas, FontSecondary);
    for(uint32_t row = 0; row < SHOP_ROWS; ++row) {
        if(row && (uint32_t)app->catalog_scroll + row - 1 >= app->entry_count) break;
        uint8_t y = 14 + row * 12;
        bool selected = row == 0 ? app->catalog_selected == 0 :
            app->catalog_selected == app->catalog_scroll + row;
        canvas_set_color(canvas, ColorBlack);
        if(selected) {
            canvas_draw_rbox(canvas, 1, y, 124, 12, 2);
            canvas_set_color(canvas, ColorWhite);
        }
        if(row == 0) {
            canvas_draw_str(canvas, 5, y + 9, app->catalog_query[0] ?
                "Search / change filter" : "Search applications");
        } else {
            uint32_t i = (uint32_t)app->catalog_scroll + row - 1;
            if(app->icons[i]) canvas_draw_xbm(canvas, 3, y + 1, 11, 10, app->icons[i]);
            else canvas_draw_icon(canvas, 3, y + 1, weave_shop_list_icon(app, i));
            const char* label = furi_string_get_cstr(app->labels[i]);
            char shortened[24];
            snprintf(shortened, sizeof(shortened), "%.19s", label);
            canvas_draw_str(canvas, 17, y + 9, shortened);
        }
        canvas_set_color(canvas, ColorBlack);
    }
    if(!app->entry_count) canvas_draw_str(canvas, 4, 38, "No matching apps");
    /* An understated scrollbar communicates more entries without page controls. */
    if(app->entry_count > SHOP_VISIBLE || app->catalog_has_more || app->catalog_offset) {
        uint8_t height = 8;
        uint8_t range = app->entry_count > SHOP_VISIBLE ? app->entry_count - SHOP_VISIBLE : 0;
        uint8_t y = 27 + (range ? (32 * app->catalog_scroll / range) : 0);
        canvas_draw_frame(canvas, 126, 26, 2, 37);
        canvas_draw_box(canvas, 126, y, 2, height);
    }
    furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
}

static void weave_shop_scroll_to_selection(WeavePackageApp* app) {
    if(app->catalog_selected == 0) {
        app->catalog_scroll = 0;
    } else if(app->catalog_selected <= app->catalog_scroll) {
        app->catalog_scroll = app->catalog_selected - 1;
    } else if(app->catalog_selected > (uint32_t)app->catalog_scroll + SHOP_VISIBLE) {
        app->catalog_scroll = app->catalog_selected - SHOP_VISIBLE;
    }
}

bool weave_packages_catalog_input(InputEvent* event, void* context) {
    WeavePackageApp* app = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return false;
    if(event->key == InputKeyLeft || event->key == InputKeyRight) {
        /* Horizontal navigation belongs exclusively to the detail screen. */
        return true;
    }
    if(event->key == InputKeyUp) {
        if(app->catalog_selected > 1) {
            app->catalog_selected--;
        } else if(app->catalog_selected == 1 && app->catalog_offset >= SHOP_WINDOW_SIZE) {
            app->catalog_offset -= SHOP_WINDOW_SIZE;
            bool more = false;
            if(weave_shop_load(app, &more) && app->entry_count) {
                app->catalog_has_more = true; /* Current window has a successor. */
                app->catalog_selected = app->entry_count;
                app->catalog_scroll = app->entry_count > SHOP_VISIBLE ?
                    app->entry_count - SHOP_VISIBLE : 0;
            } else {
                app->catalog_selected = 0;
                app->catalog_scroll = 0;
            }
        } else if(app->catalog_selected == 1) {
            app->catalog_selected = 0;
        }
        weave_shop_scroll_to_selection(app);
        view_commit_model(app->catalog_view, true);
        return true;
    }
    if(event->key == InputKeyDown) {
        if(app->catalog_selected == 0 && app->entry_count) {
            /* Search is the first element, even after a completed search. */
            app->catalog_selected = 1;
        } else if(app->catalog_selected < app->entry_count) {
            app->catalog_selected++;
        } else if(app->catalog_has_more) {
            app->catalog_offset += SHOP_WINDOW_SIZE;
            bool more = false;
            if(weave_shop_load(app, &more)) {
                app->catalog_has_more = more;
                app->catalog_selected = app->entry_count ? 1 : 0;
                app->catalog_scroll = 0;
            }
        }
        weave_shop_scroll_to_selection(app);
        view_commit_model(app->catalog_view, true);
        return true;
    }
    if(event->key == InputKeyOk && event->type == InputTypeShort) {
        weave_packages_catalog_entry(app, app->catalog_selected == 0 ?
            SHOP_SEARCH : app->catalog_selected - 1);
        return true;
    }
    return false;
}

/* One-screen native app details on the 128x64 display. No modal dialog
 * nested inside another modal; Back returns to the same catalog page. */
static void weave_shop_draw_line(Canvas* canvas, int y, const char* text, size_t offset) {
    char row[27] = {0};
    size_t length = strlen(text);
    if(offset >= length) return;
    for(size_t i = 0; i < SHOP_DETAIL_COLUMNS && offset + i < length; i++) {
        char c = text[offset + i];
        row[i] = c == '\n' || c == '\r' ? ' ' : c;
    }
    canvas_draw_str(canvas, 8, y, row);
}

static void weave_shop_uninstall(WeavePackageApp* app, uint32_t index) {
    char path[160];
    bool update = false;
    if(!weave_shop_entry_installed(app, index, path, sizeof(path), &update)) {
        weave_packages_notice(app, "FAP is not installed.");
        view_commit_model(app->catalog_detail, true);
        return;
    }
    DialogMessage* dialog = dialog_message_alloc();
    dialog_message_set_header(dialog, "Uninstall FAP?", 64, 5, AlignCenter, AlignTop);
    dialog_message_set_text(dialog, "Remove this application?", 64, 30, AlignCenter, AlignCenter);
    dialog_message_set_buttons(dialog, "Cancel", NULL, "Remove");
    bool confirmed = dialog_message_show(app->dialogs, dialog) == DialogMessageButtonRight;
    dialog_message_free(dialog);
    if(!confirmed) return;
    // Réutiliser les mêmes gardes FAP pour Shop et Uninstall.
    furi_string_set(app->path, path);
    bool removed = weave_packages_remove(app, false);
    weave_shop_refresh_detail_state(app);
    weave_packages_notice(app, removed ? "FAP uninstalled." : "Uninstall failed.");
    view_commit_model(app->catalog_detail, true);
}

void weave_packages_catalog_detail_draw(Canvas* canvas, void* model) {
    WeavePackageApp* app = *(WeavePackageApp**)model;
    furi_check(furi_mutex_acquire(app->entries_mutex, FuriWaitForever) == FuriStatusOk);
    if(app->catalog_detail_index >= app->entry_count) {
        furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
        return;
    }
    uint32_t index = app->catalog_detail_index;
    const char* title = furi_string_get_cstr(app->labels[index]);
    const char* description = furi_string_get_cstr(app->catalog_descriptions[index]);
    canvas_clear(canvas);
    canvas_set_color(canvas, ColorBlack);
    char category[21]={0};
    size_t len=strcspn(description,"\n");
    if(len>20)len=20;
    memcpy(category,description,len);
    const char* details=strchr(description,'\n');
    details=details?details+1:description;
    if(app->icons[index]) canvas_draw_xbm(canvas,3,3,11,10,app->icons[index]);
    else canvas_draw_icon(canvas,3,3,weave_packages_category_icon(category));
    canvas_set_font(canvas, FontPrimary);
    char title_line[20];
    snprintf(title_line,sizeof(title_line),"%.18s",title);
    while(title_line[0] && canvas_string_width(canvas, title_line) > 105)
        title_line[strlen(title_line) - 1] = '\0';
    canvas_draw_str(canvas,20,11,title_line);
    canvas_set_font(canvas,FontSecondary);
    canvas_draw_str(canvas,20,21,category);
    canvas_draw_line(canvas, 1, 23, 126, 23);
    weave_shop_draw_line(canvas, 33, details, app->catalog_detail_offset);
    weave_shop_draw_line(canvas, 43, details, app->catalog_detail_offset + SHOP_DETAIL_COLUMNS);
    weave_shop_draw_line(canvas, 53, details, app->catalog_detail_offset + SHOP_DETAIL_COLUMNS * 2);
    canvas_draw_line(canvas, 0, 55, 127, 55);
    canvas_draw_str(canvas, 3, 63, "Back");
    if(strlen(details) > SHOP_DETAIL_COLUMNS * 3) {
        char position[12];
        snprintf(position, sizeof(position), "%u/%u",
            (unsigned)(app->catalog_detail_offset / (SHOP_DETAIL_COLUMNS * 3) + 1),
            (unsigned)((strlen(details) - 1) / (SHOP_DETAIL_COLUMNS * 3) + 1));
        canvas_draw_str_aligned(canvas, 47, 63, AlignCenter, AlignBottom, position);
    }
    const bool active = app->catalog_install_request[0] &&
        !weave_shop_install_finished(app);
    /* Compare alias before the token separator. */
    size_t alias_len = strcspn(app->catalog_install_request, "|");
    const char* id = furi_string_get_cstr(app->entries[index]);
    const bool installing = active && strlen(id) == alias_len &&
        !strncmp(id, app->catalog_install_request, alias_len);
    canvas_draw_str_aligned(canvas, 125, 63, AlignRight, AlignBottom,
        installing ? "Progress" : !app->catalog_detail_installed ? "OK Install" :
        app->catalog_detail_update ? "OK Update" : "OK Uninstall");
    furi_check(furi_mutex_release(app->entries_mutex) == FuriStatusOk);
}

bool weave_packages_catalog_detail_input(InputEvent* event, void* context) {
    WeavePackageApp* app = context;
    if(event->type != InputTypeShort && event->type != InputTypeRepeat) return true;
    if(event->key == InputKeyBack && event->type == InputTypeShort) {
        app->page = WeavePackagePageCatalog;
        view_dispatcher_switch_to_view(app->dispatcher, 5);
    } else if(event->key == InputKeyOk && event->type == InputTypeShort &&
              app->catalog_detail_index < app->entry_count) {
        const char* selected_id = furi_string_get_cstr(app->entries[app->catalog_detail_index]);
        const size_t token_len = strcspn(app->catalog_install_request, "|");
        const bool installing = app->catalog_install_request[0] &&
            !weave_shop_install_finished(app) &&
            strlen(selected_id) == token_len &&
            !strncmp(selected_id, app->catalog_install_request, token_len);
        if(!installing && app->catalog_install_request[0] && !weave_shop_install_finished(app)) {
            weave_packages_notice(app, "Install already running.\nCheck current app first.");
        } else if(!installing) {
            weave_shop_refresh_detail_state(app);
            if(app->catalog_detail_installed && !app->catalog_detail_update)
                weave_shop_uninstall(app, app->catalog_detail_index);
            else weave_shop_write_request(app, selected_id);
        } else {
            app->page = WeavePackagePageCatalogInstall;
            view_dispatcher_switch_to_view(app->dispatcher, 6);
        }
    } else if(event->key == InputKeyDown || event->key == InputKeyRight ||
              event->key == InputKeyUp || event->key == InputKeyLeft) {
        const char* description = furi_string_get_cstr(
            app->catalog_descriptions[app->catalog_detail_index]);
        const char* details = strchr(description, '\n');
        if(details) description = details + 1;
        size_t length = strlen(description);
        const size_t step = (event->key == InputKeyRight || event->key == InputKeyLeft) ?
            SHOP_DETAIL_COLUMNS * 3 : SHOP_DETAIL_COLUMNS;
        if(event->key == InputKeyDown || event->key == InputKeyRight) {
            if((size_t)app->catalog_detail_offset + step < length &&
               step <= (size_t)UINT8_MAX - app->catalog_detail_offset)
                app->catalog_detail_offset += step;
        } else if(app->catalog_detail_offset >= step) {
            app->catalog_detail_offset -= step;
        } else {
            app->catalog_detail_offset = 0;
        }
        view_commit_model(app->catalog_detail, true);
    }
    return true;
}
