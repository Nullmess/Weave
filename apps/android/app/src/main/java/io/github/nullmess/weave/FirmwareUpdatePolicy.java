package io.github.nullmess.weave;

/** Règles FW partagées, sans dépendance Android (tests JVM). */
final class FirmwareUpdatePolicy {
    static boolean validVersion(String value) {
        return value != null && value.matches("[0-9]{1,9}\\.[0-9]{1,9}\\.[0-9]{1,9}");
    }
    static String versionIn(String value) {
        if (value == null) return null;
        java.util.regex.Matcher m = java.util.regex.Pattern.compile(
                "(?i)(?<![0-9.])v?([0-9]{1,9}\\.[0-9]{1,9}\\.[0-9]{1,9})(?![0-9.])").matcher(value);
        return m.find() ? m.group(1) : null;
    }
    static int compare(String a, String b) {
        if (!validVersion(a) || !validVersion(b)) return 0;
        String[] x = a.split("\\."), y = b.split("\\.");
        for (int i = 0; i < 3; i++) {
            int cmp = Integer.compare(Integer.parseInt(x[i]), Integer.parseInt(y[i]));
            if (cmp != 0) return cmp;
        }
        return 0;
    }
    static boolean authorized(String[] fields, String device, String checkDevice, String token,
                              String version, String nonce, String consumed, long age) {
        return fields != null && fields.length == 3 && device != null && device.equals(checkDevice) &&
                fields[0].matches("[0-9]{1,12}-[0-9]{1,12}") && fields[0].equals(token) &&
                validVersion(fields[1]) && fields[1].equals(version) &&
                fields[2].matches("[0-9a-f]{24}") && fields[2].equals(nonce) &&
                !fields[2].equals(consumed) && age >= 0 && age <= 600000L;
    }
    static boolean verified(String uid, String observedUid, String expected, String reported) {
        return uid != null && uid.equals(observedUid) && validVersion(expected) && expected.equals(versionIn(reported));
    }
    static int uploadPercent(long sent, long total) {
        if (total <= 0) return 15;
        return 15 + (int)(83L * Math.max(0L, Math.min(sent, total)) / total);
    }
    static boolean activePhase(String phase) {
        return "waiting".equals(phase) || "downloading".equals(phase) || "staging".equals(phase) ||
                "uploading".equals(phase) || "preparing".equals(phase) || "rebooting".equals(phase);
    }
    private FirmwareUpdatePolicy() { }
}
