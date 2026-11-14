package io.github.nullmess.weave;

/** Visible-page UI budget. This does NOT suspend the Flipper transport, network
 * gateway, global TextInput bridge or in-flight file/install operations. */
final class PageWorkPolicy {
    private PageWorkPolicy() { }

    static long refreshDelayMillis(int page, boolean activeTransfer) {
        // Fast feedback for the live Home screen and any ongoing transfer.
        if (page == 0 || activeTransfer) return 1000L;
        // Terminal text arrives through events; periodic checks just reconcile state.
        if (page == 2) return 1800L;
        // Apps, Files, Settings get their data through broadcasts or user actions.
        return 2800L;
    }
}
