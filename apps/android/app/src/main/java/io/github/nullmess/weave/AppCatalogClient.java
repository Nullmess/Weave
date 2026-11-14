package io.github.nullmess.weave;

import android.content.Context;
import android.content.SharedPreferences;
import android.graphics.Bitmap;
import android.graphics.Color;
import android.graphics.BitmapFactory;
import android.os.Handler;
import android.os.Looper;
import android.util.LruCache;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.security.MessageDigest;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.net.URLEncoder;
import java.util.ArrayList;
import java.util.Locale;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.CompletionService;
import java.util.concurrent.CopyOnWriteArrayList;
import java.util.concurrent.ExecutorCompletionService;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.Future;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.TimeUnit;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Read-only public catalog. No local install or version state is inferred from these records. */
final class AppCatalogClient {
    static final class Entry {
        final String id, title, category, version, iconUrl, pageUrl, description;
        final String sourceUrl, manifestUrl, descriptionFileUrl, descriptionFallbackUrl;
        final String archiveId;
        final List<String> screenshots;
        final long updated, views;

        Entry(String id, String title, String category, String version, String iconUrl,
                String pageUrl, String description, String sourceUrl, String manifestUrl,
                String descriptionFileUrl, List<String> screenshots, long updated, long views) {
            this(id, title, category, version, iconUrl, pageUrl, description, sourceUrl,
                    manifestUrl, descriptionFileUrl, screenshots, updated, views, null);
        }

        Entry(String id, String title, String category, String version, String iconUrl,
                String pageUrl, String description, String sourceUrl, String manifestUrl,
                String descriptionFileUrl, List<String> screenshots, long updated, long views,
                String archiveId) {
            this(id, title, category, version, iconUrl, pageUrl, description, sourceUrl,
                    manifestUrl, descriptionFileUrl, screenshots, updated, views, archiveId, null);
        }

        Entry(String id, String title, String category, String version, String iconUrl,
                String pageUrl, String description, String sourceUrl, String manifestUrl,
                String descriptionFileUrl, List<String> screenshots, long updated, long views,
                String archiveId, String descriptionFallbackUrl) {
            this.archiveId = archiveId;
            this.descriptionFallbackUrl = descriptionFallbackUrl;
            this.id = id;
            this.title = title;
            this.category = category;
            this.version = version;
            this.iconUrl = iconUrl;
            this.pageUrl = pageUrl;
            this.description = description;
            this.sourceUrl = sourceUrl;
            this.manifestUrl = manifestUrl;
            this.descriptionFileUrl = descriptionFileUrl;
            this.screenshots = Collections.unmodifiableList(new ArrayList<>(screenshots));
            this.updated = updated;
            this.views = views;
        }
    }

    interface Callback { void completed(List<Entry> result, String error); }
    interface IconCallback { void ready(Bitmap bitmap); }
    interface DetailCallback { void ready(String description); }
    interface FapCallback { void ready(byte[] data, String error); }
    interface DownloadProgress { void changed(long bytes, long total); }
    static final class DownloadTicket {
        volatile boolean cancelled;
        volatile HttpURLConnection connection;
        void cancel() {
            cancelled = true;
            HttpURLConnection active = connection;
            if (active != null) active.disconnect();
        }
    }
    interface BuildChoicesCallback { void ready(List<String> apis, String error); }

    private static final long CATALOG_TTL_MS = 12L * 60 * 60 * 1000;
    private static final int CATALOG_CACHE_MAX_BYTES = 6_000_000;
    private final File catalogCacheFile;
    private final File iconCacheDir;
    private final SharedPreferences resolvedIconCache;
    private static final long ICON_CACHE_MAX_BYTES = 96L * 1024 * 1024;
    private volatile long lastImageCacheTrimAt;

    AppCatalogClient() { catalogCacheFile = null; iconCacheDir = null; resolvedIconCache = null; }

    AppCatalogClient(Context context) {
        catalogCacheFile = new File(context.getApplicationContext().getFilesDir(),
                "weave-app-catalog.json");
        iconCacheDir = new File(context.getApplicationContext().getFilesDir(), "weave-images");
        resolvedIconCache = context.getApplicationContext().getSharedPreferences(
                "weave-resolved-icon-urls", Context.MODE_PRIVATE);
    }

    /** Persist only published metadata, never app files or device installation state. */
    private List<Entry> cachedCatalog() {
        if (catalogCacheFile == null || !catalogCacheFile.isFile() ||
                catalogCacheFile.length() > CATALOG_CACHE_MAX_BYTES) return Collections.emptyList();
        try (FileInputStream stream = new FileInputStream(catalogCacheFile)) {
            byte[] bytes = readLimited(stream, CATALOG_CACHE_MAX_BYTES);
            JSONArray catalog = new JSONArray(new String(bytes, java.nio.charset.StandardCharsets.UTF_8));
            List<Entry> entries = new ArrayList<>();
            for (int i = 0; i < catalog.length(); i++) {
                JSONObject row = catalog.optJSONObject(i);
                if (row == null) continue;
                String id = row.optString("id", "");
                if (!id.matches("[A-Za-z0-9_-]{2,100}")) continue;
                JSONArray shots = row.optJSONArray("screenshots");
                List<String> screenshots = new ArrayList<>();
                if (shots != null) for (int j = 0; j < shots.length(); j++)
                    screenshots.add(shots.optString(j));
                entries.add(new Entry(id, row.optString("title", id),
                        row.optString("category", "Other"), nullable(row, "version"),
                        nullable(row, "iconUrl"), nullable(row, "pageUrl"),
                        nullable(row, "description"), nullable(row, "sourceUrl"),
                        nullable(row, "manifestUrl"), nullable(row, "descriptionFileUrl"),
                        screenshots, row.optLong("updated"), row.optLong("views"),
                        nullable(row, "archiveId"), nullable(row, "descriptionFallbackUrl")));
            }
            return entries;
        } catch (Exception invalid) { return Collections.emptyList(); }
    }

    private static String nullable(JSONObject obj, String field) {
        return obj.isNull(field) ? null : obj.optString(field, null);
    }

    private static void putOptional(JSONObject json, String key, String value) throws Exception {
        json.put(key, value == null ? JSONObject.NULL : value);
    }

    private void cacheCatalog(List<Entry> entries) {
        if (catalogCacheFile == null || entries == null || entries.isEmpty()) return;
        try {
            JSONArray catalog = new JSONArray();
            for (Entry entry : entries) {
                JSONObject row = new JSONObject();
                row.put("id", entry.id);
                row.put("title", entry.title);
                row.put("category", entry.category);
                putOptional(row, "version", entry.version);
                putOptional(row, "iconUrl", entry.iconUrl);
                putOptional(row, "pageUrl", entry.pageUrl);
                putOptional(row, "description", entry.description);
                putOptional(row, "sourceUrl", entry.sourceUrl);
                putOptional(row, "manifestUrl", entry.manifestUrl);
                putOptional(row, "descriptionFileUrl", entry.descriptionFileUrl);
                putOptional(row, "archiveId", entry.archiveId);
                putOptional(row, "descriptionFallbackUrl", entry.descriptionFallbackUrl);
                row.put("updated", entry.updated);
                row.put("views", entry.views);
                row.put("screenshots", new JSONArray(entry.screenshots));
                catalog.put(row);
            }
            byte[] bytes = catalog.toString().getBytes(java.nio.charset.StandardCharsets.UTF_8);
            if (bytes.length > CATALOG_CACHE_MAX_BYTES) return;
            File temp = new File(catalogCacheFile.getParentFile(), "weave-app-catalog.json.tmp");
            try (FileOutputStream stream = new FileOutputStream(temp)) {
                stream.write(bytes);
                stream.getFD().sync();
            }
            if (!temp.renameTo(catalogCacheFile)) {
                // Leave the last complete cache intact on a failed replacement.
                temp.delete();
            }
        } catch (Exception ignored) { /* An inaccessible cache must not break browsing. */ }
    }

    /** Optional 11x10 monochrome artwork for the Flipper's 128x64 Shop. */
    String cachedDeviceIconHex(Entry entry) {
        if (entry == null || entry.id == null) return null;
        Bitmap bitmap = appIcons.get(entry.id.toLowerCase(Locale.ROOT));
        if (bitmap == null && entry.iconUrl != null) bitmap = images.get(entry.iconUrl);
        if (bitmap == null) return null;
        byte[] data = new byte[20]; // 2 bytes per row, 10 rows.
        for (int y = 0; y < 10; y++) for (int x = 0; x < 11; x++) {
            int pixel = bitmap.getPixel(Math.min(bitmap.getWidth() - 1, x * bitmap.getWidth() / 11),
                    Math.min(bitmap.getHeight() - 1, y * bitmap.getHeight() / 10));
            int luma = (Color.red(pixel) * 299 + Color.green(pixel) * 587 +
                    Color.blue(pixel) * 114) / 1000;
            if (Color.alpha(pixel) >= 96 && luma < 156)
                data[y * 2 + (x >> 3)] |= (byte)(1 << (x & 7));
        }
        StringBuilder hex = new StringBuilder(40);
        for (byte value : data) hex.append(String.format(Locale.ROOT, "%02x", value & 0xff));
        return hex.toString();
    }

