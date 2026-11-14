package io.github.nullmess.weave;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.provider.OpenableColumns;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import java.io.File;
import java.io.FileNotFoundException;

/** Read-only bridge for downloaded APK/firmware supplied to Android installers. */
public final class WeaveUpdateProvider extends ContentProvider {
    @Override public boolean onCreate() { return true; }
    @Override public String getType(Uri uri) {
        return "weave-app.apk".equals(uri.getLastPathSegment()) ?
                "application/vnd.android.package-archive" : "application/gzip";
    }
    private File validatedFile(Uri uri) throws FileNotFoundException {
        String name = uri.getLastPathSegment();
        if (!"weave-app.apk".equals(name) && !"weave-firmware.tgz".equals(name))
            throw new FileNotFoundException("Unknown update");
        File file = new File(new File(getContext().getCacheDir(), "weave-updates"), name);
        if (!file.isFile()) throw new FileNotFoundException("No downloaded update");
        return file;
    }
    @Override public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new FileNotFoundException("Read-only");
        return ParcelFileDescriptor.open(validatedFile(uri), ParcelFileDescriptor.MODE_READ_ONLY);
    }
    @Override public Cursor query(Uri uri, String[] projection, String selection, String[] args, String order) {
        try {
            File file = validatedFile(uri);
            String[] columns = projection == null ?
                    new String[]{OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE} : projection;
            MatrixCursor cursor = new MatrixCursor(columns, 1);
            Object[] values = new Object[columns.length];
            for (int i=0;i<columns.length;i++) {
                if (OpenableColumns.DISPLAY_NAME.equals(columns[i])) values[i] = file.getName();
                else if (OpenableColumns.SIZE.equals(columns[i])) values[i] = file.length();
            }
            cursor.addRow(values);
            return cursor;
        } catch (FileNotFoundException e) { return null; }
    }
    @Override public Uri insert(Uri uri, ContentValues values) { return null; }
    @Override public int delete(Uri uri, String selection, String[] args) { return 0; }
    @Override public int update(Uri uri, ContentValues values, String selection, String[] args) { return 0; }
}
