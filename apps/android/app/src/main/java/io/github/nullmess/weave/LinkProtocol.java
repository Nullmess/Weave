package io.github.nullmess.weave;

import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;

/** Wire codec shared by USB CDC and BLE. Pure Java for platform-independent protocol encoding. */
final class LinkProtocol {
    static final int MAX_LINE = 1024;
    static final int MAX_BODY = 512;

    static final class Message {
        final String action;
        final String[] fields;
        Message(String action, String[] fields) { this.action = action; this.fields = fields; }
    }

    static byte[] frame(String action, String... fields) {
        StringBuilder builder = new StringBuilder("WV1|").append(action);
        for (String field : fields) {
            if (field.indexOf('|') >= 0 || field.indexOf('\n') >= 0 || field.indexOf('\r') >= 0)
                throw new IllegalArgumentException("Invalid field");
            builder.append('|').append(field);
        }
        byte[] output = builder.append('\n').toString().getBytes(StandardCharsets.US_ASCII);
        if (output.length > MAX_LINE) throw new IllegalArgumentException("Frame too long");
        return output;
    }

    static Message parse(byte[] line) {
        if (line.length > MAX_LINE || line.length == 0 || line[line.length - 1] != '\n')
            throw new IllegalArgumentException("Invalid frame size");
        String content = new String(line, 0, line.length - 1, StandardCharsets.US_ASCII);
        String[] fields = content.split("\\|", -1);
        if (fields.length < 2 || !fields[0].equals("WV1")) throw new IllegalArgumentException("Not WV1");
        String[] rest = new String[fields.length - 2];
        System.arraycopy(fields, 2, rest, 0, rest.length);
        return new Message(fields[1], rest);
    }

    static String b64(byte[] data) { return Base64.getUrlEncoder().withoutPadding().encodeToString(data); }
    static byte[] fromB64(String data) { return Base64.getUrlDecoder().decode(data); }

    /** Reassembles lines fragmented across BLE ATT and USB read callbacks. */
    static final class Lines {
        final ByteArrayOutputStream pending = new ByteArrayOutputStream();
        synchronized List<byte[]> accept(byte[] packet) {
            List<byte[]> ready = new ArrayList<>();
            for (byte b : packet) {
                if (pending.size() >= MAX_LINE * 2) { pending.reset(); break; }
                pending.write(b);
                if (b == '\n') {
                    if (pending.size() <= MAX_LINE) ready.add(pending.toByteArray());
                    pending.reset();
                }
            }
            return ready;
        }
    }
}