    private static final String HOST = "https://catalog.flipperzero.one";
    private static final String TREE = "https://api.github.com/repos/flipperdevices/flipper-application-catalog/git/trees/main?recursive=1";
    private static final String RAW = "https://raw.githubusercontent.com/flipperdevices/flipper-application-catalog/main/";
    private final ExecutorService worker = Executors.newFixedThreadPool(6);
    // User search is replace-latest; never let old queries keep fetching tens
    // of pages while Refresh, icons and a newer search compete for bandwidth.
    private final AtomicInteger searchGeneration = new AtomicInteger();
    private Future<?> activeSearchTask;
    private final Handler main = new Handler(Looper.getMainLooper());
    // RAM LRU : le cache persistant reste sur disque, pas en heap Android.
    private final LruCache<String, Bitmap> images = new LruCache<String, Bitmap>(24 * 1024 * 1024) {
        @Override protected int sizeOf(String key, Bitmap value) { return value.getByteCount(); }
    };
    // Resolve missing/broken published icons using the official catalog manifest,
    // then the app's committed application.fam. One lookup per app at a time.
    private final LruCache<String, Bitmap> appIcons = new LruCache<String, Bitmap>(12 * 1024 * 1024) {
        @Override protected int sizeOf(String key, Bitmap value) { return value.getByteCount(); }
    };
    private final Map<String, Long> iconLastFailedAt = new ConcurrentHashMap<>();
    private final Map<String, List<IconCallback>> iconWaiters = new HashMap<>();
    private final Map<String, List<IconCallback>> imageWaiters = new HashMap<>();
    private final Object manifestIndexLock = new Object();
    private Map<String, String> catalogManifestIndex;
    private final Map<String, Entry> resolvedManifests = new ConcurrentHashMap<>();
    private final Map<String, String> publishedMarkdownCache = new ConcurrentHashMap<>();
    private final Map<String, Integer> descriptionEpochs = new ConcurrentHashMap<>();
    // The catalog archive owns the built .fap. A source repository release may contain none.
    private final Map<String, Long> archivedFapSizes = new ConcurrentHashMap<>();
    private final Map<String, String> resolvedFapSizes = new ConcurrentHashMap<>();
    private final Set<String> sizeLookupsStarted = Collections.newSetFromMap(new ConcurrentHashMap<String, Boolean>());
    // A detail view may re-render during lookup. Notify all awaiting views, not just the first.
    private final Map<String, List<DetailCallback>> sizeCallbacks = new ConcurrentHashMap<>();

    void load(Callback callback) { load(callback, false); }

    void load(Callback callback, boolean forceRefresh) {
        load(callback, forceRefresh, null);
    }

    void load(Callback callback, boolean forceRefresh, Runnable refreshFinished) {
        worker.execute(() -> {
            List<Entry> cached = cachedCatalog();
            boolean fresh = !forceRefresh && !cached.isEmpty() && catalogCacheFile != null &&
                    System.currentTimeMillis() - catalogCacheFile.lastModified() < CATALOG_TTL_MS;
            if (!cached.isEmpty()) main.post(() -> callback.completed(cached, null));
            if (fresh) {
                if (refreshFinished != null) main.post(refreshFinished);
                return;
            }
            // Only remote refresh invalidates metadata and lookup failures.
            sizeLookupsStarted.clear();
            sizeCallbacks.clear();
            iconLastFailedAt.clear();
            publishedMarkdownCache.clear();
            List<Entry> entries = Collections.emptyList();
            String error = null;
            try {
                entries = loadArchivarius();
                if (entries.isEmpty()) throw new IOException("No apps");
            } catch (Exception unavailable) {
                try {
                    entries = loadGithubManifests();
                    if (entries.isEmpty()) throw new IOException("No manifests");
                } catch (Exception secondary) {
                    error = "Catalog unavailable. Tap refresh to retry.";
                }
            }
            // Keep the public TOTP record visible even when older catalog API pages
            // omit it. The actual .fap must still be verified before installation.
            if (!entries.isEmpty()) entries = withKnownPublicApps(entries);
            if (!entries.isEmpty()) cacheCatalog(entries);
            List<Entry> result = entries;
            String message = !cached.isEmpty() && entries.isEmpty() ? null : error;
            main.post(() -> callback.completed(result, message));
            // Some installations of the public archive API return only the first
            // page repeatedly. Show that page immediately, then supplement it
            // from the official GitHub manifest index in the background.
            if (error == null && entries.size() <= 150) {
                try {
                    List<Entry> fromGitHub = loadGithubManifests();
                    Map<String, Entry> merged = new LinkedHashMap<>();
                    for (Entry entry : entries) merged.put(entry.id, entry);
                    for (Entry entry : fromGitHub) merged.putIfAbsent(entry.id, entry);
                    if (merged.size() > entries.size()) {
                        List<Entry> expanded = withKnownPublicApps(new ArrayList<>(merged.values()));
                        cacheCatalog(expanded);
                        main.post(() -> callback.completed(expanded, null));
                    }
                } catch (Exception ignored) {
                    // Keep the already published official archive records.
                }
            }
            if (refreshFinished != null) main.post(refreshFinished);
        });
    }

    private static final String TOTP_ARCHIVE_ID = "672735a56cb04a64f7ee03ab";

    private List<Entry> withKnownPublicApps(List<Entry> entries) {
        List<Entry> complete = new ArrayList<>(entries);
        for (int i = 0; i < complete.size(); i++) {
            Entry entry = complete.get(i);
            if (!"totp".equalsIgnoreCase(entry.id)) continue;
            if (TOTP_ARCHIVE_ID.equals(entry.archiveId)) return entries;
            // A GitHub manifest may lack the archive ID; retain real metadata
            // and fill only the known canonical public catalogue identity.
            complete.set(i, new Entry(entry.id, "Authenticator", entry.category,
                    entry.version, entry.iconUrl, HOST + "/application/" + TOTP_ARCHIVE_ID + "/page",
                    entry.description, entry.sourceUrl, entry.manifestUrl,
                    entry.descriptionFileUrl, entry.screenshots, entry.updated, entry.views, TOTP_ARCHIVE_ID));
            return complete;
        }
        // Public record from catalog.flipperzero.one/application/672735a56cb04a64f7ee03ab/page.
        // The version and build are deliberately left unset until resolved live.
        complete.add(new Entry("totp", "Authenticator", "Tools", "", null,
                HOST + "/application/" + TOTP_ARCHIVE_ID + "/page",
                "Time-based and HMAC one-time password authenticator", null, null,
                null, Collections.emptyList(), 0, 0, TOTP_ARCHIVE_ID));
        return complete;
    }

    private List<Entry> loadArchivarius() throws Exception {
        return loadArchivarius(null, 10000);
    }

    /** Remote search is debounced by MainActivity; the local catalog remains available offline. */
    synchronized void cancelSearch() {
        searchGeneration.incrementAndGet();
        if (activeSearchTask != null) activeSearchTask.cancel(true);
        activeSearchTask = null;
    }

    synchronized void search(String query, Callback callback) {
        cancelSearch();
        if (query == null || query.trim().length() < 2) { load(callback); return; }
        final String term = query.trim();
        final int generation = searchGeneration.get();
        activeSearchTask = worker.submit(() -> {
            List<Entry> entries = Collections.emptyList();
            String error = null;
            try { entries = loadArchivarius(term, 1000, generation); }
            catch (Exception failure) { error = "Online search unavailable"; }
            if (generation != searchGeneration.get() || Thread.currentThread().isInterrupted()) return;
            if (term.toLowerCase(Locale.ROOT).contains("totp") ||
                    term.toLowerCase(Locale.ROOT).contains("auth"))
                entries = withKnownPublicApps(entries);
            List<Entry> result = entries;
            String message = error;
            main.post(() -> {
                if (generation == searchGeneration.get()) callback.completed(result, message);
            });
        });
    }

    private List<Entry> loadArchivarius(String search, int maxOffset) throws Exception {
        return loadArchivarius(search, maxOffset, -1);
    }

    private List<Entry> loadArchivarius(String search, int maxOffset, int searchToken) throws Exception {
        Map<String, Entry> entries = new LinkedHashMap<>();
        // The catalog contains substantially more than the first 300 apps.
        // Fetch every available page so search includes the whole official catalog.
        for (int offset = 0; offset < maxOffset; offset += 100) {
            if (Thread.currentThread().isInterrupted() ||
                    (searchToken >= 0 && searchToken != searchGeneration.get()))
                throw new IOException("Superseded catalog query");
            int beforePage = entries.size();
            String text;
            String request = HOST + "/api/v0/application?limit=100&offset=" + offset;
            if (search != null && !search.trim().isEmpty())
                request += "&search=" + URLEncoder.encode(search, "UTF-8");
            try { text = downloadText(request, 5_000_000); }
            catch (IOException networkError) {
                if (!entries.isEmpty()) break;
                throw networkError;
            }
            JSONArray list;
            if (text.trim().startsWith("[")) list = new JSONArray(text);
            else {
                JSONObject object = new JSONObject(text);
                list = object.optJSONArray("applications");
                if (list == null) list = object.optJSONArray("items");
                if (list == null) list = object.optJSONArray("data");
                if (list == null) list = object.optJSONArray("results");
            }
            if (list == null) throw new IOException("Unrecognized catalog response");
            for (int i = 0; i < list.length(); i++) {
                JSONObject obj = list.optJSONObject(i);
                if (obj == null) continue;
                String id = first(obj.optString("alias"), obj.optString("slug"), obj.optString("appid"), obj.optString("id"));
                String title = first(obj.optString("name"), obj.optString("title"));
                if (id == null || title == null || !id.matches("[A-Za-z0-9_-]{2,100}")) continue;
                // Do not re-filter official search results by their title. Flipper Lab
                // also matches app metadata, descriptions and tags (e.g. "auth").
                // Archivarius uses a 24-character application ID distinct from its alias.
                String archiveId = first(obj.optString("_id"), obj.optString("id"));
                if (archiveId != null && !archiveId.matches("[a-fA-F0-9]{24}")) archiveId = null;
                String version = first(publishedVersion(obj),
                        publishedVersion(obj.optJSONObject("latest_version")));
                String icon = imageUrl(first(stringField(obj, "icon_url"), stringField(obj, "iconUrl"), stringField(obj, "icon")));
                String category = first(stringField(obj, "category"), "Other");
                String description = first(stringField(obj, "description"), stringField(obj, "short_description"));
                String source = safeGithub(stringField(obj, "repository"));
                long archiveSize = extractFapSize(obj, 0, false);
                if (archiveSize > 0) archivedFapSizes.put(id, archiveSize);
                List<String> pictures = new ArrayList<>();
                JSONArray screenshots = obj.optJSONArray("screenshots");
                if (screenshots != null) for (int j = 0; j < screenshots.length() && j < 12; j++) {
                    Object image = screenshots.opt(j);
                    String url = image instanceof JSONObject ? first(((JSONObject) image).optString("url"), ((JSONObject) image).optString("src")) : String.valueOf(image);
                    url = imageUrl(url);
                    if (url != null) pictures.add(url);
                }
                entries.put(id, new Entry(id, title, category, version, icon,
                        HOST + "/application/" + (archiveId == null ? id : archiveId) + "/page",
                        description, source, null,
                        null, pictures, obj.optLong("updated_at", 0), obj.optLong("views", 0), archiveId));
            }
            if (list.length() < 100 || entries.size() == beforePage) break;
        }
        return new ArrayList<>(entries.values());
    }

