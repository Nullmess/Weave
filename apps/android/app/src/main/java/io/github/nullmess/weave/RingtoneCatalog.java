package io.github.nullmess.weave;

import org.json.JSONArray;
import org.json.JSONObject;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/** Allowlisted GitHub music libraries, no arbitrary remote URLs supplied by a Flipper. */
final class RingtoneCatalog {
    private static final String[] REPOS = {
            "Tonsil/flipper-music-files",
            "neverfa11ing/FlipperMusicRTTTL"
    };
    static final class Result {
        final int source;
        final String path;
        Result(int source, String path) { this.source = source; this.path = path; }
    }
    private static byte[] http(String url, int limit, String accept) throws IOException {
        if (!url.startsWith("https://api.github.com/") &&
                !url.startsWith("https://raw.githubusercontent.com/"))
            throw new IOException("Untrusted ringtone source");
        HttpURLConnection connection = (HttpURLConnection)new URL(url).openConnection();
        connection.setConnectTimeout(9000);
        connection.setReadTimeout(13000);
        connection.setRequestProperty("Accept", accept);
        connection.setRequestProperty("User-Agent", "Weave-Firmware/1.0.0");
        connection.setInstanceFollowRedirects(false);
        try {
            if (connection.getResponseCode() != 200) throw new IOException("HTTP " + connection.getResponseCode());
            try (InputStream stream = connection.getInputStream();
                 ByteArrayOutputStream buffer = new ByteArrayOutputStream()) {
                byte[] chunk = new byte[4096];
                int n;
                while ((n = stream.read(chunk)) >= 0) {
                    if (buffer.size() + n > limit) throw new IOException("Ringtone payload too large");
                    buffer.write(chunk, 0, n);
                }
                return buffer.toByteArray();
            }
        } finally { connection.disconnect(); }
    }
    private static String json(String path) throws IOException {
        return new String(http("https://api.github.com/" + path,
                3_000_000, "application/vnd.github+json"), StandardCharsets.UTF_8);
    }
    private static String branch(int source) throws Exception {
        JSONObject repo = new JSONObject(json("repos/" + REPOS[source]));
        String branch = repo.optString("default_branch", "main");
        if (!branch.matches("[A-Za-z0-9_.-]{1,50}")) throw new IOException("Invalid branch");
        return branch;
    }
    static boolean validPath(String path) {
        return path != null && path.length() > 4 && path.length() <= 125 &&
                path.matches("[A-Za-z0-9_./ -]+") && !path.startsWith("/") &&
                !path.contains("..") &&
                path.toLowerCase(Locale.ROOT).matches(".*\\.(tone|fmf|txt|rtttl)");
    }
    static List<Result> search(String term) throws Exception {
        String q = term == null ? "" : term.trim().toLowerCase(Locale.ROOT);
        if (!q.matches("[a-z0-9 _-]{1,30}")) throw new IOException("Invalid search term");
        ArrayList<Result> matches = new ArrayList<>();
        for (int source = 0; source < REPOS.length && matches.size() < 12; source++) {
            String tree = json("repos/" + REPOS[source] + "/git/trees/" + branch(source) + "?recursive=1");
            JSONArray entries = new JSONObject(tree).optJSONArray("tree");
            if(entries == null) continue;
            for(int i=0; i<entries.length() && matches.size()<12; i++) {
                JSONObject item = entries.optJSONObject(i);
                if(item == null || !"blob".equals(item.optString("type"))) continue;
                String path = item.optString("path");
                String filename = path.substring(path.lastIndexOf('/')+1).toLowerCase(Locale.ROOT);
                if(validPath(path) && filename.contains(q)) matches.add(new Result(source, path));
            }
        }
        return matches;
    }
    static byte[] download(int source, String path) throws Exception {
        if(source < 0 || source >= REPOS.length || !validPath(path))
            throw new IOException("Invalid music file");
        String b = branch(source);
        String encoded = path.replace(" ", "%20");
        byte[] body = http("https://raw.githubusercontent.com/" + REPOS[source] + "/" + b + "/" + encoded,
                16384, "text/plain");
        // Never pass arbitrary executable content to the embedded parser.
        for(byte value : body) if(value == 0) throw new IOException("Binary file rejected");
        return body;
    }
    static String safeLocalName(int source, String path, boolean temporary) {
        String sourceName = path.substring(path.lastIndexOf('/') + 1);
        sourceName = sourceName.replaceAll("[^A-Za-z0-9_.-]", "_");
        int dot = sourceName.lastIndexOf('.');
        String ext = dot >= 0 ? sourceName.substring(dot) : ".fmf";
        String base = dot >= 0 ? sourceName.substring(0, dot) : sourceName;
        base = base.substring(0, Math.min(base.length(), 23));
        return temporary ? "Preview_remote" + ext :
                (source == 0 ? "FMF_" : "RTTTL_") + base + ext;
    }
    private RingtoneCatalog() {}
}
