package io.github.nullmess.weave;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Orders real catalog build links. This NEVER changes the firmware's advertised API. */
final class FapBuildSelector {
    private static final Pattern SDK = Pattern.compile(
            "/build/f7/(\\d+)\\.(\\d+)$");

    private FapBuildSelector() { }

    static String apiOf(String path) {
        if (path == null) return null;
        Matcher match = SDK.matcher(path);
        return match.find() ? match.group(1) + "." + match.group(2) : null;
    }

    private static int[] version(String value) {
        if (value == null) return null;
        Matcher match = Pattern.compile("^(\\d+)\\.(\\d+)$").matcher(value);
        if (!match.matches()) return null;
        try {
            return new int[] {Integer.parseInt(match.group(1)), Integer.parseInt(match.group(2))};
        } catch (NumberFormatException ignored) {
            return null;
        }
    }

    static boolean compatible(String buildApi, String deviceApi) {
        int[] build = version(buildApi), device = version(deviceApi);
        return build != null && device != null && build[0] == device[0] && build[1] <= device[1];
    }

    /**
     * Exact or backwards-compatible builds first; only then try another published build.
     * When the API cannot be read, use the newest published SDK build, not a fabricated API.
     */
    static List<String> ordered(Collection<String> publishedLinks, String deviceApi) {
        final int[] device = version(deviceApi);
        List<String> result = new ArrayList<>();
        if (publishedLinks == null) return result;
        for (String path : new LinkedHashSet<>(publishedLinks)) {
            if (version(apiOf(path)) != null) result.add(path);
        }
        result.sort(new Comparator<String>() {
            @Override public int compare(String left, String right) {
                int[] a = version(apiOf(left)), b = version(apiOf(right));
                int pa = priority(a, device), pb = priority(b, device);
                if (pa != pb) return Integer.compare(pa, pb);
                if (device != null && pa == 1) {
                    // For newer minor versions, try the smallest difference first.
                    int cmp = Integer.compare(a[1], b[1]);
                    if (cmp != 0) return cmp;
                }
                if (device != null && pa == 2) {
                    int cmp = Integer.compare(Math.abs(a[0] - device[0]), Math.abs(b[0] - device[0]));
                    if (cmp != 0) return cmp;
                }
                int majorCmp = Integer.compare(b[0], a[0]);
                if (majorCmp != 0) return majorCmp;
                int minorCmp = Integer.compare(b[1], a[1]);
                return minorCmp != 0 ? minorCmp : left.compareTo(right);
            }
        });
        return result;
    }

    private static int priority(int[] build, int[] device) {
        if (device == null) return 0;
        if (build[0] == device[0] && build[1] <= device[1]) return 0;
        if (build[0] == device[0]) return 1;
        return 2;
    }
}