    private List<Entry> loadGithubManifests() throws Exception {
        JSONObject response = new JSONObject(downloadText(TREE, 10_000_000));
        if (response.optBoolean("truncated")) throw new IOException("Tree truncated");
        JSONArray tree = response.getJSONArray("tree");
        Map<String, List<String>> categories = new LinkedHashMap<>();
        for (int i = 0; i < tree.length(); i++) {
            JSONObject entry = tree.optJSONObject(i);
            if (entry == null) continue;
            String path = entry.optString("path", "");
            if (!path.matches("applications/[^/]+/[^/]+/manifest\\.yml")) continue;
            String category = path.split("/")[1];
            if (!categories.containsKey(category)) categories.put(category, new ArrayList<>());
            categories.get(category).add(path);
        }
        // The Git tree is itself the complete list of public manifest paths.
        // Fetch details for a bounded subset, but show ALL remaining aliases as
        // catalog entries rather than silently dropping hundreds of apps.
        List<String> allPaths = new ArrayList<>();
        for (int pos = 0; ; pos++) {
            boolean found = false;
            for (List<String> category : categories.values()) {
                if (pos >= category.size()) continue;
                allPaths.add(category.get(pos));
                found = true;
            }
            if (!found) break;
        }
        List<String> selected = new ArrayList<>();
        for (String path : allPaths) if (path.endsWith("/totp/manifest.yml")) selected.add(path);
        for (String path : allPaths) {
            if (selected.size() >= 120) break;
            if (!selected.contains(path)) selected.add(path);
        }
        CompletionService<Entry> jobs = new ExecutorCompletionService<>(worker);
        for (String path : selected) jobs.submit(() -> {
            try { return manifestEntry(path, downloadText(RAW + path, 200_000)); }
            catch (Exception ignored) { return null; }
        });
        Map<String, Entry> entries = new LinkedHashMap<>();
        long end = System.currentTimeMillis() + 35_000;
        for (int i = 0; i < selected.size(); i++) {
            long remaining = end - System.currentTimeMillis();
            if (remaining <= 0) break;
            Future<Entry> future = jobs.poll(remaining, TimeUnit.MILLISECONDS);
            if (future == null) break;
            try {
                Entry entry = future.get();
                if (entry != null) entries.put(entry.id, entry);
            } catch (Exception ignored) { }
        }
        for (String path : allPaths) {
            String[] parts = path.split("/");
            if (parts.length != 4) continue;
            String alias = parts[2];
            if (!alias.matches("[A-Za-z0-9_-]{2,100}") || entries.containsKey(alias)) continue;
            // Name is a neutral alias label until a real manifest can be retrieved.
            entries.put(alias, new Entry(alias, alias.replace('_', ' '), parts[1], "", null,
                    "https://lab.flipper.net/apps/" + alias, null, null, null,
                    null, Collections.emptyList(), 0, 0));
        }
        return new ArrayList<>(entries.values());
    }

    private Entry manifestEntry(String path, String document) {
        String[] segments = path.split("/");
        if (segments.length != 4) return null;
        String id = segments[2];
        String category = first(scalar(document, "category"), segments[1]);
        String name = scalar(document, "name");
        String version = scalar(document, "version");
        String icon = scalar(document, "icon");
        String description = first(scalar(document, "short_description"), scalar(document, "description"));
        String descFile = scalar(document, "description");
        String source = null, base = null, fileUrl = null, descriptionFallback = null;
        List<String> screenshots = new ArrayList<>();
        // origin, commit_sha, and subdir are under sourcecode.location, NOT manifest root.
        String origin = scalar(document, "origin");
        String sha = scalar(document, "commit_sha");
        String subdir = scalar(document, "subdir");
        if (origin != null && sha != null && sha.matches("[a-fA-F0-9]{40}")) {
            source = safeGithub(origin);
            if (source != null) {
                String folder = subdir != null && safePath(subdir) ? trimSlashes(subdir) + "/" : "";
                String repo = source.substring("https://github.com/".length());
                base = "https://raw.githubusercontent.com/" + repo + "/" + sha + "/" + folder;
                descriptionFallback = base + "README.md";
                if (icon == null || version == null || name == null) {
                    // These fields are often declared in application.fam instead of manifest.yml.
                    try {
                        String fam = downloadText(base + "application.fam", 60_000);
                        if (icon == null) icon = pythonScalar(fam, "fap_icon");
                        if (version == null) version = pythonScalar(fam, "fap_version");
                        if (name == null) name = pythonScalar(fam, "name");
                    } catch (Exception ignored) { }
                }
                if (icon != null && safePath(icon) && icon.toLowerCase().endsWith(".png"))
                    fileUrl = base + trimSlashes(icon);
                for (String shot : screenshotPaths(document)) {
                    if (safePath(shot) && shot.toLowerCase().endsWith(".png") && screenshots.size() < 10)
                        screenshots.add(base + trimSlashes(shot));
                }
                if (descFile != null && descFile.startsWith("@") && safePath(descFile.substring(1)))
                    descFile = base + trimSlashes(descFile.substring(1));
                else descFile = null;
            }
        }
        if (description != null && description.startsWith("@")) description = null;
        return new Entry(id, first(name, id.replace('_', ' ')), category, version,
                fileUrl, HOST + "/application/" + id + "/page", description,
                source, "https://github.com/flipperdevices/flipper-application-catalog/blob/main/" + path,
                descFile, screenshots, 0, 0, null, descriptionFallback);
    }

    private List<String> screenshotPaths(String yml) {
        List<String> paths = new ArrayList<>();
        boolean inside = false;
        for (String line : yml.split("\\r?\\n")) {
            if (line.matches("^screenshots\\s*:\\s*(#.*)?$")) { inside = true; continue; }
            if (!inside) continue;
            if (!line.startsWith(" ") && !line.startsWith("\\t") && !line.isEmpty()) break;
            Matcher m = Pattern.compile("^\\s+-\\s+([^#]+)").matcher(line);
            if (m.find()) {
                String image = unquote(m.group(1).trim());
                if (safePath(image)) paths.add(image);
            }
        }
        return paths;
    }

    private String scalar(String yml, String key) {
        Matcher m = Pattern.compile("(?m)^\\s*" + Pattern.quote(key) + "\\s*:\\s*([^#\\r\\n]+)").matcher(yml);
        if (!m.find()) return null;
        String value = unquote(m.group(1).trim());
        return value.isEmpty() || value.equals("|") || value.equals(">") ? null : value;
    }

    private String pythonScalar(String source, String key) {
        Matcher m = Pattern.compile("(?m)\\b" + Pattern.quote(key) + "\\s*=\\s*['\\\"]([^'\\\"]+)['\\\"]").matcher(source);
        return m.find() ? m.group(1) : null;
    }

    private String unquote(String value) {
        if (value.length() > 1 && ((value.startsWith("\"") && value.endsWith("\"")) ||
                (value.startsWith("'") && value.endsWith("'")))) return value.substring(1, value.length() - 1);
        return value;
    }

    private String trimSlashes(String path) {
        return path.replaceFirst("^\\./", "").replaceAll("^/+", "");
    }

    private boolean safePath(String path) {
        return path != null && path.length() < 200 && !path.contains("..") &&
                path.matches("[A-Za-z0-9_./-]+") && !path.startsWith("/");
    }

    private String safeGithub(String origin) {
        if (origin == null || !origin.matches("https://github\\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+(?:\\.git)?/?")) return null;
        return origin.replaceFirst("\\.git/?$", "").replaceAll("/$", "");
    }

    private String stringField(JSONObject obj, String key) {
        Object value = obj.opt(key);
        if (value instanceof String) return first((String) value);
        if (value instanceof JSONObject) return first(((JSONObject) value).optString("url"),
                ((JSONObject) value).optString("src"), ((JSONObject) value).optString("link"),
                ((JSONObject) value).optString("path"), ((JSONObject) value).optString("name"),
                ((JSONObject) value).optString("title"));
        return null;
    }

    /** Versions in Archivarius vary between strings and nested release records. */
    private String publishedVersion(JSONObject data) {
        if (data == null) return null;
        Object raw = data.opt("version");
        if (raw instanceof Number) return String.valueOf(raw);
        if (raw instanceof String) return first((String) raw);
        if (raw instanceof JSONObject) {
            JSONObject v = (JSONObject) raw;
            return first(v.optString("version", null), v.optString("name", null),
                    v.optString("number", null), v.optString("tag", null), v.optString("value", null));
        }
        return null;
    }

