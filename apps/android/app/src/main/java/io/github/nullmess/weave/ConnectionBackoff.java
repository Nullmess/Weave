package io.github.nullmess.weave;

/** Backoff BLE borne, reset uniquement sur connexion RPC effective. */
final class ConnectionBackoff {
    private int failures;
    private long nextAt;

    void attempted(long now) {
        failures = Math.min(8, failures + 1);
        long delay = Math.min(30000L, 3500L * (1L << Math.min(3, failures - 1)));
        nextAt = now + delay;
    }

    void ready() { failures = 0; nextAt = 0; }
    boolean permits(long now) { return now >= nextAt; }
    long remaining(long now) { return Math.max(0, nextAt - now); }
}
