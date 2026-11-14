package io.github.nullmess.weave;

/** Etat des inputs physiques virtuels, sans dependance Android. */
final class GamepadGesture {
    private boolean pressed;
    private boolean held;

    int down() {
        if (pressed) return -1;
        pressed = true;
        held = false;
        return 0; // PRESS
    }

    int hold() {
        if (!pressed || held) return -1;
        held = true;
        return 3; // LONG
    }

    int repeat() { return pressed && held ? 4 : -1; }

    int up(boolean cancelled) {
        if (!pressed) return -2;
        pressed = false;
        return cancelled || held ? -1 : 2; // SHORT, puis RELEASE
    }
}