    private Entry archivariusDetail(String alias) {
        try {
            JSONObject app = new JSONObject(downloadText(HOST + "/api/v0/application/" +
                    URLEncoder.encode(alias, "UTF-8"), 1_000_000));
            JSONObject data = app.optJSONObject("application");
            if (data == null) data = app.optJSONObject("data");
            if (data == null) data = app;
            String id = first(data.optString("alias"), data.optString("appid"), alias);
            if (!id.matches("[A-Za-z0-9_-]{2,100}")) return null;
            String uuid = first(data.optString("_id"), data.optString("id"));
            if (uuid != null && !uuid.matches("[a-fA-F0-9]{24}")) uuid = null;
            String icon = imageUrl(first(stringField(data, "icon_url"),
                    stringField(data, "iconUrl"), stringField(data, "icon")));
            String version = first(publishedVersion(data),
                    publishedVersion(data.optJSONObject("latest_version")),
                    publishedVersion(data.optJSONObject("latestVersion")));
            return new Entry(id, first(data.optString("name"), data.optString("title"), alias),
                    first(stringField(data, "category"), "Tools"), version, icon,
                    HOST + "/application/" + (uuid == null ? id : uuid) + "/page",
                    first(stringField(data, "description"), stringField(data, "short_description")),
                    safeGithub(stringField(data, "repository")), null, null,
                    Collections.emptyList(), 0, 0, uuid);
        } catch (Exception ignored) { return null; }
    }

    private String latestPublishedPageVersion(Entry entry) {
        String id = entry.archiveId == null ? entry.id : entry.archiveId;
        try {
            String page = downloadText(HOST + "/application/" + id + "/page", 2_000_000);
            Matcher rows = VERSION_ROW.matcher(page);
            while (rows.find()) {
                String row = rows.group(1);
                if (!OFFICIAL_F7_BUILD.matcher(row).find()) continue;
                Matcher cell = CELL.matcher(row);
                if (!cell.find()) continue;
                String version = cell.group(1).replaceAll("<[^>]+>", "").trim();
                if (version.matches("[vV]?[0-9]+(?:\\.[0-9]+){1,3}")) return version;
            }
        } catch (Exception ignored) { }
        return null;
    }

    /** Enrich only when an app's detail is opened; no guessed icons or release numbers. */
    void enrichMetadata(Entry entry, java.util.function.Consumer<Entry> callback) {
        if (entry == null) { main.post(() -> callback.accept(null)); return; }
        worker.execute(() -> {
            Entry remote = archivariusDetail(entry.id);
            Entry manifest = resolveCatalogManifest(entry);
            String version = first(remote == null ? null : remote.version,
                    manifest == null ? null : manifest.version, entry.version);
            String icon = first(entry.iconUrl, remote == null ? null : remote.iconUrl,
                    manifest == null ? null : manifest.iconUrl);
            String desc = first(entry.description, remote == null ? null : remote.description,
                    manifest == null ? null : manifest.description);
            String source = first(entry.sourceUrl, manifest == null ? null : manifest.sourceUrl,
                    remote == null ? null : remote.sourceUrl);
            String archiveId = first(entry.archiveId, remote == null ? null : remote.archiveId);
            if (version == null) {
                Entry lookup = new Entry(entry.id, entry.title, entry.category, null, icon,
                        entry.pageUrl, desc, source, entry.manifestUrl,
                        entry.descriptionFileUrl, entry.screenshots, entry.updated, entry.views, archiveId);
                version = latestPublishedPageVersion(lookup);
            }
            List<String> screenshots = !entry.screenshots.isEmpty() ? entry.screenshots :
                    manifest != null && !manifest.screenshots.isEmpty() ? manifest.screenshots :
                    remote == null ? Collections.emptyList() : remote.screenshots;
            Entry ready = new Entry(entry.id, first(entry.title, remote == null ? null : remote.title),
                    first(entry.category, manifest == null ? null : manifest.category),
                    version, imageUrl(icon), first(entry.pageUrl, remote == null ? null : remote.pageUrl),
                    desc, source,
                    first(entry.manifestUrl, manifest == null ? null : manifest.manifestUrl),
                    first(entry.descriptionFileUrl, manifest == null ? null : manifest.descriptionFileUrl),
                    screenshots, entry.updated, entry.views, archiveId,
                    manifest == null ? entry.descriptionFallbackUrl :
                            first(entry.descriptionFallbackUrl, manifest.descriptionFallbackUrl));
            main.post(() -> callback.accept(ready));
        });
    }

    private String imageUrl(String url) {
        if (url == null) return null;
        if (url.startsWith("/") && !url.startsWith("//") && !url.contains("..") && url.length() < 500)
            return HOST + url;
        if (!url.startsWith("https://")) return null;
        try {
            String host = new URL(url).getHost();
            if (host.equals("catalog.flipperzero.one") || host.equals("cdn.flipperzero.one") ||
                    host.equals("raw.githubusercontent.com") || host.equals("avatars.githubusercontent.com") ||
                    host.equals("flipper.net") || host.equals("flipperzero.one")) return url;
        } catch (Exception ignored) { }
        return null;
    }

    private String first(String... values) {
        for (String value : values)
            if (value != null && !value.trim().isEmpty() && !value.equals("null")) return value.trim();
        return null;
    }

    /** Expire only the selected app's README for an explicit detail refresh. */
    void invalidateDescription(String normalizedId) {
        if (normalizedId == null) return;
        String key = normalizedId.toLowerCase(Locale.ROOT);
        descriptionEpochs.merge(key, 1, Integer::sum);
        publishedMarkdownCache.remove(key);
    }

    void loadDescription(Entry entry, DetailCallback callback) {
        if (entry == null) { main.post(() -> callback.ready(null)); return; }
        String key = entry.id == null ? "" : entry.id.toLowerCase(Locale.ROOT);
        final int epoch = descriptionEpochs.getOrDefault(key, 0);
        String saved = publishedMarkdownCache.get(key);
        if (saved != null) { main.post(() -> callback.ready(saved)); return; }
        // An API snippet is not the whole app README. First resolve the
        // published manifest, then prefer its Markdown documentation when present.
        worker.execute(() -> {
            Entry resolved = resolveCatalogManifest(entry);
            String text = entry.description;
            if (text != null && text.trim().startsWith("@")) text = null;
            if (text == null && resolved != null) text = resolved.description;
            // A description may reference @docs/catalog.md, @README.md, etc.
            // Missing optional files never turn into an error or invalid link.
            String primary = entry.descriptionFileUrl != null ? entry.descriptionFileUrl :
                    resolved == null ? null : resolved.descriptionFileUrl;
            String fallback = entry.descriptionFallbackUrl != null ? entry.descriptionFallbackUrl :
                    resolved == null ? null : resolved.descriptionFallbackUrl;
            for (String candidate : new String[]{primary, fallback}) {
                if (candidate == null) continue;
                try {
                    String content = downloadText(candidate, 100_000);
                    if (!content.trim().isEmpty()) { text = content; break; }
                } catch (IOException ignored) { /* Description file is optional. */ }
            }
            String desc = text;
            // An older in-flight fetch must not repopulate an explicitly expired README.
            if (desc != null && !key.isEmpty() &&
                    epoch == descriptionEpochs.getOrDefault(key, 0))
                publishedMarkdownCache.put(key, desc);
            main.post(() -> callback.ready(desc));
        });
    }

    /** Load the real Archivarius record for a locally installed FAP alias. */
    void loadInstalledMetadata(String alias, java.util.function.Consumer<Entry> callback) {
        if (alias == null || !alias.matches("[A-Za-z0-9_-]{2,100}")) {
            main.post(() -> callback.accept(null)); return;
        }
        worker.execute(() -> {
            Entry entry = null;
            try {
                JSONObject app = new JSONObject(downloadText(HOST + "/api/v0/application/" +
                        URLEncoder.encode(alias, "UTF-8"), 1_000_000));
                JSONObject data = app.optJSONObject("application");
                if (data == null) data = app.optJSONObject("data");
                if (data == null) data = app;
                String id = first(data.optString("alias"), data.optString("appid"), alias);
                String title = first(data.optString("name"), data.optString("title"), alias);
                String uuid = first(data.optString("_id"), data.optString("id"));
                if (uuid != null && !uuid.matches("[a-fA-F0-9]{24}")) uuid = null;
                String icon = imageUrl(first(stringField(data, "icon_url"),
                        stringField(data, "iconUrl"), stringField(data, "icon")));
                String category = first(stringField(data, "category"), "Local");
                String version = publishedVersion(data);
                if (id.matches("[A-Za-z0-9_-]{2,100}")) entry = new Entry(id, title, category,
                        version, icon, HOST + "/application/" + (uuid == null ? id : uuid) + "/page",
                        first(stringField(data, "description"), stringField(data, "short_description")),
                        safeGithub(stringField(data, "repository")), null, null,
                        Collections.emptyList(), 0, 0, uuid);
            } catch (Exception ignored) { }
            Entry ready = entry;
            main.post(() -> callback.accept(ready));
        });
    }

    /** Select only SDK builds actually published on the official application page. */
    void listOfficialBuildApis(Entry entry, BuildChoicesCallback callback) {
        worker.execute(() -> {
            java.util.LinkedHashSet<String> apis = new java.util.LinkedHashSet<>();
            String error = null;
            try {
                for (String path : officialLinksFor(entry)) {
                    Matcher matcher = Pattern.compile("/build/f7/([0-9]+\\.[0-9]+)$").matcher(path);
                    if (matcher.find()) apis.add(matcher.group(1));
                }
                if (apis.isEmpty()) error = "No published Flipper FAP build found";
            } catch (Exception e) { error = "Unable to read the official Flipper build list"; }
            List<String> result = new ArrayList<>(apis);
            String message = error;
            main.post(() -> callback.ready(result, message));
        });
    }

    private List<String> officialLinksFor(Entry entry) throws IOException {
        java.util.LinkedHashSet<String> ids = new java.util.LinkedHashSet<>();
        if (entry.archiveId != null) ids.add(entry.archiveId);
        if (entry.id.matches("[a-fA-F0-9]{24}")) ids.add(entry.id);
        if (ids.isEmpty()) {
            String resolved = resolveOfficialApplicationId(entry.id);
            if (resolved != null) ids.add(resolved);
        }
        List<String> result = new ArrayList<>();
        for (String id : ids) {
            try {
                String html = downloadText(HOST + "/application/" + id + "/page", 2_000_000);
                for (String link : officialBuildLinks(html, entry.version))
                    if (!result.contains(link)) result.add(link);
            } catch (Exception ignored) { }
        }
        return result;
    }

