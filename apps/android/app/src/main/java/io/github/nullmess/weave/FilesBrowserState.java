package io.github.nullmess.weave;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/** UI state for a verified Flipper directory listing. No artificial entries or capacity figures. */
final class FilesBrowserState {
    static final String INTERNAL = "/int";
    static final String SD = "/ext";
    static final int NEW = 0, IMPORT = 1, EXPORT = 2, DELETE = 3;
    static final int RENAME = 4, MOVE = 5, COPY = 6, PASTE = 7;

    static final class Entry {
        final String name;
        final boolean directory;
        final long size;

        Entry(String name, boolean directory, long size) {
            if (!validName(name)) throw new IllegalArgumentException("Invalid file name");
            this.name = name;
            this.directory = directory;
            this.size = size;
        }
    }

    String storage = SD;
    String path = SD;
    private boolean listingVerified;
    private List<Entry> visible = Collections.emptyList();
    private final Set<String> selected = new LinkedHashSet<>();
    private final List<String> clipboardPaths = new ArrayList<>();
    private boolean clipboardMove;
    private boolean clipboardDirectories;

    boolean hasListing() { return listingVerified; }
    List<Entry> entries() { return Collections.unmodifiableList(visible); }
    int selectionCount() { return selected.size(); }
    List<String> selectedNames() { return new ArrayList<>(selected); }
    void invalidateListing() { listingVerified = false; visible = Collections.emptyList(); selected.clear(); }

    boolean isSelected(String name) { return selected.contains(name); }
    boolean isAtRoot() { return path.equals(storage); }

    void resetForConnectionChange() {
        listingVerified = false;
        visible = Collections.emptyList();
        selected.clear();
        clipboardPaths.clear();
        clipboardMove = false;
        clipboardDirectories = false;
    }

    void openStorage(String root) {
        if (!INTERNAL.equals(root) && !SD.equals(root)) throw new IllegalArgumentException("Storage root");
        storage = root;
        path = root;
        listingVerified = false;
        visible = Collections.emptyList();
        selected.clear();
    }

    /** Called only when a device-side response contains the listing for this exact path. */
    void setVerifiedListing(String root, String folder, List<Entry> files) {
        if (!storage.equals(root) || !path.equals(folder) || files == null) return;
        ArrayList<Entry> sanitized = new ArrayList<>();
        for (Entry item : files) if (item != null) sanitized.add(item);
        sanitized.sort((a, b) -> {
            if (a.directory != b.directory) return a.directory ? -1 : 1;
            int alphabetic = a.name.compareToIgnoreCase(b.name);
            return alphabetic != 0 ? alphabetic : a.name.compareTo(b.name);
        });
        visible = sanitized;
        listingVerified = true;
        selected.clear();
    }

    void enterDirectory(String folder) {
        if (!listingVerified || !validName(folder)) return;
        Entry found = null;
        for (Entry entry : visible) if (entry.name.equals(folder)) { found = entry; break; }
        if (found == null || !found.directory) return;
        path = path + "/" + folder;
        selected.clear();
        listingVerified = false;
        visible = Collections.emptyList();
    }

    /** Reveals a global search result without exposing internal firmware storage. */
    boolean goToDirectory(String absolutePath) {
        if (absolutePath == null || !(absolutePath.equals(SD) || absolutePath.startsWith(SD + "/")))
            return false;
        if (!absolutePath.equals(SD)) {
            String[] components = absolutePath.substring(SD.length() + 1).split("/", -1);
            for (String component : components) if (!validName(component)) return false;
        }
        storage = SD;
        path = absolutePath;
        invalidateListing();
        return true;
    }

    void goParent() {
        if (isAtRoot()) return;
        int slash = path.lastIndexOf('/');
        path = slash <= 0 ? storage : path.substring(0, slash);
        if (path.length() < storage.length()) path = storage;
        selected.clear();
        listingVerified = false;
        visible = Collections.emptyList();
    }

    void toggle(String name) {
        if (!listingVerified || !validName(name)) return;
        boolean found = false;
        for (Entry entry : visible) if (entry.name.equals(name)) { found = true; break; }
        if (!found) return;
        if (!selected.add(name)) selected.remove(name);
    }

    boolean allows(int operation) {
        if (!listingVerified) return false;
        int count = selected.size();
        if (count == 0) return operation == NEW || operation == IMPORT
                || (operation == PASTE && !clipboardPaths.isEmpty());
        if (count == 1) return operation == EXPORT || operation == DELETE
                || operation == RENAME || operation == MOVE || operation == COPY;
        return operation == EXPORT || operation == DELETE || operation == MOVE || operation == COPY;
    }

    void prepareCopy() { prepareClipboard(false); }
    void prepareMove() { prepareClipboard(true); }
    private void prepareClipboard(boolean move) {
        if (!allows(move ? MOVE : COPY)) return;
        clipboardPaths.clear();
        for (String name : selected) clipboardPaths.add(path + "/" + name);
        clipboardMove = move;
        clipboardDirectories = false;
        for (Entry entry : visible)
            if (entry.directory && selected.contains(entry.name)) clipboardDirectories = true;
        selected.clear();
    }
    boolean hasClipboard() { return !clipboardPaths.isEmpty(); }
    boolean clipboardHasDirectories() { return clipboardDirectories; }
    boolean isMoveClipboard() { return clipboardMove; }
    List<String> clipboardSources() { return new ArrayList<>(clipboardPaths); }
    void clearClipboard() { clipboardPaths.clear(); clipboardMove = false; clipboardDirectories = false; }

    static boolean validName(String name) {
        return name != null && !name.isEmpty() && !name.equals(".") && !name.equals("..")
                && name.indexOf('/') < 0 && name.indexOf('\\') < 0
                && name.indexOf('\n') < 0 && name.indexOf('\r') < 0;
    }
}
