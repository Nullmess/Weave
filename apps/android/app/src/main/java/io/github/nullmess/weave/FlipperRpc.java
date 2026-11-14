package io.github.nullmess.weave;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/** Protocol implementation for Flipper's official length-delimited PB.Main RPC. */
final class FlipperRpc {
    static final int STORAGE_INFO_REQUEST = 28, STORAGE_INFO_RESPONSE = 29;
    static final int STORAGE_LIST_REQUEST = 7, STORAGE_LIST_RESPONSE = 8;
    static final int STORAGE_READ_REQUEST = 9, STORAGE_READ_RESPONSE = 10;
    static final int STORAGE_WRITE_REQUEST = 11, STORAGE_DELETE_REQUEST = 12;
    static final int STORAGE_MKDIR_REQUEST = 13, STORAGE_RENAME_REQUEST = 30;
    static final int GUI_START_STREAM_REQUEST = 20, GUI_STOP_STREAM_REQUEST = 21;
    static final int GUI_FRAME = 22, GUI_INPUT_REQUEST = 23;
    static final int APP_START_REQUEST = 16, SYSTEM_DEVICE_INFO_REQUEST = 32;
    static final int SYSTEM_DEVICE_INFO_RESPONSE = 33;


    static final class Item {
        final String name;
        final boolean directory;
        final long size;
        Item(String name, boolean directory) { this(name, directory, 0); }
        Item(String name, boolean directory, long size) {
            this.name = name; this.directory = directory; this.size = size;
        }
    }
    static final class Reply {
        final int commandId, status, type;
        final boolean hasNext;
        final byte[] content;
        Reply(int commandId, int status, int type, boolean hasNext, byte[] content) {
            this.commandId = commandId; this.status = status; this.type = type;
            this.hasNext = hasNext; this.content = content;
        }
    }
    static final class StorageSpace {
        final long total, free;
        StorageSpace(long total, long free) { this.total = total; this.free = free; }
    }
    static byte[] storageInfoRequest(int id, String path) {
        if (!"/int".equals(path) && !"/ext".equals(path)) throw new IllegalArgumentException("Invalid storage path");
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, path);
        return message(id, STORAGE_INFO_REQUEST, body.toByteArray());
    }
    static StorageSpace parseStorageInfo(byte[] content) throws IOException {
        Reader info = new Reader(content);
        long total = -1, free = -1;
        while (info.available()) {
            long tag = info.varint();
            int field = (int)(tag >>> 3), wire = (int)(tag & 7);
            if (field == 1 && wire == 0) total = info.varint();
            else if (field == 2 && wire == 0) free = info.varint();
            else info.skip(wire);
        }
        if (total <= 0 || free < 0 || free > total)
            throw new IOException("Invalid Flipper storage counters");
        return new StorageSpace(total, free);
    }
    static final class ListReply {
        final int commandId, status;
        final boolean hasNext;
        final List<Item> items;
        ListReply(int commandId, int status, boolean hasNext, List<Item> items) {
            this.commandId = commandId; this.status = status;
            this.hasNext = hasNext; this.items = items;
        }
    }
    static byte[] message(int commandId, int field, byte[] body) {
        return message(commandId, field, body, false);
    }
    static byte[] message(int commandId, int field, byte[] body, boolean hasNext) {
        ByteArrayOutputStream main = new ByteArrayOutputStream();
        writeVarField(main, 1, commandId);
        if (hasNext) writeVarField(main, 3, 1);
        writeByteField(main, field, body);
        byte[] data = main.toByteArray();
        ByteArrayOutputStream framed = new ByteArrayOutputStream();
        writeVarint(framed, data.length);
        framed.write(data, 0, data.length);
        return framed.toByteArray();
    }
    static byte[] listRequest(int commandId, String path) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, path);
        return message(commandId, STORAGE_LIST_REQUEST, body.toByteArray());
    }
    // RPC officiel : PB_System.RebootRequest(mode=DFU=1).
    static byte[] rebootDfuRequest(int id) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeVarField(body, 1, 1);
        return message(id, 31, body.toByteArray());
    }

    static byte[] streamRequest(int id, boolean start) {
        return message(id, start ? GUI_START_STREAM_REQUEST : GUI_STOP_STREAM_REQUEST, new byte[0]);
    }
    /** Send PRESS, SHORT, RELEASE to replicate the Flipper's physical short button sequence. */
    static byte[] inputRequest(int id, int key, int inputType) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeVarField(body, 1, key);
        writeVarField(body, 2, inputType);
        return message(id, GUI_INPUT_REQUEST, body.toByteArray());
    }
    static byte[] launchRequest(int id, String verifiedPath) {
        if (verifiedPath == null || !verifiedPath.matches("/ext/apps/(?:[A-Za-z0-9_. -]+/)*[A-Za-z0-9_. -]+\\.fap")
                || verifiedPath.contains("..")) throw new IllegalArgumentException("Invalid FAP path");
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, verifiedPath); // PB_App.StartRequest.name
        return message(id, APP_START_REQUEST, body.toByteArray());
    }
    /** Explicitly start the BUILT-IN Weave Apps progress view, not an untrusted FAP. */
    static byte[] weaveInstallScreenRequest(int id) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, "Weave Apps");
        writeTextField(body, 2, "weave-install");
        return message(id, APP_START_REQUEST, body.toByteArray());
    }

    static byte[] deviceInfoRequest(int id) {
        return message(id, SYSTEM_DEVICE_INFO_REQUEST, new byte[0]);
    }
    static String[] deviceInfoPair(byte[] body) throws IOException {
        Reader r = new Reader(body);
        String[] pair = {null, null};
        while (r.available()) {
            long tag = r.varint();
            int field = (int)(tag >>> 3), wire = (int)(tag & 7);
            if ((field == 1 || field == 2) && wire == 2) {
                pair[field - 1] = new String(r.bytes(), StandardCharsets.UTF_8);
            } else r.skip(wire);
        }
        return pair;
    }
    static byte[] readRequest(int id, String path) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, path);
        return message(id, STORAGE_READ_REQUEST, body.toByteArray());
    }
    static byte[] readFileData(byte[] payload) throws IOException {
        Reader outer = new Reader(payload);
        while (outer.available()) {
            long tag = outer.varint();
            if ((tag >>> 3) == 1 && (tag & 7) == 2) {
                Reader file = new Reader(outer.bytes());
                while (file.available()) {
                    long ft = file.varint();
                    if ((ft >>> 3) == 4 && (ft & 7) == 2) return file.bytes();
                    file.skip((int)(ft & 7));
                }
            } else outer.skip((int)(tag & 7));
        }
        return new byte[0];
    }
    static byte[] mkdirRequest(int id, String path) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, path);
        return message(id, STORAGE_MKDIR_REQUEST, body.toByteArray());
    }
    static byte[] deleteRequest(int id, String path, boolean recursive) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, path);
        if (recursive) writeVarField(body, 2, 1);
        return message(id, STORAGE_DELETE_REQUEST, body.toByteArray());
    }
    static byte[] renameRequest(int id, String from, String to) {
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, from);
        writeTextField(body, 2, to);
        return message(id, STORAGE_RENAME_REQUEST, body.toByteArray());
    }
    static byte[] writeRequest(int id, String path, byte[] data, int offset, int length, boolean more) {
        ByteArrayOutputStream file = new ByteArrayOutputStream();
        byte[] part = new byte[length];
        System.arraycopy(data, offset, part, 0, length);
        writeByteField(file, 4, part);
        ByteArrayOutputStream body = new ByteArrayOutputStream();
        writeTextField(body, 1, path);
        writeByteField(body, 2, file.toByteArray());
        return message(id, STORAGE_WRITE_REQUEST, body.toByteArray(), more);
    }
    static Reply parseReply(byte[] bytes) throws IOException {
        Reader r = new Reader(bytes);
        int id = -1, status = 0, type = -1;
        boolean hasNext = false;
        byte[] payload = new byte[0];
        while (r.available()) {
            long tag = r.varint();
            int key = (int)(tag >>> 3), wire = (int)(tag & 7);
            if (key == 1 && wire == 0) id = (int)r.varint();
            else if (key == 2 && wire == 0) status = (int)r.varint();
            else if (key == 3 && wire == 0) hasNext = r.varint() != 0;
            else if (key > 3 && wire == 2) { type = key; payload = r.bytes(); }
            else r.skip(wire);
        }
        return new Reply(id, status, type, hasNext, payload);
    }
    static List<Item> parseFiles(byte[] content) throws IOException {
        Reader list = new Reader(content);
        List<Item> items = new ArrayList<>();
        while (list.available()) {
            long tag = list.varint();
            if ((tag >>> 3) == 1 && (tag & 7) == 2) {
                Reader file = new Reader(list.bytes());
                String name = null;
                boolean dir = false;
                long size = 0;
                while (file.available()) {
                    long ft = file.varint();
                    int field = (int)(ft >>> 3), wire = (int)(ft & 7);
                    if (field == 1 && wire == 0) dir = file.varint() == 1;
                    else if (field == 2 && wire == 2) name = new String(file.bytes(), StandardCharsets.UTF_8);
                    else if (field == 3 && wire == 0) size = file.varint();
                    else file.skip(wire);
                }
                if (name != null && !name.isEmpty()) items.add(new Item(name, dir, size));
            } else list.skip((int)(tag & 7));
        }
        return items;
    }
    static byte[] parseScreen(byte[] frame) throws IOException {
        Reader screen = new Reader(frame);
        while (screen.available()) {
            long tag = screen.varint();
            if ((tag >>> 3) == 1 && (tag & 7) == 2) return screen.bytes();
            screen.skip((int)(tag & 7));
        }
        return new byte[0];
    }
    static ListReply parseListReply(byte[] bytes) throws IOException {
        Reply r = parseReply(bytes);
        return new ListReply(r.commandId, r.status, r.hasNext,
                r.type == STORAGE_LIST_RESPONSE ? parseFiles(r.content) : new ArrayList<>());
    }
    static void writeVarField(ByteArrayOutputStream out, int field, long value) {
        writeVarint(out, (long)field * 8); writeVarint(out, value);
    }
    static void writeTextField(ByteArrayOutputStream out, int field, String text) {
        writeByteField(out, field, text.getBytes(StandardCharsets.UTF_8));
    }
    static void writeByteField(ByteArrayOutputStream out, int field, byte[] data) {
        writeVarint(out, (long)field * 8 + 2);
        writeVarint(out, data.length);
        out.write(data, 0, data.length);
    }
    static void writeVarint(ByteArrayOutputStream out, long value) {
        while ((value & ~127L) != 0) { out.write((int)(value & 127) | 128); value >>>= 7; }
        out.write((int)value);
    }
    static int frameLength(byte[] buffer, int offset, int[] prefixBytes) throws IOException {
        long v = 0;
        for (int i = 0; i < 5; i++) {
            if (offset + i >= buffer.length) return -1;
            int b = buffer[offset + i] & 255;
            v |= (long)(b & 127) << (7 * i);
            if ((b & 128) == 0) {
                if (v > 1_048_576) throw new IOException("RPC frame too large");
                prefixBytes[0] = i + 1;
                return (int)v;
            }
        }
        throw new IOException("Invalid protobuf length");
    }
    private static final class Reader {
        final byte[] data;
        int pos;
        Reader(byte[] data) { this.data = data; }
        boolean available() { return pos < data.length; }
        long varint() throws IOException {
            long value = 0;
            for (int i = 0; i < 10; i++) {
                if (pos >= data.length) throw new IOException("Truncated varint");
                int b = data[pos++] & 255;
                value |= (long)(b & 127) << (7 * i);
                if ((b & 128) == 0) return value;
            }
            throw new IOException("Invalid varint");
        }
        byte[] bytes() throws IOException {
            long sz = varint();
            if (sz < 0 || sz > data.length - pos) throw new IOException("Invalid byte field");
            int length = (int)sz;
            byte[] value = new byte[length];
            System.arraycopy(data, pos, value, 0, length);
            pos += length;
            return value;
        }
        void skip(int wire) throws IOException {
            switch (wire) {
                case 0: varint(); return;
                case 1: skipBytes(8); return;
                case 2: long length = varint(); if (length > Integer.MAX_VALUE) throw new IOException("Too long"); skipBytes((int)length); return;
                case 5: skipBytes(4); return;
                default: throw new IOException("Unsupported wire type " + wire);
            }
        }
        void skipBytes(int length) throws IOException {
            if (length < 0 || length > data.length - pos) throw new IOException("Truncated field");
            pos += length;
        }
    }
}