    /** Download a build for the firmware API obtained from the connected Flipper RPC. */
    void downloadOfficialFap(Entry entry, String firmwareApi, FapCallback callback) {
        if (entry == null || entry.id == null || !entry.id.matches("[A-Za-z0-9_-]{2,100}")) {
            main.post(() -> callback.ready(null, "Unknown Flipper catalog application"));
            return;
        }
        if (firmwareApi == null || !firmwareApi.matches("[0-9]+\\.[0-9]+")) {
            main.post(() -> callback.ready(null, "Flipper firmware API version is not available"));
            return;
        }
        worker.execute(() -> {
            byte[] result = null;
            String error = "The official catalog has no compatible FAP build for firmware API " + firmwareApi;
            boolean sawBuild = false;
            boolean sawCompatible = false;
            List<String> builds = Collections.emptyList();
            try { builds = officialLinksFor(entry); }
            catch (Exception network) {
                error = "Official Flipper catalog is currently unavailable";
            }
            if (!builds.isEmpty()) sawBuild = true;
            for (String path : compatibleBuilds(builds, firmwareApi)) {
                sawCompatible = true;
                HttpURLConnection conn = null;
                try {
                    conn = connect(HOST + path);
                    String type = conn.getContentType();
                    if (type != null && (type.contains("text/html") || type.contains("application/json")))
                        continue;
                    try (InputStream in = conn.getInputStream()) { result = readLimited(in, 2_000_000); }
                    if (result.length > 4 && result[0] == 0x7f && result[1] == 'E'
                            && result[2] == 'L' && result[3] == 'F') break;
                    result = null;
                } catch (Exception failure) {
                    error = "Official FAP download failed: " + failure.getClass().getSimpleName();
                } finally { if (conn != null) conn.disconnect(); }
            }
            if (!sawBuild)
                error = "No downloadable FAP was listed for this app in the official archive";
            else if (sawBuild && !sawCompatible)
                error = "No official FAP build is compatible with Flipper firmware API " + firmwareApi;
            byte[] ready = result;
            String message = ready == null ? error : null;
            main.post(() -> callback.ready(ready, message));
        });
    }

    interface PublishedFapCallback {
        void ready(byte[] data, String publishedApi, boolean verifiedCompatible, String error);
    }

    /**
     * Install workflow: use the firmware's real reported API if available. If the archive
     * has no matching build, allow a real published binary rather than inventing a
     * different firmware API. Incompatible builds may still be rejected on launch.
     */
    void downloadAvailableOfficialFap(Entry entry, String deviceApi, PublishedFapCallback callback) {
        downloadAvailableOfficialFap(entry, deviceApi, null, callback);
    }

    void downloadAvailableOfficialFap(Entry entry, String deviceApi,
            DownloadProgress progress, PublishedFapCallback callback) {
        downloadAvailableOfficialFap(entry, deviceApi, progress, null, callback);
    }

    void downloadAvailableOfficialFap(Entry entry, String deviceApi,
            DownloadProgress progress, DownloadTicket ticket, PublishedFapCallback callback) {
        if (entry == null || entry.id == null || !entry.id.matches("[A-Za-z0-9_-]{2,100}")) {
            main.post(() -> callback.ready(null, null, false, "Unknown catalog application"));
            return;
        }
        worker.execute(() -> {
            byte[] result = null;
            String selectedApi = null;
            String failure = "No published .fap was found for this application";
            try {
                List<String> published = FapBuildSelector.ordered(officialLinksFor(entry), deviceApi);
                if (published.isEmpty()) {
                    failure = "No downloadable .fap build appears in the official archive";
                } else {
                    for (String path : published) {
                        if (ticket != null && ticket.cancelled) break;
                        HttpURLConnection connection = null;
                        try {
                            connection = connect(HOST + path);
                            if (ticket != null) {
                                ticket.connection = connection;
                                if (ticket.cancelled) break;
                            }
                            String contentType = connection.getContentType();
                            if (contentType != null && (contentType.contains("text/html") ||
                                    contentType.contains("application/json"))) continue;
                            byte[] bytes;
                            try (InputStream input = connection.getInputStream()) {
                                bytes = readLimitedWithProgress(input, 2_000_000,
                                        connection.getContentLengthLong(), progress);
                            }
                            if (ticket != null && ticket.cancelled) break;
                            if (bytes.length > 32 && (bytes[0] & 255) == 0x7f &&
                                    bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F') {
                                result = bytes;
                                selectedApi = FapBuildSelector.apiOf(path);
                                break;
                            }
                            failure = "The archive response was not a valid .fap";
                        } catch (Exception problem) {
                            failure = "Could not fetch published .fap: " + problem.getClass().getSimpleName();
                        } finally {
                            if (connection != null) connection.disconnect();
                            if (ticket != null) ticket.connection = null;
                        }
                    }
                }
            } catch (Exception problem) {
                failure = "Could not read the official app archive";
            }
            if (ticket != null && ticket.cancelled) {
                result = null;
                failure = "Cancelled";
            }
            byte[] downloaded = result;
            String buildApi = selectedApi;
            String error = downloaded == null ? failure : null;
            boolean compatible = FapBuildSelector.compatible(buildApi, deviceApi);
            main.post(() -> callback.ready(downloaded, buildApi, compatible, error));
        });
    }

    /** Import user-selected .fap binaries from HTTPS links, independent of Flipper Lab. */
    void downloadExternalFap(String url, FapCallback callback) {
        worker.execute(() -> {
            byte[] result = null;
            String error = null;
            HttpURLConnection conn = null;
            try {
                URL parsed = new URL(url == null ? "" : url.trim());
                if (!"https".equalsIgnoreCase(parsed.getProtocol()) || parsed.getHost().isEmpty())
                    throw new IOException("Use a direct HTTPS link to a .fap (or import a local file)");
                conn = (HttpURLConnection) parsed.openConnection();
                conn.setConnectTimeout(12000);
                conn.setReadTimeout(20000);
                conn.setRequestProperty("User-Agent", "Weave/1.0");
                if (conn.getResponseCode() != 200)
                    throw new IOException("External .fap returned HTTP " + conn.getResponseCode());
                if (!"https".equalsIgnoreCase(conn.getURL().getProtocol()))
                    throw new IOException("Insecure download redirect rejected");
                if (conn.getContentLengthLong() > 2_000_000)
                    throw new IOException(".fap exceeds the 2 MiB upload limit");
                try (InputStream in = conn.getInputStream()) { result = readLimited(in, 2_000_000); }
                if (result.length < 32 || (result[0] & 255) != 0x7f ||
                        result[1] != 'E' || result[2] != 'L' || result[3] != 'F')
                    throw new IOException("This URL did not return an ELF .fap binary");
            } catch (Exception failure) {
                result = null;
                error = failure.getMessage() == null ? "External download failed" : failure.getMessage();
            } finally {
                if (conn != null) conn.disconnect();
            }
            byte[] completed = result;
            String message = error;
            main.post(() -> callback.ready(completed, message));
        });
    }

    /** Resolve aliases to the archive's 24-character application ID. Never guess a build ID. */
    private String resolveOfficialApplicationId(String alias) {
        try {
            JSONObject data = new JSONObject(downloadText(HOST + "/api/v0/application/" + alias, 500_000));
            String id = first(data.optString("_id"), data.optString("id"),
                    data.optString("application_id"));
            if (id != null && id.matches("[a-fA-F0-9]{24}")) return id;
        } catch (Exception ignored) { }
        // The official list carries both the app alias and its archive ID.
        for (int offset = 0; offset < 2500; offset += 100) {
            try {
                String text = downloadText(HOST + "/api/v0/application?limit=100&offset=" + offset, 5_000_000);
                JSONArray items;
                if (text.trim().startsWith("[")) items = new JSONArray(text);
                else {
                    JSONObject page = new JSONObject(text);
                    items = page.optJSONArray("applications");
                    if (items == null) items = page.optJSONArray("items");
                    if (items == null) items = page.optJSONArray("data");
                    if (items == null) items = page.optJSONArray("results");
                }
                if (items == null) break;
                for (int i = 0; i < items.length(); i++) {
                    JSONObject app = items.optJSONObject(i);
                    if (app == null) continue;
                    String listed = first(app.optString("alias"), app.optString("slug"),
                            app.optString("appid"));
                    if (!alias.equalsIgnoreCase(listed)) continue;
                    String id = first(app.optString("_id"), app.optString("id"));
                    if (id != null && id.matches("[a-fA-F0-9]{24}")) return id;
                }
                if (items.length() < 100) break;
            } catch (Exception ignored) { break; }
        }
        return null;
    }

    /** The FAP API major must equal the device's API major; choose the newest minor <= device. */
    private List<String> compatibleBuilds(List<String> builds, String firmwareApi) {
        List<String> result = new ArrayList<>();
        String[] target = firmwareApi.split("\\.");
        int major = Integer.parseInt(target[0]), minor = Integer.parseInt(target[1]);
        java.util.TreeMap<Integer, List<String>> byMinor = new java.util.TreeMap<>(Collections.reverseOrder());
        for (String path : builds) {
            Matcher m = OFFICIAL_F7_BUILD.matcher(path);
            if (!m.matches()) continue;
            Matcher sdk = Pattern.compile("/build/f7/(\\d+)\\.(\\d+)$").matcher(path);
            if (!sdk.find()) continue;
            int buildMajor = Integer.parseInt(sdk.group(1));
            int buildMinor = Integer.parseInt(sdk.group(2));
            if (buildMajor == major && buildMinor <= minor)
                byMinor.computeIfAbsent(buildMinor, k -> new ArrayList<>()).add(path);
        }
        for (List<String> paths : byMinor.values()) result.addAll(paths);
        return result;
    }

