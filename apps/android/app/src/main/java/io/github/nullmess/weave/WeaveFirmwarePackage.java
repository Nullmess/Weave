package io.github.nullmess.weave;

import java.io.BufferedInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.zip.GZIPInputStream;

/** Strict on-phone extraction of the FBT update bundle; nothing is flashed here. */
final class WeaveFirmwarePackage {
    static final long MAX_FILE_BYTES = 16_000_000;
    private static final long MAX_TOTAL_BYTES = 48_000_000;
    private static final int MAX_FILES = 64;

    static final class Member {
        final File file;
        final String remotePath;
        final long size;
        Member(File file, String remotePath, long size) {
            this.file = file;
            this.remotePath = remotePath;
            this.size = size;
        }
    }

    static final class Prepared {
        final List<Member> files;
        final String manifestPath;
        final long total;
        Prepared(List<Member> files, String manifestPath, long total) {
            this.files = Collections.unmodifiableList(files);
            this.manifestPath = manifestPath;
            this.total = total;
        }
    }

    static Prepared extract(File tgz, File stage, String version) throws IOException {
        if (tgz == null || !tgz.isFile() || tgz.length() < 128 ||
                version == null || !version.matches("\\d+\\.\\d+\\.\\d+"))
            throw new IOException("Invalid firmware download or version");
        if (!stage.isDirectory() && !stage.mkdirs()) throw new IOException("Cannot prepare firmware cache");
        String root = "f7-update-" + version;
        ArrayList<Member> files = new ArrayList<>();
        java.util.HashSet<String> names = new java.util.HashSet<>();
        long total = 0;
        boolean manifestFound = false;
        try (InputStream input = new BufferedInputStream(new GZIPInputStream(new FileInputStream(tgz)))) {
            byte[] header = new byte[512];
            while (true) {
                readFully(input, header, 0, 512);
                boolean allZero = true;
                for (byte value : header) if (value != 0) { allZero = false; break; }
                if (allZero) break;
                // POSIX tar checksum rejects a corrupt archive before placing files on the SD.
                long expected = octal(header, 148, 8);
                long sum = 0;
                for (int i = 0; i < 512; i++) sum += i >= 148 && i < 156 ? 32 : header[i] & 0xff;
                if (sum != expected) throw new IOException("Invalid tar header checksum");
                String name = ascii(header, 0, 100);
                String prefix = ascii(header, 345, 155);
                if (!prefix.isEmpty()) name = prefix + "/" + name;
                long size = octal(header, 124, 12);
                byte type = header[156];
                if (size < 0 || size > MAX_FILE_BYTES || total + size > MAX_TOTAL_BYTES)
                    throw new IOException("Firmware archive exceeds transfer limits");
                boolean directory = type == '5';
                // Reject links, PAX path overrides and other special file kinds outright.
                if (!(type == '0' || type == 0 || directory))
                    throw new IOException("Unsupported firmware archive entry type");
                String normalized = name.endsWith("/") ? name.substring(0, name.length() - 1) : name;
                String[] parts = normalized.split("/", -1);
                if (parts.length < 1 || parts.length > 2 || !root.equals(parts[0]) ||
                        (parts.length == 2 && (!parts[1].matches("[A-Za-z0-9_.-]+") ||
                                ".".equals(parts[1]) || "..".equals(parts[1]))) ||
                        !names.add(normalized) || (directory && parts.length != 1) ||
                        (!directory && parts.length != 2))
                    throw new IOException("Unexpected firmware archive path");
                if (directory && size != 0) throw new IOException("Invalid tar directory entry");
                if (!directory) {
                    if (files.size() >= MAX_FILES || size == 0) throw new IOException("Empty or excessive firmware entries");
                    File target = new File(stage, parts[1]);
                    try (FileOutputStream output = new FileOutputStream(target)) {
                        byte[] buffer = new byte[16384];
                        long remaining = size;
                        while (remaining > 0) {
                            int n = input.read(buffer, 0, (int)Math.min(buffer.length, remaining));
                            if (n < 0) throw new IOException("Truncated firmware archive");
                            output.write(buffer, 0, n);
                            remaining -= n;
                        }
                        output.getFD().sync();
                    }
                    files.add(new Member(target, "/ext/update/" + root + "/" + parts[1], size));
                    total += size;
                    if ("update.fuf".equals(parts[1])) {
                        // Flipper update manifests are plain text and begin with a magic header.
                        if (size < 40 || size > 16384) throw new IOException("Invalid update manifest size");
                        try (FileInputStream check = new FileInputStream(target)) {
                            byte[] signature = new byte[80];
                            int n = check.read(signature);
                            if (n <= 0 || !new String(signature, 0, n, StandardCharsets.US_ASCII)
                                    .contains("Flipper firmware upgrade configuration"))
                                throw new IOException("Unexpected Flipper update manifest");
                        }
                        manifestFound = true;
                    }
                }
                int padding = (int)((512 - size % 512) % 512);
                if (padding > 0) {
                    byte[] discard = new byte[padding];
                    readFully(input, discard, 0, padding);
                }
            }
        } catch (IOException failure) {
            for (Member member : files) member.file.delete();
            throw failure;
        }
        if (!manifestFound || files.isEmpty()) {
            for (Member member : files) member.file.delete();
            throw new IOException("Firmware bundle has no update.fuf");
        }
        // Le manifeste doit référencer uniquement des fichiers du TGZ vérifié,
        // jamais un ancien fichier laissé sur la SD par un transfert interrompu.
        try {
            java.util.Map<String, String> fields = new java.util.HashMap<>();
            for (String line : java.nio.file.Files.readAllLines(new File(stage, "update.fuf").toPath(),
                    StandardCharsets.US_ASCII)) {
                if (line.startsWith("#")) continue;
                int colon = line.indexOf(':');
                if (colon <= 0) continue;
                String key = line.substring(0, colon).trim();
                if (fields.put(key, line.substring(colon + 1).trim()) != null)
                    throw new IOException("Duplicate update manifest field");
            }
            if (!version.equals(fields.get("Info")) || !"7".equals(fields.get("Target")))
                throw new IOException("Firmware manifest version or target mismatch");
            for (String key : new String[]{"Loader", "Firmware", "Radio", "Resources", "Splashscreen"}) {
                String name = fields.getOrDefault(key, "");
                if (name.isEmpty() && !"Loader".equals(key) && !"Firmware".equals(key)) continue;
                if (!name.matches("[A-Za-z0-9_.-]+") || !names.contains(root + "/" + name))
                    throw new IOException("Missing or invalid manifest member: " + key);
            }
        } catch (IOException invalid) {
            for (Member member : files) member.file.delete();
            throw invalid;
        }
        // Publier le manifeste en dernier. Aucun reboot avant l'ACK de préparation RPC.
        files.sort((a, b) -> Boolean.compare(a.remotePath.endsWith("/update.fuf"),
                b.remotePath.endsWith("/update.fuf")));
        return new Prepared(files, "/ext/update/" + root + "/update.fuf", total);
    }
    private static void readFully(InputStream input, byte[] data, int offset, int length) throws IOException {
        int received = 0;
        while (received < length) {
            int n = input.read(data, offset + received, length - received);
            if (n < 0) throw new IOException("Truncated firmware tar");
            received += n;
        }
    }
    private static long octal(byte[] header, int offset, int length) throws IOException {
        String value = ascii(header, offset, length).trim();
        try { return value.isEmpty() ? 0 : Long.parseLong(value, 8); }
        catch (NumberFormatException e) { throw new IOException("Invalid tar size", e); }
    }
    private static String ascii(byte[] bytes, int offset, int len) throws IOException {
        int end = offset;
        while (end < offset + len && bytes[end] != 0) {
            int b = bytes[end] & 0xff;
            if (b < 32 || b > 126) throw new IOException("Invalid tar header text");
            end++;
        }
        return new String(bytes, offset, end - offset, StandardCharsets.US_ASCII);
    }
    private WeaveFirmwarePackage() { }
}
