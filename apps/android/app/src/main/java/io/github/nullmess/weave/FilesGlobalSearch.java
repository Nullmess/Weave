package io.github.nullmess.weave;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Set;

/** Nonblocking, bounded traversal of Flipper /ext using one standard RPC listing at a time. */
final class FilesGlobalSearch {
    static final int MAX_DIRECTORIES = 256;
    static final int MAX_MATCHES = 150;
    private static final int MAX_DEPTH = 16;

    static final class Result {
        final String path;
        final String parent;
        final String name;
        final boolean directory;
        Result(String parent, String name, boolean directory) {
            this.parent = parent;
            this.name = name;
            this.path = parent + "/" + name;
            this.directory = directory;
        }
    }

    private final String query;
    private final String foldedQuery;
    private final ArrayDeque<String> folders = new ArrayDeque<>();
    private final Set<String> knownFolders = new HashSet<>();
    private final List<Result> matches = new ArrayList<>();
    private String waitingFor;
    private boolean finished;
    private boolean limited;
    private int scanned;
    private int failures;

    FilesGlobalSearch(String search) {
        query = search == null ? "" : search.trim();
        foldedQuery = query.toLowerCase(Locale.ROOT);
        folders.add(FilesBrowserState.SD);
        knownFolders.add(FilesBrowserState.SD);
        if (foldedQuery.isEmpty()) finished = true;
    }

    String query() { return query; }
    boolean isFinished() { return finished; }
    boolean isWaitingFor(String path) { return waitingFor != null && waitingFor.equals(path); }
    String waitingFor() { return waitingFor; }
    List<Result> matches() { return Collections.unmodifiableList(matches); }

    String nextDirectory() {
        if (finished || waitingFor != null) return null;
        if (scanned >= MAX_DIRECTORIES || matches.size() >= MAX_MATCHES) {
            limited = !folders.isEmpty();
            finished = true;
            return null;
        }
        String next = folders.poll();
        if (next == null) {
            finished = true;
            return null;
        }
        waitingFor = next;
        return next;
    }

    void accept(String path, boolean ok, String[] names, boolean[] dirs, long[] sizes) {
        if (!isWaitingFor(path) || finished) return;
        waitingFor = null;
        scanned++;
        if (!ok || names == null || dirs == null || sizes == null) {
            failures++;
        } else {
            int n = Math.min(names.length, Math.min(dirs.length, sizes.length));
            for (int i = 0; i < n; i++) {
                String name = names[i];
                if (!FilesBrowserState.validName(name)) continue;
                String absolute = path + "/" + name;
                if (dirs[i] && pathDepth(absolute) <= MAX_DEPTH && !knownFolders.contains(absolute)) {
                    if (knownFolders.size() < MAX_DIRECTORIES) {
                        knownFolders.add(absolute);
                        folders.add(absolute);
                    } else limited = true;
                }
                if (name.toLowerCase(Locale.ROOT).contains(foldedQuery)) {
                    matches.add(new Result(path, name, dirs[i]));
                    if (matches.size() >= MAX_MATCHES) break;
                }
            }
        }
        if (matches.size() >= MAX_MATCHES || scanned >= MAX_DIRECTORIES) {
            limited = limited || !folders.isEmpty() || matches.size() >= MAX_MATCHES;
            finished = true;
        } else if (folders.isEmpty()) finished = true;
    }

    void stopEarly() { finished = true; limited = true; waitingFor = null; }

    String status() {
        if (!finished) return "Searching SD… " + scanned + " folders · " + matches.size() + " results";
        if (limited) return matches.size() + " results · Partial search (refine query)";
        if (failures > 0) return matches.size() + " results · " + failures + " folders unavailable";
        return matches.size() + " results · Search complete";
    }

    private static int pathDepth(String path) {
        int count = 0;
        for (int i = 0; i < path.length(); i++) if (path.charAt(i) == '/') count++;
        return count - 1; // /ext itself is depth zero.
    }
}