    void loadReleaseSize(Entry entry, DetailCallback callback) {
        if (entry == null || entry.id == null || !entry.id.matches("[A-Za-z0-9_-]{2,100}")) {
            main.post(() -> callback.ready(null));
            return;
        }
        String cached = resolvedFapSizes.get(entry.id);
        if (cached != null) {
            main.post(() -> callback.ready(cached));
            return;
        }
        // Re-rendering the same detail page must not silently lose the callback.
        sizeCallbacks.computeIfAbsent(entry.id, key -> new CopyOnWriteArrayList<>()).add(callback);
        if (!sizeLookupsStarted.add(entry.id)) return;
        worker.execute(() -> {
            long bytes = -1;
            // Flipper Lab uses Archivarius-built binaries, not the developer's
            // repository releases. The official application page exposes the exact
            // /api/v0/1/application/version/{id}/build/f7/{sdk} links.
            try {
                bytes = sizeFromOfficialPage(entry);
            } catch (Exception ignored) { }
            if (bytes <= 0) {
                // Some catalog versions are accessible by alias even when the
                // application listing lacks the Archivarius UUID.
                String version = entry.version == null ? "" : entry.version.trim();
                if (version.matches("[0-9A-Za-z._-]{1,40}")) {
                    try {
                        String url = HOST + "/api/v0/application/" + entry.id + "/versions?version=" +
                                URLEncoder.encode(version, "UTF-8");
                        bytes = sizeFromVersionListing(downloadText(url, 2_000_000));
                    } catch (Exception ignored) { }
                }
            }
            if (bytes <= 0) {
                // Explicitly published FAP size fields are acceptable, but a
                // general application ZIP size is not a binary's size.
                try {
                    bytes = fapBytesInResponse(downloadText(HOST + "/api/v0/application/" + entry.id,
                            2_000_000));
                } catch (Exception ignored) { }
            }
            // Do not substitute files from developer GitHub releases. Their
            // artifacts may not match what Flipper Lab actually distributes.
            String result = bytes > 0 ? humanBytes(bytes) : null;
            if (bytes > 0) {
                archivedFapSizes.put(entry.id, bytes);
                resolvedFapSizes.put(entry.id, result);
            }
            // No negative-cache lockout: reopening the page can retry a failed network call.
            sizeLookupsStarted.remove(entry.id);
            List<DetailCallback> waiting = sizeCallbacks.remove(entry.id);
            if (waiting != null) main.post(() -> {
                for (DetailCallback listener : waiting) listener.ready(result);
            });
        });
    }

    // Archivarius serves already-built Flipper packages. Example verified from
    // its public application page:
    // /api/v0/1/application/version/68b03a50721e74ef7e62c036/build/f7/87.1
    private static final Pattern OFFICIAL_F7_BUILD = Pattern.compile(
            "(?:https?://catalog\\.flipperzero\\.one)?(/api/v0/1/application/version/[a-fA-F0-9]{24}/build/f7/[0-9]+\\.[0-9]+)");
    private static final Pattern VERSION_ROW = Pattern.compile(
            "(?is)<tr\\b[^>]*>(.*?)</tr>");
    private static final Pattern CELL = Pattern.compile("(?is)<td\\b[^>]*>(.*?)</td>");

    private long sizeFromOfficialPage(Entry entry) throws IOException {
        // Public alias and Archivarius application UUID are different identifiers.
        List<String> candidates = new ArrayList<>();
        if (entry.archiveId != null) candidates.add(entry.archiveId);
        if (entry.id.matches("[a-fA-F0-9]{24}") && !candidates.contains(entry.id))
            candidates.add(entry.id);

        // GitHub-manifest fallback entries don't know the application UUID.
        // Resolve it from the official catalog before reading the build table.
        if (candidates.isEmpty()) {
            try {
                String response = downloadText(HOST + "/api/v0/application/" + entry.id, 500_000);
                JSONObject data = new JSONObject(response);
                String uuid = first(data.optString("_id"), data.optString("id"),
                        data.optString("application_id"));
                if (uuid != null && uuid.matches("[a-fA-F0-9]{24}")) candidates.add(uuid);
            } catch (Exception ignored) { }
        }
        if (!candidates.contains(entry.id)) candidates.add(entry.id);
        for (String id : candidates) {
            if (!id.matches("[a-fA-F0-9]{24}|[A-Za-z0-9_-]{2,100}")) continue;
            String page;
            try { page = downloadText(HOST + "/application/" + id + "/page", 2_000_000); }
            catch (Exception e) { continue; }
            List<String> builds = officialBuildLinks(page, entry.version);
            for (int i = 0; i < builds.size() && i < 8; i++) {
                long bytes = remoteFapLength(HOST + builds.get(i));
                if (bytes > 0) return bytes;
            }
        }
        return -1;
    }

    /** Keep links to real downloadable binaries; prefer the displayed app version. */
    private List<String> officialBuildLinks(String html, String version) {
        List<String> matching = new ArrayList<>();
        List<String> other = new ArrayList<>();
        Matcher rows = VERSION_ROW.matcher(html);
        while (rows.find() && matching.size() + other.size() < 40) {
            String row = rows.group(1);
            Matcher cells = CELL.matcher(row);
            String appVersion = null;
            if (cells.find()) appVersion = cells.group(1).replaceAll("<[^>]+>", "").trim();
            Matcher links = OFFICIAL_F7_BUILD.matcher(row.replace("&amp;", "&"));
            while (links.find()) {
                String path = links.group(1);
                if ((version == null || version.trim().isEmpty() || version.trim().equals(appVersion))) {
                    if (!matching.contains(path)) matching.add(path);
                } else if (!other.contains(path)) other.add(path);
            }
        }
        // Handle pages where the HTML is not table-based.
        if (matching.isEmpty() && other.isEmpty()) {
            Matcher links = OFFICIAL_F7_BUILD.matcher(html.replace("&amp;", "&"));
            while (links.find() && matching.size() < 24) {
                String path = links.group(1);
                if (!matching.contains(path)) matching.add(path);
            }
        }
        // The official page lists the newest SDK builds first. Do not use an
        // older app version if a version filter matched something explicitly.
        if (!matching.isEmpty()) return matching;
        return other;
    }

    private long sizeFromVersionListing(String text) throws Exception {
        Object versions = text.trim().startsWith("[") ? new JSONArray(text) : new JSONObject(text);
        long direct = extractFapSize(versions, 0, false);
        if (direct > 0) return direct;
        for (String link : fapUrls(versions, 0)) {
            long bytes = remoteFapLength(link);
            if (bytes > 0) return bytes;
        }
        // A version ID without a build's SDK/API cannot establish an artifact.
        // No fabricated SDK versions or guessed binary URLs here.
        return -1;
    }

    private long fapBytesInResponse(String response) throws Exception {
        Object data = response.trim().startsWith("[") ? new JSONArray(response) : new JSONObject(response);
        long size = extractFapSize(data, 0, false);
        if (size > 0) return size;
        // The official version/build record can carry the download URL rather than
        // the size. Request only the headers of a real .fap, never download it here.
        for (String url : fapUrls(data, 0)) {
            size = remoteFapLength(url);
            if (size > 0) return size;
        }
        return -1;
    }

    private List<String> fapUrls(Object node, int depth) {
        List<String> urls = new ArrayList<>();
        collectFapUrls(node, depth, urls);
        return urls;
    }

    private void collectFapUrls(Object node, int depth, List<String> urls) {
        if (depth > 7 || urls.size() >= 12 || node == null || node == JSONObject.NULL) return;
        if (node instanceof JSONArray) {
            JSONArray array = (JSONArray) node;
            for (int i = 0; i < array.length() && i < 60; i++)
                collectFapUrls(array.opt(i), depth + 1, urls);
            return;
        }
        if (!(node instanceof JSONObject)) return;
        JSONObject object = (JSONObject) node;
        String name = first(object.optString("filename"), object.optString("file_name"),
                object.optString("name"), object.optString("path"));
        for (String field : new String[]{"fap_url", "fapUrl", "download_url", "downloadUrl", "url", "href"}) {
            String value = object.optString(field, "");
            boolean isFap = value.matches("(?i)^https://[^?#\\s]+\\.fap(?:[?#].*)?$") ||
                    (name != null && name.toLowerCase(Locale.ROOT).endsWith(".fap"));
            if (isFap && isTrustedArtifactUrl(value) && !urls.contains(value)) urls.add(value);
        }
        for (String field : new String[]{"data", "results", "application", "applications", "versions",
                "version", "builds", "build", "artifacts", "files", "assets", "downloads", "fap", "binary"}) {
            collectFapUrls(object.opt(field), depth + 1, urls);
        }
    }

    private boolean isTrustedArtifactUrl(String raw) {
        if (raw == null || !raw.startsWith("https://")) return false;
        try {
            URL url = new URL(raw);
            String host = url.getHost().toLowerCase(Locale.ROOT);
            return host.equals("catalog.flipperzero.one") || host.equals("cdn.flipperzero.one") ||
                    host.equals("update.flipperzero.one") || host.equals("api.flipperzero.one");
        } catch (Exception ignored) { return false; }
    }

    private long remoteFapLength(String url) {
        if (!isTrustedArtifactUrl(url)) return -1;
        // HEAD often works for binaries; some object-storage backends only
        // allow GET. The Range request does not download the whole FAP.
        for (String method : new String[]{"HEAD", "GET"}) {
            HttpURLConnection conn = null;
            try {
                conn = (HttpURLConnection) new URL(url).openConnection();
                conn.setRequestMethod(method);
                conn.setInstanceFollowRedirects(true);
                conn.setConnectTimeout(6500);
                conn.setReadTimeout(8500);
                conn.setRequestProperty("User-Agent", "Weave/1.0 (Flipper catalog)");
                if (method.equals("GET")) conn.setRequestProperty("Range", "bytes=0-0");
                int code = conn.getResponseCode();
                if (code != 200 && code != 206) continue;
                String type = conn.getContentType();
                if (type != null && (type.toLowerCase(Locale.ROOT).contains("text/html") ||
                        type.toLowerCase(Locale.ROOT).contains("application/json"))) continue;
                long bytes = -1;
                if (code == 206) {
                    String range = conn.getHeaderField("Content-Range");
                    if (range != null) {
                        Matcher total = Pattern.compile("(?i)^bytes\\s+\\d+-\\d+/(\\d+)$").matcher(range.trim());
                        if (total.matches()) bytes = Long.parseLong(total.group(1));
                    }
                } else {
                    bytes = conn.getContentLengthLong();
                }
                if (bytes > 0 && bytes <= 100_000_000) return bytes;
            } catch (Exception ignored) {
                // Fall through to the second method.
            } finally {
                if (conn != null) conn.disconnect();
            }
        }
        return -1;
    }

