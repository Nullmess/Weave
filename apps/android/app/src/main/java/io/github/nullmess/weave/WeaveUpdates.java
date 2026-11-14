package io.github.nullmess.weave;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

/** GitHub release metadata and bounded, local downloads; no unverified release URLs. */
final class WeaveUpdates {
    private static final String REPO = "Nullmess/Weave";
    private static final int MAX_JSON = 1_000_000;
    private static final long MAX_ASSET = 250_000_000L;

    static final class Release {
        final String version, name, url;
        Release(String version, String name, String url) {
            this.version = version; this.name = name; this.url = url;
        }
    }
    interface Progress { void onProgress(int percent); }

    private static boolean allowedHost(String host) {
        return host != null && (host.equals("api.github.com") || host.equals("github.com") ||
                host.equals("release-assets.githubusercontent.com") || host.equals("objects.githubusercontent.com") ||
                host.equals("avatars.githubusercontent.com") || host.equals("cdn.flipper.net"));
    }
    private static HttpURLConnection connect(String address) throws Exception {
        URL url = new URL(address);
        for (int hop = 0; hop < 6; hop++) {
            if (!"https".equalsIgnoreCase(url.getProtocol()) || !allowedHost(url.getHost()))
                throw new java.io.IOException("Untrusted download source");
            HttpURLConnection connection = (HttpURLConnection) url.openConnection();
            connection.setInstanceFollowRedirects(false);
            connection.setConnectTimeout(12000);
            connection.setReadTimeout(20000);
            connection.setRequestProperty("User-Agent", "Weave/1.0.0");
            connection.setRequestProperty("Accept", "application/vnd.github+json, */*");
            int status = connection.getResponseCode();
            if (status >= 300 && status < 400) {
                String redirect = connection.getHeaderField("Location");
                connection.disconnect();
                if (redirect == null) throw new java.io.IOException("Missing redirect");
                url = new URL(url, redirect);
                continue;
            }
            if (status != 200) {
                connection.disconnect();
                throw new java.io.IOException(status == 404 ? "No public GitHub release found" : "Download HTTP " + status);
            }
            return connection;
        }
        throw new java.io.IOException("Too many download redirects");
    }
    private static byte[] read(HttpURLConnection connection, long limit, Progress progress) throws Exception {
        long declared = connection.getContentLengthLong();
        if (declared > limit) throw new java.io.IOException("File too large");
        try (InputStream input = connection.getInputStream(); ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[16384]; long count = 0; int n;
            while ((n = input.read(buffer)) >= 0) {
                if (n == 0) continue;
                count += n;
                if (count > limit) throw new java.io.IOException("File too large");
                output.write(buffer, 0, n);
                if (progress != null && declared > 0) progress.onProgress((int)Math.min(99, count * 100 / declared));
            }
            return output.toByteArray();
        } finally { connection.disconnect(); }
    }
    static int compare(String current, String latest) {
        if (current == null || latest == null) return 0;
        String a = current.replaceFirst("^[vV]", ""), b = latest.replaceFirst("^[vV]", "");
        if (!a.matches("\\d+\\.\\d+\\.\\d+") || !b.matches("\\d+\\.\\d+\\.\\d+")) return 0;
        String[] aa = a.split("\\."), bb = b.split("\\.");
        for (int i=0;i<3;i++) {
            int cmp = Long.compare(Long.parseLong(aa[i]), Long.parseLong(bb[i]));
            if (cmp != 0) return cmp;
        }
        return 0;
    }
    static Release latest(boolean firmware) throws Exception {
        HttpURLConnection conn = connect("https://api.github.com/repos/" + REPO + "/releases/latest");
        JSONObject release = new JSONObject(new String(read(conn, MAX_JSON, null), StandardCharsets.UTF_8));
        String version = release.optString("tag_name", "").replaceFirst("^[vV]", "");
        if (!version.matches("\\d+\\.\\d+\\.\\d+")) throw new java.io.IOException("Release version unavailable");
        JSONArray assets = release.optJSONArray("assets");
        if (assets == null) throw new java.io.IOException("No published download");
        for (int i=0;i<assets.length();i++) {
            JSONObject asset = assets.optJSONObject(i);
            if (asset == null) continue;
            String name = asset.optString("name", "");
            String expected = "Weave-" + version + (firmware ? "-firmware.tgz" : "-android.apk");
            if (!expected.equals(name)) continue;
            String url = asset.optString("browser_download_url", "");
            String expectedUrl = "https://github.com/" + REPO + "/releases/download/"
                    + release.optString("tag_name", "") + "/" + expected;
            if (!expectedUrl.equals(url)) continue;
            return new Release(version, name, url);
        }
        throw new java.io.IOException("No compatible published " + (firmware ? "firmware" : "Android") + " asset");
    }
    static File download(Context context, Release release, boolean firmware, Progress progress) throws Exception {
        if (release == null) throw new java.io.IOException("No verified release");
        File directory = new File(context.getCacheDir(), "weave-updates");
        if (!directory.exists() && !directory.mkdirs()) throw new java.io.IOException("Cannot create update directory");
        File output = new File(directory, firmware ? "weave-firmware.tgz" : "weave-app.apk");
        File temp = new File(directory, output.getName() + ".part");
        HttpURLConnection conn = connect(release.url);
        try {
            long declared = conn.getContentLengthLong();
            if (declared > MAX_ASSET) throw new java.io.IOException("Update exceeds size limit");
            long count = 0;
            try (InputStream source = conn.getInputStream(); FileOutputStream target = new FileOutputStream(temp)) {
                byte[] buffer = new byte[32768]; int read;
                while ((read = source.read(buffer)) != -1) {
                    if (read == 0) continue;
                    count += read;
                    if (count > MAX_ASSET) throw new java.io.IOException("Update exceeds size limit");
                    target.write(buffer, 0, read);
                    if (progress != null && declared > 0)
                        progress.onProgress((int)Math.min(99, count * 100 / declared));
                }
                target.getFD().sync();
            }
            if (count < 128) throw new java.io.IOException("Empty update file");
            try (java.io.FileInputStream stream = new java.io.FileInputStream(temp)) {
                int first = stream.read(), second = stream.read();
                boolean valid = firmware ? first == 0x1f && second == 0x8b : first == 'P' && second == 'K';
                if (!valid) throw new java.io.IOException("Unexpected update format");
            }
            if (output.exists() && !output.delete()) throw new java.io.IOException("Cannot replace old download");
            if (!temp.renameTo(output)) throw new java.io.IOException("Cannot finish update download");
            if (progress != null) progress.onProgress(100);
            return output;
        } catch (Exception e) { temp.delete(); throw e; }
        finally { conn.disconnect(); }
    }
    static void loadCachedImage(Context context, android.os.Handler ui, String key, String url,
                                java.util.function.Consumer<Bitmap> callback) {
        File cache = new File(context.getFilesDir(), "weave-images/" + key + ".img");
        Bitmap existing = BitmapFactory.decodeFile(cache.getAbsolutePath());
        if (existing != null) {callback.accept(existing); return;}
        new Thread(() -> {
            try {
                HttpURLConnection c = connect(url);
                byte[] bytes = read(c, 3_000_000, null);
                Bitmap bitmap = BitmapFactory.decodeByteArray(bytes, 0, bytes.length);
                if (bitmap == null) return;
                if (!cache.getParentFile().exists()) cache.getParentFile().mkdirs();
                File temp = new File(cache.getParentFile(), key + ".tmp");
                try (FileOutputStream stream = new FileOutputStream(temp)) {stream.write(bytes);}
                if (!temp.renameTo(cache)) temp.delete();
                ui.post(() -> callback.accept(bitmap));
            } catch (Exception ignored) { /* Keep the drawable fallback when offline. */ }
        }, "Weave-image-" + key).start();
    }
    private WeaveUpdates() { }
}
