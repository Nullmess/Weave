package io.github.nullmess.weave;

/** Lit les icônes FXBM 10x10 des manifestes FAP (XBM brut ou heatshrink). */
final class FapIconCodec {
    private static final int WIDTH = 10;
    private static final int HEIGHT = 10;
    private static final int ROW_BYTES = 2;
    private static final int RAW_BYTES = HEIGHT * ROW_BYTES;

    private FapIconCodec() { }

    static boolean[] decode(String hex) {
        if (hex == null || !hex.matches("[0-9a-fA-F]{64}")) return null;
        byte[] payload = new byte[32];
        for (int i = 0; i < payload.length; i++) {
            try { payload[i] = (byte) Integer.parseInt(hex.substring(i * 2, i * 2 + 2), 16); }
            catch (NumberFormatException invalid) { return null; }
        }
        byte[] xbm;
        if (payload[0] == 0) {
            xbm = new byte[RAW_BYTES];
            System.arraycopy(payload, 1, xbm, 0, RAW_BYTES);
        } else if (payload[0] == 1 && payload[1] == 0) {
            int compressedLength = (payload[2] & 255) | ((payload[3] & 255) << 8);
            if (compressedLength < 1 || compressedLength > payload.length - 4) return null;
            xbm = expandHeatshrink(payload, 4, compressedLength);
        } else return null;
        if (xbm == null || xbm.length != RAW_BYTES) return null;
        boolean[] pixels = new boolean[WIDTH * HEIGHT];
        boolean nonEmpty = false;
        for (int y = 0; y < HEIGHT; y++) {
            for (int x = 0; x < WIDTH; x++) {
                boolean lit = ((xbm[y * ROW_BYTES + (x >> 3)] & 255) & (1 << (x & 7))) != 0;
                pixels[y * WIDTH + x] = lit;
                nonEmpty |= lit;
            }
        }
        return nonEmpty ? pixels : null;
    }

    // Heatshrink 8/4, MSB-first : 1=literal, 0=backref (distance + 1, length + 1).
    private static byte[] expandHeatshrink(byte[] input, int start, int length) {
        byte[] output = new byte[RAW_BYTES];
        int bit = 0;
        int end = length * 8;
        int out = 0;
        while (out < output.length) {
            if (bit >= end) return null;
            int tag = getBits(input, start, bit++, 1);
            if (tag == 1) {
                if (bit + 8 > end) return null;
                output[out++] = (byte) getBits(input, start, bit, 8);
                bit += 8;
            } else {
                if (bit + 12 > end) return null;
                int distance = getBits(input, start, bit, 8) + 1;
                bit += 8;
                int count = getBits(input, start, bit, 4) + 1;
                bit += 4;
                if (distance > out || out + count > output.length) return null;
                for (int j = 0; j < count; j++) {
                    output[out] = output[out - distance];
                    out++;
                }
            }
        }
        return output;
    }

    private static int getBits(byte[] input, int start, int bitOffset, int count) {
        int value = 0;
        for (int k = 0; k < count; k++) {
            int offset = bitOffset + k;
            int next = (input[start + (offset >> 3)] >> (7 - (offset & 7))) & 1;
            value = (value << 1) | next;
        }
        return value;
    }
}