    /** Extract actual binary sizes, not ZIPs, screenshots, or repository archive sizes. */
    private long extractFapSize(Object node, int depth, boolean buildContext) {
        if (depth > 7 || node == null || node == JSONObject.NULL) return -1;
        if (node instanceof JSONArray) {
            JSONArray values = (JSONArray) node;
            for (int i = 0; i < values.length() && i < 60; i++) {
                long result = extractFapSize(values.opt(i), depth + 1, buildContext);
                if (result > 0) return result;
            }
            return -1;
        }
        if (!(node instanceof JSONObject)) return -1;
        JSONObject value = (JSONObject) node;
        for (String key : new String[]{"fap_size", "fapSize", "fap_size_bytes", "fapSizeBytes",
                "fap_file_size", "binary_size", "binarySize", "build_size", "file_size", "fileSize",
                "size_bytes", "sizeBytes", "download_size", "downloadSize", "package_size", "packageSize"}) {
            long bytes = positiveSize(value.opt(key));
            if (bytes > 0) return bytes;
        }
        // A generic size field is trustworthy only when tied to an actual .fap artifact.
        String assetName = first(value.optString("filename"), value.optString("name"),
                value.optString("path"), value.optString("file_name"));
        boolean fapArtifact = assetName != null && assetName.toLowerCase(Locale.ROOT).endsWith(".fap");
        if (fapArtifact || (buildContext && value.has("size") &&
                (value.has("target") || value.has("api") || value.has("firmware_api")))) {
            long bytes = positiveSize(value.opt("size"));
            if (bytes > 0) return bytes;
        }
        for (String child : new String[]{"latest_build", "latestBuild", "build", "builds",
                "artifacts", "artifact", "binary", "binaries", "releases", "release",
                "versions", "version", "assets", "files", "fap", "package",
                "application", "applications", "data", "result", "latest_release", "latest", "downloads"}) {
            Object nested = value.opt(child);
            if (nested == null || nested == JSONObject.NULL) continue;
            long bytes = extractFapSize(nested, depth + 1, true);
            if (bytes > 0) return bytes;
        }
        return -1;
    }

    private long positiveSize(Object value) {
        if (value instanceof Number) {
            long bytes = ((Number) value).longValue();
            return bytes > 0 && bytes <= 100_000_000 ? bytes : -1;
        }
        if (value instanceof String) {
            String str = ((String) value).trim();
            if (str.matches("[0-9]{1,9}")) {
                try {
                    long bytes = Long.parseLong(str);
                    return bytes > 0 && bytes <= 100_000_000 ? bytes : -1;
                } catch (NumberFormatException ignored) { }
            }
            java.util.regex.Matcher readable = java.util.regex.Pattern.compile(
                    "(?i)^([0-9]+(?:\\.[0-9]+)?)\\s*(B|KiB|KB|MiB|MB)$").matcher(str);
            if (readable.matches()) {
                double amount = Double.parseDouble(readable.group(1));
                String unit = readable.group(2).toLowerCase(Locale.ROOT);
                double multiplier = unit.equals("kib") ? 1024.0 : unit.equals("kb") ? 1000.0 :
                        unit.equals("mib") ? 1024.0 * 1024.0 : unit.equals("mb") ? 1000.0 * 1000.0 : 1.0;
                long bytes = (long) (amount * multiplier);
                return bytes > 0 && bytes <= 100_000_000 ? bytes : -1;
            }
        }
        return -1;
    }

    private long fetchMatchingGithubFapSize(Entry entry) throws Exception {
        if (entry.sourceUrl == null || !entry.sourceUrl.matches(
                "https://github\\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+")) return -1;
        String repo = entry.sourceUrl.substring("https://github.com/".length());
        JSONObject release = new JSONObject(downloadText(
                "https://api.github.com/repos/" + repo + "/releases/latest", 1_000_000));
        JSONArray assets = release.optJSONArray("assets");
        if (assets == null) return -1;
        String requestedId = entry.id.toLowerCase(Locale.ROOT).replaceAll("[^a-z0-9]", "");
        String repoName = repo.substring(repo.indexOf('/') + 1).toLowerCase(Locale.ROOT).replaceAll("[^a-z0-9]", "");
        long single = -1;
        int count = 0;
        for (int i = 0; i < assets.length(); i++) {
            JSONObject asset = assets.optJSONObject(i);
            if (asset == null) continue;
            String filename = asset.optString("name", "").toLowerCase(Locale.ROOT);
            if (!filename.endsWith(".fap")) continue;
            long bytes = positiveSize(asset.opt("size"));
            if (bytes <= 0) continue;
            count++;
            single = bytes;
            // A monorepo can have many unrelated FAPs; never use a random first match.
            String normalized = filename.substring(0, filename.length() - 4).replaceAll("[^a-z0-9]", "");
            if (normalized.equals(requestedId) || normalized.startsWith(requestedId + "f7")
                    || normalized.startsWith(requestedId + "v")) return bytes;
        }
        // A single .fap asset is safe if the repository itself is named after the app.
        return count == 1 && (repoName.equals(requestedId) || repoName.startsWith(requestedId)) ? single : -1;
    }

    private String humanBytes(long bytes) {
        if (bytes < 1024) return bytes + " B";
        double kib = bytes / 1024.0;
        if (kib < 1024) return String.format(Locale.US, "%.1f KiB", kib);
        return String.format(Locale.US, "%.2f MiB", kib / 1024.0);
    }

    Bitmap peekAppIcon(Entry entry) {
        if (entry == null || entry.id == null) return null;
        Bitmap ready = appIcons.get(entry.id.toLowerCase(Locale.ROOT));
        return ready != null ? ready : (entry.iconUrl == null ? null : images.get(entry.iconUrl));
    }

    void loadIcon(String url, IconCallback callback) { loadImage(url, true, callback); }
    void loadScreenshot(String url, IconCallback callback) { loadImage(url, false, callback); }

    /** Use the app identity, not just an optional icon URL, when loading its icon. */
    void loadIcon(Entry entry, IconCallback callback) {
        if (entry == null || entry.id == null || !entry.id.matches("[A-Za-z0-9_-]{2,100}")) {
            main.post(() -> callback.ready(null)); return;
        }
        final String key = entry.id.toLowerCase(Locale.ROOT);
        Bitmap cached = appIcons.get(key);
        if (cached != null) { main.post(() -> callback.ready(cached)); return; }
        Long failedAt = iconLastFailedAt.get(key);
        if (failedAt == null && resolvedIconCache != null) {
            long persistedFailure = resolvedIconCache.getLong("fail:" + key, 0);
            if (persistedFailure > 0 && System.currentTimeMillis() - persistedFailure < 6 * 3600000L)
                failedAt = persistedFailure;
        }
        if (failedAt != null && System.currentTimeMillis() - failedAt < 6 * 3600000L) {
            main.post(() -> callback.ready(null)); return;
        }
        synchronized (iconWaiters) {
            List<IconCallback> pending = iconWaiters.get(key);
            if (pending != null) { pending.add(callback); return; }
            pending = new ArrayList<>();
            pending.add(callback);
            iconWaiters.put(key, pending);
        }
        worker.execute(() -> {
            Bitmap bitmap = null;
            String resolvedUrl = null;
            try {
                String knownUrl = resolvedIconCache == null ? null :
                        resolvedIconCache.getString("url:" + key, null);
                if (knownUrl != null && imageUrl(knownUrl) != null) {
                    bitmap = fetchBitmap(knownUrl, true);
                    if (bitmap != null) resolvedUrl = knownUrl;
                }
                if (bitmap == null) {
                    bitmap = fetchBitmap(entry.iconUrl, true);
                    if (bitmap != null) resolvedUrl = entry.iconUrl;
                }
                if (bitmap == null) {
                    String fallback = resolveCatalogIcon(entry);
                    if (fallback != null) {
                        bitmap = fetchBitmap(fallback, true);
                        if (bitmap != null) resolvedUrl = fallback;
                        if (bitmap == null && fallback.startsWith("https://raw.githubusercontent.com/")) {
                            int slash = fallback.lastIndexOf('/');
                            if (slash > 0) {
                                String directory = fallback.substring(0, slash + 1);
                                String name = fallback.substring(slash + 1);
                                if (name.matches("[A-Za-z0-9_.-]+\\.png")) {
                                    for (String folder : new String[]{"assets/", "images/", "icons/"}) {
                                        String url = directory + folder + name;
                                        bitmap = fetchBitmap(url, true);
                                        if (bitmap != null) { resolvedUrl = url; break; }
                                    }
                                }
                            }
                        }
                    }
                }
            } catch (Exception ignored) { }
            if (bitmap != null) {
                appIcons.put(key, bitmap);
                iconLastFailedAt.remove(key);
                if (resolvedIconCache != null && resolvedUrl != null)
                    resolvedIconCache.edit().putString("url:" + key, resolvedUrl)
                            .remove("fail:" + key).apply();
            } else {
                long failedAtTime = System.currentTimeMillis();
                iconLastFailedAt.put(key, failedAtTime);
                if (resolvedIconCache != null)
                    resolvedIconCache.edit().putLong("fail:" + key, failedAtTime).apply();
            }
            final Bitmap loaded = bitmap;
            final List<IconCallback> waiting;
            synchronized (iconWaiters) { waiting = iconWaiters.remove(key); }
            if (waiting != null)
                main.post(() -> { for (IconCallback cb : waiting) cb.ready(loaded); });
        });
    }

