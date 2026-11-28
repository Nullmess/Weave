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
import java.io.IOException;
import java.security.MessageDigest;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.Locale;

/** GitHub release metadata and bounded, local downloads; no unverified release URLs. */
final class WeaveUpdates {
    private static final String REPO = "Nullmess/Weave";
    private static final int MAX_JSON = 1_000_000;
    private static final long MAX_ASSET = 250_000_000L;
    private static JSONObject cachedRelease;
    private static long cachedAt;
    static final class AssetPendingException extends IOException {
        final String version;
        AssetPendingException(String version, boolean firmware) {
            super("v" + version + " " + (firmware ? "firmware" : "Android") + " build is not published yet");
            this.version = version;
        }
    }

    static final class Release {
        final String version, name, url, digest;
        final long size;
        Release(String version, String name, String url, String digest, long size) {
            this.version = version; this.name = name; this.url = url; this.digest = digest; this.size = size;
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
            connection.setRequestProperty("User-Agent", "Weave/1.0.4");
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
                String remaining = connection.getHeaderField("X-RateLimit-Remaining");
                connection.disconnect();
                throw new IOException(status == 404 ? "No public GitHub release found" :
                        status == 429 || (status == 403 && "0".equals(remaining)) ?
                                "GitHub rate limit; retry later" : "GitHub HTTP " + status);
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
        return FirmwareUpdatePolicy.compare(current == null ? null : current.replaceFirst("^[vV]", ""),
                latest == null ? null : latest.replaceFirst("^[vV]", ""));
    }
    // Cache des releases stables.
    private static synchronized JSONObject latestMetadata() throws Exception {
        long now = android.os.SystemClock.elapsedRealtime();
        if (cachedRelease != null && now - cachedAt < 20_000L) return cachedRelease;
        HttpURLConnection conn = connect("https://api.github.com/repos/" + REPO + "/releases/latest");
        JSONObject metadata = new JSONObject(new String(read(conn, MAX_JSON, null), StandardCharsets.UTF_8));
        if (metadata.optBoolean("draft", false) || metadata.optBoolean("prerelease", false))
            throw new IOException("No stable Weave release found");
        cachedRelease = metadata;
        cachedAt = android.os.SystemClock.elapsedRealtime();
        return metadata;
    }
    static Release latest(boolean firmware) throws Exception {
        JSONObject release = latestMetadata();
        String version = release.optString("tag_name", "").replaceFirst("^[vV]", "");
        if (!FirmwareUpdatePolicy.validVersion(version)) throw new IOException("Release version unavailable");
        JSONArray assets = release.optJSONArray("assets");
        if (assets == null) throw new AssetPendingException(version, firmware);
        for (int i = 0; i < assets.length(); i++) {
            JSONObject asset = assets.optJSONObject(i);
            if (asset == null) continue;
            String name = asset.optString("name", "");
            String expected = "Weave-" + version + (firmware ? "-firmware.tgz" : "-android.apk");
            if (!expected.equals(name)) continue;
            String url = asset.optString("browser_download_url", "");
            String expectedUrl = "https://github.com/" + REPO + "/releases/download/"
                    + release.optString("tag_name", "") + "/" + expected;
            if (!expectedUrl.equals(url)) throw new IOException("Unexpected GitHub asset URL");
            long size = asset.optLong("size", 0);
            if (size <= 128 || size > MAX_ASSET) throw new IOException("Invalid published asset size");
            String digest = asset.optString("digest", "");
            if (!digest.isEmpty() && !digest.matches("sha256:[0-9a-fA-F]{64}"))
                throw new IOException("Unsupported release checksum");
            return new Release(version, name, url, digest, size);
        }
        throw new AssetPendingException(version, firmware);
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
            if (declared > MAX_ASSET || (declared > 0 && declared != release.size))
                throw new java.io.IOException("Unexpected update download size");
            long count = 0;
            MessageDigest checksum = MessageDigest.getInstance("SHA-256");
            try (InputStream source = conn.getInputStream(); FileOutputStream target = new FileOutputStream(temp)) {
                byte[] buffer = new byte[32768]; int read;
                long deadline = android.os.SystemClock.elapsedRealtime() + 600000L;
                while ((read = source.read(buffer)) != -1) {
                    if (Thread.currentThread().isInterrupted() || android.os.SystemClock.elapsedRealtime() > deadline)
                        throw new java.io.IOException("Update download interrupted or timed out");
                    if (read == 0) continue;
                    count += read;
                    if (count > MAX_ASSET) throw new java.io.IOException("Update exceeds size limit");
                    target.write(buffer, 0, read);
                    checksum.update(buffer, 0, read);
                    if (progress != null)
                        progress.onProgress((int)Math.min(99, count * 100 / release.size));
                }
                target.getFD().sync();
            }
            if (count < 128 || count != release.size) throw new IOException("Incomplete update download");
            if (!release.digest.isEmpty()) {
                StringBuilder hex = new StringBuilder(64);
                for (byte b : checksum.digest()) hex.append(String.format(Locale.ROOT, "%02x", b & 0xff));
                if (!release.digest.substring(7).equalsIgnoreCase(hex.toString()))
                    throw new IOException("Release SHA-256 mismatch");
            }
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