    private String resolveCatalogIcon(Entry entry) {
        Entry resolved = resolveCatalogManifest(entry);
        return resolved == null ? null : imageUrl(resolved.iconUrl);
    }

    /** Resolve official manifest metadata without assuming a docs/catalog.md file exists. */
    private Entry resolveCatalogManifest(Entry entry) {
        if (entry == null || entry.id == null || !entry.id.matches("[A-Za-z0-9_-]{2,100}"))
            return null;
        String key = entry.id.toLowerCase(Locale.ROOT);
        Entry cached = resolvedManifests.get(key);
        if (cached != null) return cached;
        List<String> paths = new ArrayList<>();
        final String prefix = "https://github.com/flipperdevices/flipper-application-catalog/blob/main/";
        if (entry.manifestUrl != null && entry.manifestUrl.startsWith(prefix)) {
            String path = entry.manifestUrl.substring(prefix.length());
            if (path.matches("applications/[^/]+/[A-Za-z0-9_-]+/manifest\\.yml")) paths.add(path);
        }
        String category = entry.category;
        if (category != null && category.matches("[A-Za-z0-9-]{2,30}")) {
            String categoryPath = "applications/" + category + "/" + entry.id + "/manifest.yml";
            if (!paths.contains(categoryPath)) paths.add(categoryPath);
        }
        for (String path : paths) {
            try {
                Entry resolved = manifestEntry(path, downloadText(RAW + path, 200_000));
                if (resolved != null) {
                    resolvedManifests.put(key, resolved);
                    return resolved;
                }
            } catch (Exception ignored) { }
        }
        // Only query the public tree when neither the published manifest URL
        // nor the API category identifies an existing manifest.
        try {
            String path = indexedCatalogManifest(entry.id);
            if (path != null && !paths.contains(path)) {
                Entry resolved = manifestEntry(path, downloadText(RAW + path, 200_000));
                if (resolved != null) {
                    resolvedManifests.put(key, resolved);
                    return resolved;
                }
            }
        } catch (Exception ignored) { }
        return null;
    }

    private String indexedCatalogManifest(String alias) throws IOException {
        synchronized (manifestIndexLock) {
            if (catalogManifestIndex == null) {
                JSONObject result;
                try {
                    result = new JSONObject(downloadText(TREE, 10_000_000));
                } catch (org.json.JSONException e) {
                    throw new IOException("Invalid catalog tree JSON", e);
                }
                if (result.optBoolean("truncated")) throw new IOException("Catalog tree incomplete");
                JSONArray tree = result.optJSONArray("tree");
                if (tree == null) throw new IOException("Catalog tree has no tree array");
                Map<String, String> found = new HashMap<>();
                for (int i = 0; i < tree.length(); i++) {
                    JSONObject record = tree.optJSONObject(i);
                    if (record == null) continue;
                    String path = record.optString("path", "");
                    if (!path.matches("applications/[^/]+/[A-Za-z0-9_-]+/manifest\\.yml")) continue;
                    String[] segments = path.split("/");
                    found.putIfAbsent(segments[2].toLowerCase(Locale.ROOT), path);
                }
                catalogManifestIndex = found;
            }
            return catalogManifestIndex.get(alias.toLowerCase(Locale.ROOT));
        }
    }

    private File iconFile(String url) {
        if (iconCacheDir == null || url == null) return null;
        try {
            MessageDigest digest = MessageDigest.getInstance("SHA-256");
            byte[] hash = digest.digest(url.getBytes(java.nio.charset.StandardCharsets.UTF_8));
            StringBuilder filename = new StringBuilder(64);
            for (byte value : hash) filename.append(String.format(Locale.ROOT, "%02x", value & 0xff));
            return new File(iconCacheDir, filename + ".img");
        } catch (Exception ignored) { return null; }
    }

    private void rememberIcon(File file, byte[] data) {
        if (file == null || data == null || data.length > 2_000_000) return;
        try {
            if (!iconCacheDir.isDirectory() && !iconCacheDir.mkdirs()) return;
            File temp = new File(iconCacheDir, file.getName() + ".tmp");
            try (FileOutputStream out = new FileOutputStream(temp)) { out.write(data); }
            if (!temp.renameTo(file)) temp.delete();
            // Trim LRU limite a 1/min pour ne pas scanner le disque par image.
            long now = System.currentTimeMillis();
            if (now - lastImageCacheTrimAt < 60000) return;
            lastImageCacheTrimAt = now;
            File[] files = iconCacheDir.listFiles();
            if (files == null) return;
            long total = 0;
            for (File candidate : files) total += candidate.length();
            if (total <= ICON_CACHE_MAX_BYTES) return;
            java.util.Arrays.sort(files, (a, b) -> Long.compare(a.lastModified(), b.lastModified()));
            for (File candidate : files) {
                if (total <= ICON_CACHE_MAX_BYTES) break;
                long length = candidate.length();
                if (candidate.delete()) total -= length;
            }
        } catch (Exception ignored) { /* Network imagery works without disk cache. */ }
    }

    private void loadImage(String url, boolean icon, IconCallback callback) {
        if (imageUrl(url) == null) { main.post(() -> callback.ready(null)); return; }
        Bitmap cached = images.get(url);
        if (cached != null) { main.post(() -> callback.ready(cached)); return; }
        synchronized (imageWaiters) {
            List<IconCallback> pending = imageWaiters.get(url);
            if (pending != null) { pending.add(callback); return; }
            pending = new ArrayList<>();
            pending.add(callback);
            imageWaiters.put(url, pending);
        }
        worker.execute(() -> {
            Bitmap bitmap = fetchBitmap(url, icon);
            final List<IconCallback> pending;
            synchronized (imageWaiters) { pending = imageWaiters.remove(url); }
            if (pending != null) main.post(() -> {
                for (IconCallback listener : pending) listener.ready(bitmap);
            });
        });
    }

    private Bitmap fetchBitmap(String url, boolean icon) {
        if (imageUrl(url) == null) return null;
        Bitmap cached = images.get(url);
        if (cached != null) return cached;
        HttpURLConnection conn = null;
        try {
            byte[] bytes = null;
            File disk = iconFile(url);
            if (disk != null && disk.isFile() && disk.length() <= 2_000_000) {
                try (FileInputStream stream = new FileInputStream(disk)) {
                    bytes = readLimited(stream, 2_000_000);
                    disk.setLastModified(System.currentTimeMillis());
                } catch (IOException ignored) { bytes = null; }
            }
            if (bytes == null) {
                conn = connect(url);
                try (InputStream stream = conn.getInputStream()) {
                    bytes = readLimited(stream, 2_000_000);
                }
            }
            BitmapFactory.Options options = new BitmapFactory.Options();
            options.inJustDecodeBounds = true;
            BitmapFactory.decodeByteArray(bytes, 0, bytes.length, options);
            if (options.outWidth < 1 || options.outHeight < 1 ||
                    options.outWidth > 2048 || options.outHeight > 2048) return null;
            options.inJustDecodeBounds = false;
            options.inSampleSize = !icon && Math.max(options.outWidth, options.outHeight) > 900 ? 2 : 1;
            Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length, options);
            if (bitmap != null) {
                if (icon && bitmap.getWidth() <= 32 && bitmap.getHeight() <= 32)
                    bitmap = Bitmap.createScaledBitmap(bitmap, 96, 96, false);
                images.put(url, bitmap);
                if (conn != null) rememberIcon(disk, bytes);
            }
            return bitmap;
        } catch (Exception ignored) {
            return null;
        } finally {
            if (conn != null) conn.disconnect();
        }
    }

    private String downloadText(String url, int maxBytes) throws IOException {
        HttpURLConnection conn = connect(url);
        try (InputStream stream = conn.getInputStream()) {
            return new String(readLimited(stream, maxBytes), java.nio.charset.StandardCharsets.UTF_8);
        } finally { conn.disconnect(); }
    }

    /** Reports received bytes, never a guessed percentage. A negative total
     * means the server did not supply a usable Content-Length. */
    private byte[] readLimitedWithProgress(InputStream stream, int limit, long length,
            DownloadProgress progress) throws IOException {
        if(length > limit) throw new IOException("FAP exceeds size limit");
        ByteArrayOutputStream result = new ByteArrayOutputStream();
        byte[] buffer = new byte[8192];
        long lastReport = 0;
        int n;
        while((n = stream.read(buffer)) != -1) {
            if(result.size() + n > limit) throw new IOException("FAP exceeds size limit");
            result.write(buffer, 0, n);
            if(progress != null) {
                long now = android.os.SystemClock.elapsedRealtime();
                if(now - lastReport > 350 || (length > 0 && result.size() >= length)) {
                    lastReport = now;
                    long bytes = result.size();
                    long total = length > 0 && length <= limit ? length : -1;
                    main.post(() -> progress.changed(bytes, total));
                }
            }
        }
        return result.toByteArray();
    }

    private byte[] readLimited(InputStream stream, int limit) throws IOException {
        ByteArrayOutputStream output = new ByteArrayOutputStream();
        byte[] buf = new byte[8192];
        int n;
        while ((n = stream.read(buf)) != -1) {
            if (output.size() + n > limit) throw new IOException("Response too large");
            output.write(buf, 0, n);
        }
        return output.toByteArray();
    }

    private HttpURLConnection connect(String uri) throws IOException {
        HttpURLConnection conn = (HttpURLConnection) new URL(uri).openConnection();
        conn.setConnectTimeout(8000);
        conn.setReadTimeout(12000);
        conn.setRequestProperty("User-Agent", "Weave/1.0 (Flipper catalog)");
        if (conn.getResponseCode() != 200) {
            int code = conn.getResponseCode();
            conn.disconnect();
            throw new IOException("HTTP " + code);
        }
        return conn;
    }

    void close() { cancelSearch(); worker.shutdownNow(); }
}
