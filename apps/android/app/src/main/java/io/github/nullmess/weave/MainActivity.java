package io.github.nullmess.weave;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.PendingIntent;
import android.content.ClipData;
import android.content.ClipboardManager;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ActivityNotFoundException;
import android.net.Uri;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.animation.ObjectAnimator;
import android.animation.ValueAnimator;
import android.animation.Animator;
import android.animation.AnimatorListenerAdapter;
import android.view.animation.LinearInterpolator;
import android.graphics.Color;
import android.graphics.Canvas;
import android.graphics.Paint;
import android.graphics.RectF;
import android.graphics.Rect;
import android.graphics.Bitmap;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.Typeface;
import android.graphics.Outline;
import android.graphics.drawable.GradientDrawable;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.RippleDrawable;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.os.PowerManager;
import android.provider.Settings;
import android.provider.OpenableColumns;
import android.database.Cursor;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.text.Editable;
import android.text.InputType;
import android.text.SpannableString;
import android.text.Spanned;
import android.text.style.ForegroundColorSpan;
import android.text.TextWatcher;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.ScaleGestureDetector;
import android.util.TypedValue;
import android.view.View;
import android.view.ViewOutlineProvider;
import android.view.ViewGroup;
import android.view.animation.DecelerateInterpolator;
import android.view.animation.PathInterpolator;
import android.widget.Button;
import android.widget.CompoundButton;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.HorizontalScrollView;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.ProgressBar;
import android.widget.ScrollView;
import android.widget.Switch;
import android.widget.TextView;
import android.widget.Toast;
import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.File;
import java.util.zip.ZipEntry;
import java.util.zip.ZipOutputStream;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;
import org.json.JSONArray;
import java.io.OutputStream;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.regex.Pattern;
import java.util.regex.Matcher;
import java.util.HashMap;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.Locale;

/** Native Weave companion. Bluetooth link status is separate from WV1 gateway readiness. */
public final class MainActivity extends Activity {
    private static final int HOME = 0, APPS = 1, CONSOLE = 2, FILES = 3, SETTINGS = 4, TABS = 5;
    private static final int BACKGROUND = 0xff050505, SURFACE = 0xff0a0a0a;
    private static final int CARD = 0xff101010, BORDER = 0xff292929;
    private static final int ORANGE = 0xffff7a18, ORANGE_DARK = 0xff3a1c09;
    private static final int WHITE = 0xfff6f6f6, MUTED = 0xff999999;
    private static final int GREEN = 0xff5cd68a, RED = 0xffff6b6b;
    private static final String USB_PERMISSION = "io.github.nullmess.weave.USB_PERMISSION";
    private static final int REQUEST_BLE = 42, REQUEST_NOTIFICATIONS = 43, REQUEST_ENABLE_BLUETOOTH = 44, REQUEST_PHONE_STATE = 45;
    private static final int APP_NOT_INSTALLED = 0, APP_DOWNLOADING = 1, APP_INSTALLED = 2,
            APP_UPDATE = 3, APP_QUEUED = 4;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private final List<Runnable> releaseGamepadOnPause = new ArrayList<>();
    private final View[] pages = new View[TABS];
    private final TextView[] tabs = new TextView[TABS];
    // A single orange pill travels UNDER the tabs. Never recreate it when switching pages.
    private FrameLayout navigationFrame;
    private LinearLayout navigationTabRow;
    private View navigationPill;
    private ValueAnimator navigationPillAnimator;
    private boolean navigationPillPlaced;

    private SharedPreferences prefs;
    private TextView pageTitle, chip, headline, subtitle, indicator;
    private TextView serviceState, flipperState, transportState, requestsValue, errorsValue;
    private TextView statusLine, modeValue, consoleText, hint;
    private static final int REQUEST_TERMINAL_EXPORT = 75;
    private static final int REQUEST_SCREEN_EXPORT = 76;
    private static final int REQUEST_FILE_IMPORT = 77;
    private static final int REQUEST_FILE_EXPORT = 78;
    private static final int REQUEST_FAP_IMPORT = 79;
    private static final int REQUEST_FILE_EXPORT_ARCHIVE = 80;
    private static final int REQUEST_UPDATE_APK_SAVE = 81;
    private static final int REQUEST_UPDATE_APK_PERMISSION = 82;
    private static final int REQUEST_UPDATE_APK_INSTALL = 83;
    private File pendingUpdateApk;
    private String pendingApkVersion;
    private int apkUpdateRequest;
    private boolean apkReadyToInstall;

    private FrameLayout terminalPanel;
    private TextView terminalCliTab, terminalLogsTab;
    private int terminalMode = 0; // 0 = Flipper CLI (BLE or USB), 1 = LOGS
    private final StringBuilder cliTranscript = new StringBuilder();
    private ScaleGestureDetector terminalPinch;
    private final class PinchScrollView extends ScrollView {
        PinchScrollView(Context context) { super(context); }
        @Override public boolean dispatchTouchEvent(MotionEvent event) {
            if (terminalPinch != null) terminalPinch.onTouchEvent(event);
            if (event.getPointerCount() >= 2 || (terminalPinch != null && terminalPinch.isInProgress()))
                return true;
            return super.dispatchTouchEvent(event);
        }
    }
    private void applyTerminalScale(float factor) {
        float next = Math.max(9f, Math.min(24f, terminalFontSp * factor));
        if (Math.abs(next - terminalFontSp) < .08f) return;
        terminalFontSp = next;
        prefs.edit().putFloat("terminal_font_sp", terminalFontSp).apply();
        if (cliOutput != null) cliOutput.setTextSize(TypedValue.COMPLEX_UNIT_SP, terminalFontSp);
        if (terminalMode == 1 && terminalPanel != null)
            updateTerminalTextSizes(terminalPanel);
    }
    private void updateTerminalTextSizes(View node) {
        if (node instanceof TextView && Boolean.TRUE.equals(node.getTag()))
            ((TextView)node).setTextSize(TypedValue.COMPLEX_UNIT_SP, terminalFontSp);
        if (node instanceof ViewGroup) {
            ViewGroup group = (ViewGroup)node;
            for (int i = 0; i < group.getChildCount(); i++) updateTerminalTextSizes(group.getChildAt(i));
        }
    }
    private TextView cliOutput;
    private boolean cliUsbConnected, cliBleConnected, cliPermissionPending;
    private float terminalFontSp = 12f;
    private boolean terminalLastConnected;
    private String terminalLastLog = null;
    private int terminalLastMode = -1;
    private TextView flipperScreenStatus;
    private boolean screenAwaitingFreshFrame = true;
    private boolean screenWasLockedByInstall;
    private ImageView flipperScreenImage;
    private Bitmap lastScreenBitmap;
    // Alternate framebuffer objects. Updating pixels in-place on an attached
    // BitmapDrawable is not reliably invalidated by hardware-accelerated views.
    private final Bitmap[] screenBitmaps = new Bitmap[2];
    private int nextScreenBitmap;
    private long lastRealScreenFrameAt;
    // Home refresh is complete only after an actual new device framebuffer.
    private long homeScreenRefreshRequestedAt;
    private boolean screenRecoveryFailed;
    // Declare gesture state before instance-initialized callbacks that capture it.
    private boolean gestureRefreshActive;
    private int gestureRefreshPage = -1;
    private final Runnable homeScreenRefreshRetry = () -> {
        if (!gestureRefreshActive || gestureRefreshPage != HOME ||
                homeScreenRefreshRequestedAt == 0 || !flipperIsConnected() ||
                activeBadgeJob() != null || !screenAwaitingFreshFrame) return;
        // A second soft GUI RPC restart is safe; never reboot or disconnect a
        // Flipper merely because a framebuffer is late or a page is static.
        command(WeaveService.ACTION_RECOVER_SCREEN);
    };
    private final int[] screenPixelBuffer = new int[128 * 64];
    private FrameLayout contentHost;

    // One vector/animated refresh affordance shared by all five pages.
    private FrameLayout gestureRefreshIndicator;
    private ImageView gestureRefreshGlyph;
    private ObjectAnimator gestureRefreshAnimator;
    private String gestureFilesExpectedPath;
    // A detail swipe belongs to one FAP, never to the entire Apps catalog.
    private String gestureRefreshAppId;
    private int gestureSettingsChecksPending;
    private boolean gestureSettingsFirmwarePending;
    private long gestureRefreshToken;
    private final Runnable gestureRefreshFailsafe = () -> {
        if (gestureRefreshActive && gestureRefreshPage == HOME &&
                homeScreenRefreshRequestedAt != 0 && screenAwaitingFreshFrame &&
                flipperIsConnected() && activeBadgeJob() == null) {
            screenRecoveryFailed = true;
            refresh(); // Show a real failure instead of silently claiming success.
        }
        finishGestureRefresh();
    };
    private float gestureStartX, gestureStartY;
    private boolean gestureWatching, gestureCanPullDown, gestureCanPushUp;
    private ScrollView gestureScrollTarget;
    private int gestureStartPage;

    private View findTouchedLeaf(View root, float x, float y) {
        if (root == null || root.getVisibility() != View.VISIBLE) return null;
        Rect bounds = new Rect();
        if (!root.getGlobalVisibleRect(bounds) || !bounds.contains((int)x, (int)y)) return null;
        if (root instanceof ViewGroup) {
            ViewGroup group = (ViewGroup) root;
            for (int i = group.getChildCount() - 1; i >= 0; --i) {
                View leaf = findTouchedLeaf(group.getChildAt(i), x, y);
                if (leaf != null) return leaf;
            }
        }
        return root;
    }

    private void watchRefreshGesture(MotionEvent event) {
        if (contentHost == null || !appForeground || gestureRefreshActive) return;
        int action = event.getActionMasked();
        if (action == MotionEvent.ACTION_DOWN) {
            gestureWatching = false;
            gestureScrollTarget = null;
            gestureStartPage = selectedPage;
            gestureStartX = event.getRawX();
            gestureStartY = event.getRawY();
            View touched = findTouchedLeaf(pages[selectedPage], gestureStartX, gestureStartY);
            if (touched == null) return; // Never intercept tabs, navigation or the header.
            // Search fields, input controls, image gestures and the remote pad
            // must keep their own touch sequence.
            for (View v = touched; v != null && v != pages[selectedPage];) {
                if (v instanceof EditText || v instanceof Switch || v instanceof HorizontalScrollView)
                    return;
                if (v instanceof ScrollView && gestureScrollTarget == null)
                    gestureScrollTarget = (ScrollView)v;
                if (selectedPage == HOME && v.isClickable()) return;
                if (!(v.getParent() instanceof View)) break;
                v = (View)v.getParent();
            }
            gestureCanPullDown = gestureScrollTarget == null ||
                    !gestureScrollTarget.canScrollVertically(-1);
            gestureCanPushUp = gestureScrollTarget == null ||
                    !gestureScrollTarget.canScrollVertically(1);
            gestureWatching = gestureCanPullDown || gestureCanPushUp;
        } else if (action == MotionEvent.ACTION_MOVE) {
            if (!gestureWatching || event.getPointerCount() != 1 ||
                    selectedPage != gestureStartPage) return;
            float dx = event.getRawX() - gestureStartX;
            float dy = event.getRawY() - gestureStartY;
            boolean eligible = dy > 0 ? gestureCanPullDown : gestureCanPushUp;
            if (!eligible || Math.abs(dy) <= Math.abs(dx) * 1.35f) {
                if (!gestureRefreshActive) hideGestureRefreshPreview();
                return;
            }
            // Expose the same SVG-derived Android vector during the pull.
            if (Math.abs(dy) >= dp(14)) {
                showGestureRefreshPreview(Math.min(1f, Math.abs(dy) / dp(90)));
            }
        } else if (action == MotionEvent.ACTION_POINTER_DOWN || action == MotionEvent.ACTION_CANCEL) {
            gestureWatching = false;
            hideGestureRefreshPreview();
        }
    }

    private void releaseRefreshGesture(MotionEvent event) {
        if (event.getActionMasked() != MotionEvent.ACTION_UP) return;
        boolean valid = gestureWatching && event.getPointerCount() == 1 &&
                selectedPage == gestureStartPage && !gestureRefreshActive;
        gestureWatching = false;
        if (!valid) { hideGestureRefreshPreview(); return; }
        float dx = event.getRawX() - gestureStartX;
        float dy = event.getRawY() - gestureStartY;
        boolean eligible = dy > 0 ? gestureCanPullDown : gestureCanPushUp;
        boolean stillAtEdge = selectedPage == SETTINGS || gestureScrollTarget == null ||
                !gestureScrollTarget.canScrollVertically(dy > 0 ? -1 : 1);
        if (eligible && stillAtEdge && Math.abs(dy) >= dp(90) &&
                Math.abs(dy) > Math.abs(dx) * 1.35f) {
            beginGestureRefresh();
        } else hideGestureRefreshPreview();
    }

    @Override public boolean dispatchTouchEvent(MotionEvent event) {
        // Observe without consuming: taps, scrolling, keyboard and gamepad stay native.
        watchRefreshGesture(event);
        boolean consumed = super.dispatchTouchEvent(event);
        releaseRefreshGesture(event);
        return consumed;
    }

    private void showGestureRefreshPreview(float fraction) {
        if (gestureRefreshIndicator == null || gestureRefreshActive) return;
        gestureRefreshIndicator.animate().cancel();
        gestureRefreshIndicator.setAlpha(Math.min(1f, 0.35f + fraction * 0.65f));
        gestureRefreshIndicator.setTranslationY(dp(-20) * (1f - fraction));
        gestureRefreshIndicator.setVisibility(View.VISIBLE);
        gestureRefreshGlyph.setRotation(fraction * 240f);
    }

    private void hideGestureRefreshPreview() {
        if (gestureRefreshIndicator != null && !gestureRefreshActive) {
            gestureRefreshIndicator.animate().cancel();
            gestureRefreshIndicator.setVisibility(View.GONE);
            gestureRefreshIndicator.setTranslationY(0);
        }
    }

    private void beginGestureRefresh() {
        if (gestureRefreshActive) return;
        // Refresh is coalesced with the toolbar's existing request, not a second job.
        if (selectedPage == APPS && appDetailEntry == null &&
                (appsRefreshInProgress || catalogRequestInFlight)) {
            hideGestureRefreshPreview();
            return;
        }
        if (selectedPage == FILES && (!flipperIsConnected() || activeBadgeJob() != null)) {
            hideGestureRefreshPreview();
            refresh();
            return;
        }
        gestureRefreshActive = true;
        gestureRefreshPage = selectedPage;
        gestureRefreshAppId = selectedPage == APPS && appDetailEntry != null ?
                normalizedAppId(appDetailEntry.id) : null;
        final long refreshToken = ++gestureRefreshToken;
        gestureRefreshIndicator.animate().cancel();
        gestureRefreshIndicator.setVisibility(View.VISIBLE);
        gestureRefreshIndicator.setAlpha(1f);
        gestureRefreshIndicator.setTranslationY(0);
        gestureRefreshAnimator = ObjectAnimator.ofFloat(gestureRefreshGlyph, View.ROTATION, 0f, 360f);
        gestureRefreshAnimator.setDuration(850);
        gestureRefreshAnimator.setInterpolator(new LinearInterpolator());
        gestureRefreshAnimator.setRepeatCount(ObjectAnimator.INFINITE);
        gestureRefreshAnimator.start();
        handler.removeCallbacks(gestureRefreshFailsafe);
        handler.postDelayed(gestureRefreshFailsafe,
                selectedPage == APPS ? 31000 : selectedPage == SETTINGS ? 25000 : 12000);
        int page = selectedPage;
        switch (page) {
            case APPS:
                if (appDetailEntry != null) refreshCurrentAppDetail(refreshToken);
                else requestFullAppsRefresh();
                break;
            case FILES:
                if (!filesSearchTerm.isEmpty()) {
                    startFilesGlobalSearch(); // Search again, without changing folders.
                    finishGestureRefresh();
                } else {
                    gestureFilesExpectedPath = filesState.path;
                    requestedFilesPath = null;
                    filesState.invalidateListing();
                    updateFilesPanel(); // Requests one fresh directory listing.
                }
                sendDeviceCommand(WeaveService.ACTION_STORAGE_INFO, "path", FilesBrowserState.SD);
                break;
            case CONSOLE:
                renderTerminalPanel(false); // Keep CLI output and focused prompt.
                if (activeBadgeJob() == null &&
                        prefs.getBoolean(WeaveService.KEY_ENABLED, false))
                    command(WeaveService.ACTION_SCAN);
                refresh();
                handler.postDelayed(() -> { if (gestureRefreshPage == page) finishGestureRefresh(); }, 850);
                break;
            case HOME:
                // Refresh the *framebuffer session*, not only BLE device metadata.
                // Do not interrupt native FAP transfers or pretend a stale image
                // is live. A fresh EVENT_SCREEN is the completion acknowledgment.
                if (flipperIsConnected() && activeBadgeJob() == null) {
                    homeScreenRefreshRequestedAt = android.os.SystemClock.uptimeMillis();
                    screenRecoveryFailed = false;
                    screenAwaitingFreshFrame = true;
                    if (flipperScreenStatus != null) {
                        flipperScreenStatus.setText("Resynchronizing Flipper display…");
                        flipperScreenStatus.setVisibility(View.VISIBLE);
                    }
                    if (flipperScreenImage != null && lastScreenBitmap != null)
                        flipperScreenImage.setAlpha(0.28f);
                    command(WeaveService.ACTION_RECOVER_SCREEN);
                    handler.removeCallbacks(homeScreenRefreshRetry);
                    handler.postDelayed(homeScreenRefreshRetry, 4900);
                } else {
                    if (activeBadgeJob() == null &&
                            prefs.getBoolean(WeaveService.KEY_ENABLED, false))
                        command(WeaveService.ACTION_SCAN);
                    handler.postDelayed(() -> {
                        if (gestureRefreshPage == HOME) finishGestureRefresh();
                    }, 850);
                }
                refresh();
                break;
            case SETTINGS:
                // SCAN refreshes real device information when connected and
                // discovers devices when disconnected (same service path as Home).
                if (activeBadgeJob() == null &&
                        prefs.getBoolean(WeaveService.KEY_ENABLED, false) && !flipperIsConnected())
                    command(WeaveService.ACTION_SCAN);
                refresh();
                if (settingsInfoOpen) {
                    // Info is a separate page: don't check unrelated update cards.
                    handler.postDelayed(() -> {
                        if (gestureRefreshActive && gestureRefreshPage == SETTINGS &&
                                gestureRefreshToken == refreshToken) finishGestureRefresh();
                    }, 850);
                } else {
                    boolean needsFirmwareVersion = flipperIsConnected();
                    gestureSettingsFirmwarePending = needsFirmwareVersion;
                    gestureSettingsChecksPending = needsFirmwareVersion ? 3 : 2;
                    // Refresh both GitHub assets, even if an automatic check is
                    // already running, plus ask the Flipper for its installed
                    // version over the currently active USB/BLE RPC transport.
                    android.util.Log.d("WeaveUpdates", "Manual Settings refresh: App + Firmware");
                    if (needsFirmwareVersion) requestInstalledFirmwareVersion();
                    checkUpdate(UPDATE_APP, () -> completeSettingsGestureCheck(refreshToken), true);
                    checkUpdate(UPDATE_FIRMWARE, () -> completeSettingsGestureCheck(refreshToken), true);
                }
                break;
            default:
                finishGestureRefresh();
                break;
        }
    }

    private void completeSettingsGestureCheck(long token) {
        if (!gestureRefreshActive || gestureRefreshPage != SETTINGS ||
                settingsInfoOpen || gestureRefreshToken != token) return;
        if (gestureSettingsChecksPending > 0 && --gestureSettingsChecksPending == 0)
            finishGestureRefresh();
    }

    private void completeSettingsFirmwareRead() {
        if (!gestureSettingsFirmwarePending) return;
        gestureSettingsFirmwarePending = false;
        completeSettingsGestureCheck(gestureRefreshToken);
    }

    private void requestInstalledFirmwareVersion() {
        if (!flipperIsConnected() || FirmwareUpdatePolicy.activePhase(prefs.getString("firmware_update_phase", ""))) {
            completeSettingsFirmwareRead();
            return;
        }
        firmwareVersionReadPending = true;
        firmwareVersionReadFailed = false;
        prefs.edit().remove(WeaveService.KEY_FIRMWARE).apply();
        UpdateControl control = updateControls[UPDATE_FIRMWARE];
        control.state = "waiting_version";
        control.error = "";
        drawUpdateControl(UPDATE_FIRMWARE);
        handler.removeCallbacks(firmwareVersionReadTimeout);
        handler.postDelayed(firmwareVersionReadTimeout, 20000);
        try {
            Intent request = new Intent(this, WeaveService.class)
                    .setAction(WeaveService.ACTION_REFRESH_DEVICE_VERSION);
            startService(request);
        } catch (RuntimeException error) {
            firmwareVersionReadPending = false;
            firmwareVersionReadFailed = true;
            handler.removeCallbacks(firmwareVersionReadTimeout);
            control.state = "unknown";
            control.error = "Unable to query the connected Flipper";
            drawUpdateControl(UPDATE_FIRMWARE);
            completeSettingsFirmwareRead();
        }
    }

    private void refreshCurrentAppDetail(long token) {
        AppCatalogClient.Entry original = appDetailEntry;
        if (original == null) { finishGestureRefresh(); return; }
        if (appCatalogClient == null) appCatalogClient = new AppCatalogClient(this);
        final String id = normalizedAppId(original.id);
        // Refresh just this app's published metadata/description, not the
        // catalogue, active search, app inventory or other app cards.
        detailRefreshEpochs.put(id, detailRefreshEpochs.getOrDefault(id, 0) + 1);
        detailMetadataRequests.remove(id);
        detailMetadataDone.remove(id);
        detailDescriptionPending.remove(id);
        detailDescriptionDone.remove(id);
        detailDescriptions.remove(id);
        appCatalogClient.invalidateDescription(id);
        java.util.function.Consumer<AppCatalogClient.Entry> onRefreshed = updated -> {
            if (isFinishing() || isDestroyed() || !gestureRefreshActive ||
                    gestureRefreshPage != APPS || gestureRefreshToken != token ||
                    !id.equals(gestureRefreshAppId)) return;
            if (appDetailEntry != null && id.equals(normalizedAppId(appDetailEntry.id))) {
                if (updated != null && id.equals(normalizedAppId(updated.id))) {
                    // The refreshed entry is also visible when leaving this detail.
                    for (int i = 0; i < catalogApps.size(); i++) {
                        if (id.equals(normalizedAppId(catalogApps.get(i).id))) {
                            catalogApps.set(i, updated); break;
                        }
                    }
                    for (int i = 0; i < catalogSearchResults.size(); i++) {
                        if (id.equals(normalizedAppId(catalogSearchResults.get(i).id))) {
                            catalogSearchResults.set(i, updated); break;
                        }
                    }
                    appDetailEntry = updated;
                }
                int offset = appsScrollView == null ? 0 : appsScrollView.getScrollY();
                renderAppDetail(appDetailEntry);
                if (appsScrollView != null) appsScrollView.post(() -> {
                    if (appDetailEntry != null && id.equals(normalizedAppId(appDetailEntry.id)))
                        appsScrollView.scrollTo(0, offset);
                });
            }
            finishGestureRefresh();
        };
        if (isCatalogEntry(original)) appCatalogClient.enrichMetadata(original, onRefreshed);
        else appCatalogClient.loadInstalledMetadata(original.id, onRefreshed);
    }

    private void finishGestureRefresh() {
        handler.removeCallbacks(gestureRefreshFailsafe);
        handler.removeCallbacks(homeScreenRefreshRetry);
        homeScreenRefreshRequestedAt = 0;
        gestureRefreshActive = false;
        gestureRefreshPage = -1;
        gestureFilesExpectedPath = null;
        gestureRefreshAppId = null;
        gestureSettingsChecksPending = 0;
        gestureSettingsFirmwarePending = false;
        if (gestureRefreshAnimator != null) gestureRefreshAnimator.cancel();
        gestureRefreshAnimator = null;
        if (gestureRefreshIndicator != null) {
            gestureRefreshIndicator.animate().cancel();
            gestureRefreshIndicator.animate().alpha(0f).translationY(dp(-12))
                    .setDuration(170).withEndAction(() -> {
                        if (!gestureRefreshActive) hideGestureRefreshPreview();
                    }).start();
        }
    }

    private InstallRingView installRing;
    private String ringAppId;

    // Le badge suit les jobs RPC persistants, meme apres changement de tab.
    private final class InstallRingView extends View {
        private final Paint pen = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF arc = new RectF();
        private int percent = -1;
        private String phase = "";
        InstallRingView(Context context) {
            super(context);
            setOnClickListener(v -> openRingInstall());
        }
        void showJob(String name, String step, int value) {
            phase = step;
            percent = value;
            setContentDescription("Installing " + name + ", " + step +
                    (value < 0 ? "" : ", " + value + "%") + ". Open application");
            invalidate();
        }
        @Override protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float width = getWidth(), height = getHeight();
            float cx = width / 2f, cy = height / 2f;
            float radius = Math.min(width, height) / 2f - dp(6);
            pen.setStyle(Paint.Style.FILL);
            pen.setColor(CARD);
            canvas.drawCircle(cx, cy, radius + dp(5), pen);
            pen.setColor(BORDER);
            pen.setStyle(Paint.Style.STROKE);
            pen.setStrokeWidth(dp(4));
            canvas.drawCircle(cx, cy, radius, pen);
            pen.setColor(ORANGE);
            pen.setStrokeCap(Paint.Cap.ROUND);
            arc.set(cx - radius, cy - radius, cx + radius, cy + radius);
            float sweep = percent >= 0 ? 360f * Math.max(0, Math.min(100, percent)) / 100f : 76f;
            float start = percent >= 0 ? -90f : ((android.os.SystemClock.uptimeMillis() / 7) % 360) - 90f;
            canvas.drawArc(arc, start, sweep, false, pen);
            pen.setStyle(Paint.Style.FILL);
            pen.setTypeface(Typeface.create(Typeface.DEFAULT, Typeface.BOLD));
            pen.setTextAlign(Paint.Align.CENTER);
            pen.setColor(WHITE);
            pen.setTextSize(dp(percent >= 0 ? 12 : 23));
            canvas.drawText(percent < 0 ? "↓" : percent + "%", cx,
                    cy - (pen.ascent() + pen.descent()) / 2f, pen);
            if (percent < 0 && getVisibility() == VISIBLE) postInvalidateDelayed(70);
        }
    }

    // Mini ring local : chaque carte suit son propre job sans navigation obligatoire.
    private final class CardInstallRingView extends View {
        private final Paint pen = new Paint(Paint.ANTI_ALIAS_FLAG);
        private final RectF arc = new RectF();
        private final String appId;
        private int percent = -1;
        CardInstallRingView(String id) {
            super(MainActivity.this);
            appId = id;
            setContentDescription("Installation " + id);
        }
        void update() {
            org.json.JSONObject local = catalogQueueInfo(appId);
            org.json.JSONObject nativeState = nativeShopInfo(appId);
            percent = nativeState != null && ("downloading".equals(nativeState.optString("phase")) ||
                    "installing".equals(nativeState.optString("phase"))) ?
                    ("downloading".equals(nativeState.optString("phase")) ?
                            nativeState.optInt("percent", -1) : -1) : local != null ?
                    ("downloading".equals(local.optString("phase")) ?
                            local.optInt("percent", -1) : -1) : -1;
            invalidate();
        }
        @Override protected void onDraw(Canvas canvas) {
            super.onDraw(canvas);
            float cx = getWidth() / 2f, cy = getHeight() / 2f;
            float radius = Math.min(cx, cy) - dp(4);
            pen.setStyle(Paint.Style.STROKE);
            pen.setStrokeWidth(dp(3));
            pen.setStrokeCap(Paint.Cap.ROUND);
            pen.setColor(BORDER);
            canvas.drawCircle(cx, cy, radius, pen);
            pen.setColor(ORANGE);
            arc.set(cx - radius, cy - radius, cx + radius, cy + radius);
            float sweep = percent < 0 ? 85f : 360f * Math.max(0, Math.min(100, percent)) / 100f;
            float start = percent < 0 ? ((android.os.SystemClock.uptimeMillis() / 7) % 360) - 90f : -90f;
            canvas.drawArc(arc, start, sweep, false, pen);
            if (percent < 0 && getVisibility() == VISIBLE && getParent() != null)
                postInvalidateDelayed(65);
        }
    }

    private void updateVisibleAppInstallProgress() {
        for (CardInstallRingView ring : visibleAppInstallRings.values())
            if (ring.getParent() != null) ring.update();
    }

    // Shared refresh entry point: toolbar and overscroll must behave identically.
    // Never launch overlapping inventory / catalog requests.
    private void requestFullAppsRefresh() {
        if (appsRefreshInProgress || catalogRequestInFlight) return;
        ++catalogSearchGeneration;
        if (appCatalogClient != null) appCatalogClient.cancelSearch();
        startAppsRefreshAnimation();
        if (flipperIsConnected() && activeBadgeJob() == null) {
            prefs.edit().putString(WeaveService.KEY_INVENTORY_STATUS, "loading").apply();
            startService(new Intent(this, WeaveService.class)
                    .setAction(WeaveService.ACTION_REFRESH_APPS)
                    .putExtra("forceMetadata", true));
        } else appsRefreshInventoryDone = true;
        requestCatalog(true);
    }

    private void startAppsRefreshIconOnly() {
        if (appsRefreshAnimator != null) appsRefreshAnimator.cancel();
        if (appsRefreshIcon == null) return;
        appsRefreshAnimator = ObjectAnimator.ofFloat(appsRefreshIcon, View.ROTATION, 0f, 360f);
        appsRefreshAnimator.setDuration(850);
        appsRefreshAnimator.setRepeatCount(ObjectAnimator.INFINITE);
        appsRefreshAnimator.setInterpolator(new LinearInterpolator());
        appsRefreshAnimator.start();
    }

    private void startAppsRefreshAnimation() {
        appsRefreshInProgress = true;
        appsRefreshCatalogDone = false;
        appsRefreshInventoryDone = !flipperIsConnected();
        startAppsRefreshIconOnly();
        handler.removeCallbacks(refreshFailsafe);
        handler.postDelayed(refreshFailsafe, 30000);
    }

    private void stopAppsRefreshAnimation() {
        appsRefreshInProgress = false;
        handler.removeCallbacks(refreshFailsafe);
        if (appsRefreshAnimator != null) appsRefreshAnimator.cancel();
        appsRefreshAnimator = null;
        if (appsRefreshIcon != null) appsRefreshIcon.setRotation(0f);
        if (gestureRefreshActive && gestureRefreshPage == APPS &&
                gestureRefreshAppId == null) finishGestureRefresh();
    }

    private void checkAppsRefreshCompletion() {
        if (appsRefreshInProgress && appsRefreshCatalogDone && appsRefreshInventoryDone) {
            stopAppsRefreshAnimation();
            // A simultaneous search must never compete with a full catalog refresh.
            if (selectedPage == APPS && appsMode == 0 &&
                    catalogSearchTerm.length() >= 2) {
                handler.removeCallbacks(delayedCatalogSearch);
                handler.postDelayed(delayedCatalogSearch, 350);
            }
            // Native catalog synchronization is intentionally AFTER foreground
            // refresh and cannot steal RPC bandwidth from an installation.
            if (activeBadgeJob() == null) handler.postDelayed(delayedShopIconSync, 600);
        }
    }

    private static final class InstallBadgeJob {
        final String id, title, phase;
        final int percent;
        InstallBadgeJob(String id, String title, String phase, int percent) {
            this.id = id; this.title = title; this.phase = phase; this.percent = percent;
        }
    }

    private InstallBadgeJob activeBadgeJob() {
        if (prefs == null) return null;
        try {
            org.json.JSONObject nativeJob = new org.json.JSONObject(
                    prefs.getString(WeaveService.KEY_SHOP_JOB, "{}"));
            String phase = nativeJob.optString("phase", "");
            long nativeAge = System.currentTimeMillis() - nativeJob.optLong("updatedAt", 0);
            if (("downloading".equals(phase) || "installing".equals(phase) ||
                    "queued".equals(phase)) && nativeAge >= 0 && nativeAge < 900000 &&
                    !nativeJob.optString("id", "").isEmpty())
                return new InstallBadgeJob(nativeJob.optString("id"),
                        nativeJob.optString("title", nativeJob.optString("id")),
                        phase, "downloading".equals(phase) ?
                                nativeJob.optInt("percent", -1) : -1);
            org.json.JSONArray jobs = new org.json.JSONArray(
                    prefs.getString(WeaveService.KEY_CATALOG_QUEUE, "[]"));
            for (int i = 0; i < jobs.length(); i++) {
                org.json.JSONObject job = jobs.optJSONObject(i);
                if (job == null || job.optString("id", "").isEmpty()) continue;
                return new InstallBadgeJob(job.optString("id"),
                        job.optString("title", job.optString("id")),
                        job.optString("phase", "queued"),
                        "downloading".equals(job.optString("phase")) ?
                                job.optInt("percent", -1) : -1);
            }
        } catch (org.json.JSONException ignored) { }
        if (activeInstallId != null && !activeInstallId.startsWith("manual:"))
            return new InstallBadgeJob(activeInstallId, activeInstallId,
                    "installing", appProgressOf(activeInstallId));
        return null;
    }

    /** Prevent remote key presses while an app install occupies the Flipper screen.
     *  Reuse the job detection used by the mirrored installation display so
     *  its lock is released as soon as that installation completes.
     */
    private boolean remotePadLocked() {
        return activeBadgeJob() != null;
    }

    /* The Home screen always represents REAL device framebuffer data.
     * Previous versions painted a local imitation of the native install view:
     * it could show progress while the physical Flipper still showed its home
     * screen and it concealed RPC stream stalls. Never synthesize that image. */
    private void updateRemoteDisplayTransferState() {
        if (flipperScreenStatus == null || flipperScreenImage == null) return;
        InstallBadgeJob job = activeBadgeJob();
        if (job != null) {
            screenWasLockedByInstall = true;
            // The BLE RPC stream is intentionally suspended during large
            // storage uploads. Keep its last confirmed device frame visibly
            // stale instead of claiming a phone-rendered image is live.
            long age = android.os.SystemClock.uptimeMillis() - lastRealScreenFrameAt;
            if (lastScreenBitmap == null || lastRealScreenFrameAt == 0 || age > 2500) {
                flipperScreenStatus.setText("Live screen paused during transfer");
                flipperScreenStatus.setVisibility(View.VISIBLE);
                if (lastScreenBitmap != null) flipperScreenImage.setAlpha(0.30f);
            } else {
                flipperScreenStatus.setVisibility(View.GONE);
                flipperScreenImage.setAlpha(1f);
            }
            return;
        }
        if (screenWasLockedByInstall) {
            screenWasLockedByInstall = false;
            screenAwaitingFreshFrame = true;
            flipperScreenStatus.setText("Reconnecting live Flipper screen…");
            flipperScreenStatus.setVisibility(View.VISIBLE);
            if (lastScreenBitmap != null) flipperScreenImage.setAlpha(0.30f);
            if (flipperIsConnected() && appForeground && selectedPage == HOME)
                command(WeaveService.ACTION_START_SCREEN);
        }
    }

    private void updateInstallRing() {
        if (installRing == null) return;
        InstallBadgeJob job = activeBadgeJob();
        ringAppId = job == null ? null : job.id;
        boolean showingDetail = job != null && selectedPage == APPS && appDetailEntry != null &&
                normalizedAppId(job.id).equals(normalizedAppId(appDetailEntry.id));
        installRing.setVisibility(job == null || showingDetail ? View.GONE : View.VISIBLE);
        if (job != null && !showingDetail)
            installRing.showJob(job.title, job.phase, job.percent);
    }

    private void openRingInstall() {
        InstallBadgeJob job = activeBadgeJob();
        if (job == null) return;
        selectPage(APPS);
        for (List<AppCatalogClient.Entry> list :
                java.util.Arrays.asList(catalogApps, catalogSearchResults)) {
            for (AppCatalogClient.Entry entry : list) {
                if (normalizedAppId(job.id).equals(normalizedAppId(entry.id))) {
                    openAppDetail(entry);
                    updateInstallRing();
                    return;
                }
            }
        }
        if (catalogApps.isEmpty()) requestCatalog(false);
        toast("Installation in progress: " + job.title);
    }
    private boolean flipperEventsRegistered;
    private String requestedFilesPath, lastFilesError;
    private String activeInstallId;
    private String pendingUninstallId;
    private boolean pendingUninstall;
    private byte[] exportedFile;
    private String exportedFileName;
    // Home controls and display scale as a group to fit typical phone heights.
    private int homePadDp = 60;
    private int homeDisplayMaxWidthDp = 410;
    private boolean compactHome;
    private boolean tinyHome;
    private ImageView batteryIconHome;
    private TextView batteryValueHome, batteryDetailHome, btValueHome, usbValueHome;
    private ArchiveExportSession archiveExportSession;
    private LinearLayout appsListHost;
    private LinearLayout appsStickyBar;
    private ScrollView appsScrollView;
    private AppCatalogClient.Entry appDetailEntry;
    private AppCatalogClient appCatalogClient;
    private final List<AppCatalogClient.Entry> catalogApps = new ArrayList<>();
    private final List<AppCatalogClient.Entry> catalogSearchResults = new ArrayList<>();
    private String catalogSearchTerm = "";
    private boolean catalogSearchLoading;
    private String catalogSearchError;
    private boolean catalogLoading = true;
    private boolean catalogRequestInFlight;
    private String catalogError;
    private boolean appsLastLinkState;
    private EditText appsSearchInput;
    private TextView appsFilterLabel;
    private ImageButton appsFilterButton;
    private TextView[] appsModeTabs;
    private ImageView appsRefreshIcon;
    private ObjectAnimator appsRefreshAnimator;
    private boolean appsRefreshInProgress, appsRefreshCatalogDone, appsRefreshInventoryDone;
    // Reconcile missed Service broadcasts without rebuilding app cards every second.
    private String lastDetailInstallSnapshot = "";
    private final Map<String, CardInstallRingView> visibleAppInstallRings = new HashMap<>();
    private final Runnable refreshFailsafe = this::stopAppsRefreshAnimation;
    private int appsMode = 0;
    private int appsVisibleLimit = 40;
    private String appsFilterCategory = "All";
    private int appsSortMode = 0;
    private final Map<String, Integer> appStates = new HashMap<>();
    private final Map<String, Integer> appProgress = new HashMap<>();
    private ProgressBar detailInstallBar;
    private TextView detailInstallPercent;
    private TextView detailInstallStatus;
    private String detailInstallId;
    private final Map<String, Runnable> appDownloads = new HashMap<>();
    private String appsLastInventorySignature = "";
    private int catalogRequestGeneration;
    private int catalogSearchGeneration;
    private boolean shopCatalogSyncedThisConnection;
    private int shopCatalogSyncHash;
    private long lastShopIconSyncAt;
    private final Runnable delayedShopIconSync = () -> {
        if (flipperIsConnected() && !catalogApps.isEmpty()) {
            lastShopIconSyncAt = System.currentTimeMillis();
            syncShopCatalogToFlipper();
        }
    };
    private final Runnable delayedCatalogSearch = this::requestCatalogSearch;
    private int selectedPage = HOME;
    private final Runnable delayedAppsFilter = () -> {
        if (selectedPage == APPS && appDetailEntry == null) renderAppsList();
    };
    private final FilesBrowserState filesState = new FilesBrowserState();
    private LinearLayout filesToolbar, filesList;
    private EditText filesSearchInput;
    private TextView filesSearchClear;
    private String filesSearchTerm = "";
    private FilesGlobalSearch filesGlobalSearch;
    private String filesSearchRevealName;
    private final Runnable delayedFilesSearch = this::startFilesGlobalSearch;
    private final Runnable nextFilesSearchStep = this::requestNextFilesSearchFolder;
    private final Runnable filesSearchFolderTimeout = () -> {
        FilesGlobalSearch search = filesGlobalSearch;
        if (search == null || search.waitingFor() == null) return;
        search.accept(search.waitingFor(), false, null, null, null);
        if (selectedPage == FILES) updateFilesPanel();
        handler.postDelayed(nextFilesSearchStep, 70);
    };
    private TextView filesStorageName, filesStorageUsage, filesNotice;
    private ProgressBar filesStorageBar;
    private final TextView[] filesTools = new TextView[8];
    private boolean filesLastConnected;
    private boolean filesPanelReady;
    private final java.util.ArrayDeque<String> pasteSources = new java.util.ArrayDeque<>();
    private final java.util.ArrayDeque<String> pasteTargets = new java.util.ArrayDeque<>();
    private String pasteDestination, pasteReadSource, pasteWriteTarget;
    private boolean pasteRunning, pasteIsMove;
    private final Set<String> metadataLookups = new HashSet<>();
    private final Map<String, AppCatalogClient.Entry> installedMetadata = new HashMap<>();
    private final Map<String, Bitmap> nativeFapIcons = new HashMap<>();
    private long lastInventoryUiRequestAt;
    private final Set<String> detailMetadataRequests = new HashSet<>();
    private final Set<String> detailMetadataDone = new HashSet<>();
    // Ignore an older metadata/README callback after an explicit single-app refresh.
    private final Map<String, Integer> detailRefreshEpochs = new HashMap<>();
    private final Set<String> detailDescriptionPending = new HashSet<>();
    private final Set<String> detailDescriptionDone = new HashSet<>();
    private final Map<String, String> detailDescriptions = new HashMap<>();
    private Switch homeService, serviceSwitch, bootSwitch;
    private FrameLayout settingsHost;
    private AlertDialog findDeviceDialog;
    private AlertDialog flipperKeyboardDialog;
    private String flipperKeyboardToken;
    private Runnable flipperKeyboardPreviewTask;
    private String flipperKeyboardIgnoredToken;
    // Only show the finder over Weave's own foreground UI. Android background
    // activity-launch restrictions require a notification tap to bring us back.
    private boolean appForeground;
    private boolean settingsInfoOpen;
    private boolean settingsLastConnected;
    private String settingsLastTransport = "";
    private String settingsLastFirmware = "";
    // Update buttons are the sole status indicator; the orange surface doubles as progress.
    private static final int UPDATE_APP = 0, UPDATE_FIRMWARE = 1;
    private static final class UpdateControl {
        String state = "unknown", error = "", pendingVersion = "";
        long lastCheck;
        int percent;
        boolean busy;
        boolean checkingRelease;
        int releaseCheckEpoch;
        String firmwarePhase = "";
        final List<Runnable> pendingCheckListeners = new ArrayList<>();
        WeaveUpdates.Release release;
        TextView versionView, buttonText;
        ImageView statusIcon;
        FrameLayout progressTrack;
        View progressFill;
    }
    private final UpdateControl[] updateControls = {new UpdateControl(), new UpdateControl()};
    private boolean firmwareVersionReadPending;
    private boolean firmwareVersionReadFailed;
    private final Runnable firmwareVersionReadTimeout = () -> {
        if (!firmwareVersionReadPending) return;
        firmwareVersionReadPending = false;
        UpdateControl control = updateControls[UPDATE_FIRMWARE];
        if (updateCurrentVersion(UPDATE_FIRMWARE) == null) {
            firmwareVersionReadFailed = true;
            control.state = "unknown";
            control.error = "Flipper firmware version RPC timed out";
        }
        drawUpdateControl(UPDATE_FIRMWARE);
        completeSettingsFirmwareRead();
    };
    private long lastServiceRestart;
    private boolean blePermissionPromptedThisSession;
    private Button usbButton, bleScanButton, bleConnectButton;
    private boolean syncing, receiverRegistered;
    private UsbDevice pendingUsb;
    private boolean pendingCliPermission;

    private final Runnable refresher = new Runnable() {
        @Override public void run() {
            if (!appForeground) return;
            refresh();
            handler.postDelayed(this, PageWorkPolicy.refreshDelayMillis(
                    selectedPage, appsRefreshInProgress || activeBadgeJob() != null));
        }
    };
    private final BroadcastReceiver deviceEvents = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            if (intent == null || intent.getAction() == null) return;
            switch (intent.getAction()) {
                case WeaveService.EVENT_FAP_METADATA:
                    nativeFapIcons.clear();
                    if (selectedPage == APPS) renderAppsList();
                    break;
                case WeaveService.EVENT_SCREEN:
                    // A STOP can race with a frame already broadcast over BLE.
                    // No offscreen decoding, allocation or stale Home updates.
                    if (!appForeground || selectedPage != HOME) break;
                    byte[] pixels = intent.getByteArrayExtra("pixels");
                    if (pixels == null || pixels.length != 1024) return;
                    // Only render actual PB_Gui.ScreenFrame bytes received from
                    // the Flipper. Alternating bitmaps also forces ImageView to
                    // replace its hardware display list on each new frame.
                    for (int y = 0; y < 64; y++) for (int x = 0; x < 128; x++) {
                        int page = (y / 8) * 128 + x;
                        screenPixelBuffer[y * 128 + x] =
                                (pixels[page] & (1 << (y % 8))) != 0 ? 0xff30210a : 0xffffa323;
                    }
                    int slot = nextScreenBitmap++ & 1;
                    Bitmap realFrame = screenBitmaps[slot];
                    if (realFrame == null || realFrame.isRecycled()) {
                        realFrame = Bitmap.createBitmap(128, 64, Bitmap.Config.ARGB_8888);
                        screenBitmaps[slot] = realFrame;
                    }
                    realFrame.setPixels(screenPixelBuffer, 0, 128, 0, 0, 128, 64);
                    lastScreenBitmap = realFrame;
                    lastRealScreenFrameAt = android.os.SystemClock.uptimeMillis();
                    screenAwaitingFreshFrame = false;
                    screenRecoveryFailed = false;
                    if (gestureRefreshActive && gestureRefreshPage == HOME &&
                            homeScreenRefreshRequestedAt > 0 &&
                            lastRealScreenFrameAt >= homeScreenRefreshRequestedAt) {
                        // Only a device frame (never the cached Bitmap) ends refresh.
                        finishGestureRefresh();
                    }
                    if (flipperScreenImage != null) {
                        BitmapDrawable sharp = new BitmapDrawable(getResources(), realFrame);
                        sharp.setFilterBitmap(false);
                        sharp.setDither(false);
                        sharp.setAntiAlias(false);
                        flipperScreenImage.setImageDrawable(sharp);
                        flipperScreenImage.setAlpha(1f);
                        flipperScreenImage.setVisibility(View.VISIBLE);
                    }
                    if (flipperScreenStatus != null)
                        flipperScreenStatus.setVisibility(View.GONE);
                    break;
                case WeaveService.EVENT_STORAGE_INFO:
                    if (selectedPage == FILES) updateFilesStorageUsage();
                    break;
                case WeaveService.EVENT_FILES:
                    String path = intent.getStringExtra("path");
                    boolean ok = intent.getBooleanExtra("ok", false);
                    if (archiveExportSession != null && path != null && path.equals(archiveExportSession.awaitingListPath)) {
                        onArchiveListing(path, ok, intent.getStringArrayExtra("names"),
                                intent.getBooleanArrayExtra("directories"), intent.getLongArrayExtra("sizes"));
                        break;
                    }
                    if (receiveFilesSearchListing(intent, path, ok)) break;
                    // A stale directory listing never rebuilds a hidden Files page.
                    // File export/paste and search responses are handled above.
                    if (selectedPage != FILES) break;
                    // A previous browse response must not interrupt an active search.
                    if (!filesSearchTerm.isEmpty()) break;
                    requestedFilesPath = null;
                    if (path == null || !path.equals(filesState.path)) return;
                    if (gestureRefreshActive && gestureRefreshPage == FILES &&
                            path.equals(gestureFilesExpectedPath)) finishGestureRefresh();
                    if (!ok) {
                        lastFilesError = intent.getStringExtra("message");
                    } else {
                        lastFilesError = null;
                        String[] names = intent.getStringArrayExtra("names");
                        boolean[] directories = intent.getBooleanArrayExtra("directories");
                        long[] sizes = intent.getLongArrayExtra("sizes");
                        List<FilesBrowserState.Entry> verified = new ArrayList<>();
                        if (names != null && directories != null && sizes != null) {
                            for (int i = 0; i < names.length && i < directories.length && i < sizes.length; i++) {
                                if (FilesBrowserState.validName(names[i]))
                                    verified.add(new FilesBrowserState.Entry(names[i], directories[i], sizes[i]));
                            }
                            filesState.setVerifiedListing(filesState.storage, path, verified);
                            if (filesSearchRevealName != null) {
                                filesState.toggle(filesSearchRevealName);
                                filesSearchRevealName = null;
                            }
                        }
                    }
                    if (selectedPage == FILES) updateFilesPanel();
                    break;
                case WeaveService.EVENT_TEXT_INPUT:
                    if(intent.getBooleanExtra("visible", false)) {
                        if(appForeground) showFlipperKeyboardIfActive();
                    } else if(flipperKeyboardDialog != null && flipperKeyboardDialog.isShowing()) {
                        flipperKeyboardDialog.dismiss();
                    }
                    break;
                case WeaveService.EVENT_FIND_DEVICE:
                    if(intent.getBooleanExtra("ringing", false)) {
                        // This event is delivered regardless of which Weave page is open.
                        // Do not launch an Activity over other apps in the background.
                        if(appForeground) showFindDeviceDialogIfRinging();
                    } else if(findDeviceDialog != null && findDeviceDialog.isShowing()) {
                        findDeviceDialog.dismiss();
                    }
                    break;
                case WeaveService.EVENT_SHOP_PROGRESS:
                    handler.post(MainActivity.this::updateInstallRing);
                    if (selectedPage == HOME)
                        handler.post(MainActivity.this::updateRemoteDisplayTransferState);
                    if (selectedPage == APPS)
                        handler.post(MainActivity.this::updateVisibleAppInstallProgress);
                    String nativeId = intent.getStringExtra("id");
                    String nativePhase = intent.getStringExtra("phase");
                    int nativePercent = intent.getIntExtra("percent", -1);
                    long nativeEta = intent.getLongExtra("eta", -1);
                    if (selectedPage == APPS && appDetailEntry != null && nativeId != null &&
                            normalizedAppId(nativeId).equals(normalizedAppId(appDetailEntry.id))) {
                        if ("done".equals(nativePhase) || "failed".equals(nativePhase))
                            renderAppDetail(appDetailEntry);
                        else if (detailInstallBar == null || detailInstallId == null ||
                                !nativeId.equals(detailInstallId)) renderAppDetail(appDetailEntry);
                        else {
                            detailInstallBar.setIndeterminate(nativePercent < 0 ||
                                    !"downloading".equals(nativePhase));
                            if(nativePercent >= 0 && "downloading".equals(nativePhase))
                                detailInstallBar.setProgress(nativePercent);
                            if(detailInstallStatus != null) detailInstallStatus.setText(
                                    "installing".equals(nativePhase) ?
                                            "Finishing on Flipper…" : "Downloading FAP…");
                            if(detailInstallPercent != null) detailInstallPercent.setText(
                                    nativePercent < 0 || !"downloading".equals(nativePhase) ?
                                    "Finishing on Flipper…" :
                                    nativePercent + "%" + (nativeEta > 0 ? String.format(Locale.US,
                                            " · ETA %02d:%02d", nativeEta / 60, nativeEta % 60) : ""));
                        }
                    } else if(selectedPage == APPS && appDetailEntry == null &&
                            ("done".equals(nativePhase) || "failed".equals(nativePhase)))
                        renderAppsList();
                    break;
                case WeaveService.EVENT_CATALOG_QUEUE:
                    handler.post(MainActivity.this::updateInstallRing);
                    if (selectedPage == HOME)
                        handler.post(MainActivity.this::updateRemoteDisplayTransferState);
                    if (selectedPage == APPS)
                        handler.post(MainActivity.this::updateVisibleAppInstallProgress);
                    String changedAppId = intent.getStringExtra("id");
                    String changeKind = intent.getStringExtra("change");
                    if (!"progress".equals(changeKind) && selectedPage == APPS) {
                        if (appDetailEntry != null) renderAppDetail(appDetailEntry);
                        else renderAppsList();
                    } else if (selectedPage == APPS && "progress".equals(changeKind) &&
                            changedAppId != null && changedAppId.equals(detailInstallId)) {
                        int percentNow = appProgressOf(changedAppId);
                        if (detailInstallBar != null) {
                            detailInstallBar.setIndeterminate(percentNow < 0 || percentNow >= 100);
                            if (percentNow >= 0 && percentNow < 100) detailInstallBar.setProgress(percentNow);
                        }
                        if (detailInstallPercent != null) detailInstallPercent.setText(
                                percentNow < 0 ? "Downloading…" : percentNow + "%");
                    }
                    break;
                case WeaveService.EVENT_OPERATION:
                    handler.post(MainActivity.this::updateInstallRing);
                    String operation = intent.getStringExtra("operation");
                    boolean success = intent.getBooleanExtra("ok", false);
                    String error = intent.getStringExtra("message");
                    if (pasteRunning && ((pasteIsMove && "Rename".equals(operation)) ||
                            (!pasteIsMove && "Import".equals(operation)))) {
                        if (success) {
                            pasteSources.poll();
                            pasteTargets.poll();
                            pasteWriteTarget = null;
                            runNextPaste();
                        } else {
                            pasteRunning = false;
                            pasteReadSource = null;
                            pasteWriteTarget = null;
                            toast(error == null ? "Flipper refused paste" : error);
                        }
                        break;
                    }
                    if ("Install".equals(operation) && activeInstallId != null) {
                        // Do not infer installation from a queued file transfer. Re-read SD after RPC acknowledgement.
                        String installedId = activeInstallId;
                        appProgress.remove(installedId);
                        activeInstallId = null;
                        appStates.remove(installedId);
                        if (success) requestInstalledApps();
                        if (selectedPage == APPS) renderAppsList();
                    }
                    if ("Delete".equals(operation) && pendingUninstall) {
                        pendingUninstall = false;
                        String removedId = pendingUninstallId;
                        pendingUninstallId = null;
                        if (success) {
                            appStates.remove(removedId);
                            // A removed .fap must not inherit a stale published-version record
                            // if another copy later occupies the same path.
                            try {
                                org.json.JSONObject recorded = new org.json.JSONObject(prefs.getString(
                                        WeaveService.KEY_CATALOG_INSTALLED_VERSIONS, "{}"));
                                recorded.remove(normalizedAppId(removedId));
                                prefs.edit().putString(WeaveService.KEY_CATALOG_INSTALLED_VERSIONS,
                                        recorded.toString()).apply();
                            } catch (org.json.JSONException ignored) { }
                            requestInstalledApps();
                        }
                        if (selectedPage == APPS) renderAppsList();
                    }
                    if (operation != null && operation.startsWith("Shop ")) break;
                    toast(success ? operation + " confirmed by Flipper"
                            : (error == null ? "Flipper " + operation + " failed" : error));
                    if (success && selectedPage == FILES) {
                        filesState.invalidateListing();
                        requestCurrentFiles();
                    }
                    break;
                case WeaveService.EVENT_FILE_DATA:
                    String fileDataPath = intent.getStringExtra("path");
                    if (archiveExportSession != null && fileDataPath != null && fileDataPath.equals(archiveExportSession.awaitingFilePath)) {
                        onArchiveFile(fileDataPath, intent.getByteArrayExtra("bytes"), intent.getStringExtra("error"));
                        break;
                    }
                    if (pasteRunning && !pasteIsMove && pasteReadSource != null) {
                        byte[] data = intent.getByteArrayExtra("bytes");
                        if (data == null || data.length == 0 || data.length > 2_000_000) {
                            pasteRunning = false;
                            pasteWriteTarget = null;
                            toast("File copy failed: could not read source");
                            break;
                        }
                        String fileName = "upload_copy_" + System.nanoTime() + ".bin";
                        String targetPath = pasteWriteTarget != null ? pasteWriteTarget
                                : pasteDestination + "/" + pasteReadSource.substring(pasteReadSource.lastIndexOf('/') + 1);
                        try (FileOutputStream out = new FileOutputStream(new File(getCacheDir(), fileName))) {
                            out.write(data);
                            startService(new Intent(MainActivity.this, WeaveService.class).setAction(WeaveService.ACTION_UPLOAD_FILE)
                                    .putExtra("filename", fileName).putExtra("path", targetPath));
                            pasteReadSource = null;
                            pasteWriteTarget = null;
                        } catch (Exception e) {
                            pasteRunning = false;
                            pasteWriteTarget = null;
                            toast("File copy failed: unable to prepare transfer");
                        }
                        break;
                    }
                    byte[] downloaded = intent.getByteArrayExtra("bytes");
                    if (downloaded == null) {
                        String reason = intent.getStringExtra("error");
                        toast(reason == null ? "Flipper file export failed" : reason);
                    } else {
                        exportedFile = downloaded;
                        String remote = intent.getStringExtra("path");
                        exportedFileName = remote == null ? "flipper-file.bin"
                                : remote.substring(remote.lastIndexOf('/') + 1);
                        Intent save = new Intent(Intent.ACTION_CREATE_DOCUMENT);
                        save.addCategory(Intent.CATEGORY_OPENABLE);
                        save.setType("application/octet-stream");
                        save.putExtra(Intent.EXTRA_TITLE, exportedFileName);
                        try { startActivityForResult(save, REQUEST_FILE_EXPORT); }
                        catch (ActivityNotFoundException missingFileManager) { toast("No file manager available"); }
                    }
                    break;
                case WeaveService.EVENT_CLI:
                    String segment = intent.getStringExtra("text");
                    if (segment == null) break;
                    if (segment.contains("USB CLI connected")) cliUsbConnected = true;
                    if (segment.contains("USB CLI disconnected")) cliUsbConnected = false;
                    if (segment.contains("BLE CLI ready")) cliBleConnected = true;
                    if (segment.contains("BLE CLI disconnected")) cliBleConnected = false;
                    cliTranscript.append(segment);
                    if (cliTranscript.length() > 24000) cliTranscript.delete(0, cliTranscript.length() - 16000);
                    // Do not rebuild the entire terminal (and discard the focused prompt)
                    // for every incoming BLE chunk or CLI percentage/status update.
                    if (selectedPage == CONSOLE && terminalMode == 0 && cliOutput != null)
                        cliOutput.setText(cliTranscript.toString());
                    if (selectedPage == CONSOLE && terminalMode == 0 &&
                            (segment.contains("BLE CLI ready") || segment.contains("BLE CLI disconnected") ||
                             segment.contains("USB CLI connected") || segment.contains("USB CLI disconnected")))
                        renderTerminalPanel(true);
                    break;
                case WeaveService.EVENT_DEVICE_VERSION:
                    firmwareVersionReadPending = false;
                    handler.removeCallbacks(firmwareVersionReadTimeout);
                    UpdateControl deviceUpdate = updateControls[UPDATE_FIRMWARE];
                    String firmwareError = intent.getStringExtra("error");
                    if (firmwareError != null && updateCurrentVersion(UPDATE_FIRMWARE) == null) {
                        firmwareVersionReadFailed = true;
                        deviceUpdate.state = "unknown";
                        deviceUpdate.error = firmwareError;
                    } else if (updateCurrentVersion(UPDATE_FIRMWARE) != null) {
                        firmwareVersionReadFailed = false;
                        deviceUpdate.error = "";
                        if (deviceUpdate.release != null && !deviceUpdate.busy) {
                            deviceUpdate.state = WeaveUpdates.compare(
                                    updateCurrentVersion(UPDATE_FIRMWARE), deviceUpdate.release.version) < 0
                                            ? "available" : "current";
                        }
                    }
                    if (selectedPage == SETTINGS && !settingsInfoOpen)
                        drawUpdateControl(UPDATE_FIRMWARE);
                    completeSettingsFirmwareRead();
                    break;
                case WeaveService.EVENT_FIRMWARE_UPDATE:
                    String phase = intent.getStringExtra("phase");
                    UpdateControl firmwareUpdate = updateControls[UPDATE_FIRMWARE];
                    firmwareUpdate.firmwarePhase = phase;
                    if (firmwareUpdate.checkingRelease) {
                        ++firmwareUpdate.releaseCheckEpoch;
                        firmwareUpdate.checkingRelease = false;
                        List<Runnable> callbacks = new ArrayList<>(firmwareUpdate.pendingCheckListeners);
                        firmwareUpdate.pendingCheckListeners.clear();
                        for (Runnable callback : callbacks) callback.run();
                    }
                    if ("error".equals(phase)) {
                        firmwareUpdate.busy = false;
                        firmwareUpdate.state = "unknown";
                        firmwareUpdate.error = intent.getStringExtra("message");
                        toast("Firmware update failed: " + (firmwareUpdate.error == null ? "unknown error" : firmwareUpdate.error));
                    } else if ("ready".equals(phase)) {
                        firmwareUpdate.busy = false;
                        firmwareUpdate.state = "current";
                        firmwareUpdate.error = "";
                        firmwareUpdate.lastCheck = 0;
                        if (flipperIsConnected()) checkUpdate(UPDATE_FIRMWARE);
                    } else {
                        firmwareUpdate.busy = true;
                        firmwareUpdate.checkingRelease = false;
                        firmwareUpdate.percent = intent.getIntExtra("percent", 0);
                        firmwareUpdate.state = "installing";
                        firmwareUpdate.error = "";
                    }
                    drawUpdateControl(UPDATE_FIRMWARE);
                    break;
                case WeaveService.EVENT_PROGRESS:
                    handler.post(MainActivity.this::updateInstallRing);
                    if (activeInstallId == null) break;
                    int progress = intent.getIntExtra("percent", 0);
                    progress = Math.max(0, Math.min(100, progress));
                    appProgress.put(activeInstallId, progress);
                    // Keep the detail view (including its Repository/GitHub footer) mounted.
                    // Rebuilding it on every progress event caused layout shaking and image reloads.
                    if (selectedPage == APPS && activeInstallId.equals(detailInstallId)) {
                        if (detailInstallBar != null) detailInstallBar.setProgress(progress);
                        if (detailInstallPercent != null) detailInstallPercent.setText(progress + "%");
                    }
                    break;
            }
        }
    };

    private final BroadcastReceiver usbPermissions = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            if (!USB_PERMISSION.equals(intent.getAction())) return;
            UsbDevice device = pendingUsb;
            pendingUsb = null;
            boolean cli = pendingCliPermission;
            pendingCliPermission = false;
            if (device != null && ((UsbManager)getSystemService(USB_SERVICE)).hasPermission(device)) {
                command(cli ? WeaveService.ACTION_CLI_START : WeaveService.ACTION_USB);
            } else toast("USB permission not granted");
        }
    };

    @Override public void onCreate(Bundle saved) {
        super.onCreate(saved);
        prefs = getSharedPreferences(WeaveService.PREFS, MODE_PRIVATE);
        terminalFontSp = Math.max(9f, Math.min(24f, prefs.getFloat("terminal_font_sp", 12f)));
        terminalPinch = new ScaleGestureDetector(this, new ScaleGestureDetector.SimpleOnScaleGestureListener() {
            @Override public boolean onScale(ScaleGestureDetector detector) {
                applyTerminalScale(detector.getScaleFactor());
                return true;
            }
        });
        if (saved != null) {
            selectedPage = saved.getInt("page", HOME);
            pendingApkVersion = saved.getString("apk_update_version");
            if (pendingApkVersion != null) {
                pendingUpdateApk = new File(new File(getCacheDir(), "weave-updates"), "weave-app.apk");
                if (pendingUpdateApk.isFile()) {
                    apkUpdateRequest = saved.getInt("apk_update_request");
                    apkReadyToInstall = saved.getBoolean("apk_update_ready") || apkUpdateRequest == 0;
                    updateControls[UPDATE_APP].busy = true;
                    updateControls[UPDATE_APP].percent = 100;
                    updateControls[UPDATE_APP].state = "installing";
                } else clearApkUpdate();
            }
        }
        getWindow().setStatusBarColor(BACKGROUND);
        getWindow().setNavigationBarColor(SURFACE);
        IntentFilter filter = new IntentFilter(USB_PERMISSION);
        if (Build.VERSION.SDK_INT >= 33)
            registerReceiver(usbPermissions, filter, Context.RECEIVER_NOT_EXPORTED);
        else registerReceiver(usbPermissions, filter);
        receiverRegistered = true;
        IntentFilter deviceFilter = new IntentFilter();
        deviceFilter.addAction(WeaveService.EVENT_SCREEN);
        deviceFilter.addAction(WeaveService.EVENT_FILES);
        deviceFilter.addAction(WeaveService.EVENT_STORAGE_INFO);
        deviceFilter.addAction(WeaveService.EVENT_OPERATION);
        deviceFilter.addAction(WeaveService.EVENT_PROGRESS);
        deviceFilter.addAction(WeaveService.EVENT_FIRMWARE_UPDATE);
        deviceFilter.addAction(WeaveService.EVENT_DEVICE_VERSION);
        deviceFilter.addAction(WeaveService.EVENT_CATALOG_QUEUE);
        deviceFilter.addAction(WeaveService.EVENT_FAP_METADATA);
        deviceFilter.addAction(WeaveService.EVENT_SHOP_PROGRESS);
        deviceFilter.addAction(WeaveService.EVENT_FIND_DEVICE);
        deviceFilter.addAction(WeaveService.EVENT_TEXT_INPUT);
        deviceFilter.addAction(WeaveService.EVENT_CLI);
        deviceFilter.addAction(WeaveService.EVENT_FILE_DATA);
        if (Build.VERSION.SDK_INT >= 33) registerReceiver(deviceEvents, deviceFilter, Context.RECEIVER_NOT_EXPORTED);
        else registerReceiver(deviceEvents, deviceFilter);
        flipperEventsRegistered = true;

        LinearLayout shell = new LinearLayout(this);
        shell.setOrientation(LinearLayout.VERTICAL);
        shell.setBackgroundColor(BACKGROUND);
        shell.addView(header(), new LinearLayout.LayoutParams(-1, -2));
        FrameLayout host = new FrameLayout(this);
        contentHost = host;
        shell.addView(host, new LinearLayout.LayoutParams(-1, 0, 1));
        pages[HOME] = homePage();
        pages[APPS] = appsPage();
        pages[CONSOLE] = consolePage();
        pages[FILES] = filesPage();
        pages[SETTINGS] = settingsPage();
        for (View page : pages) host.addView(page, new FrameLayout.LayoutParams(-1, -1));
        gestureRefreshIndicator = new FrameLayout(this);
        gestureRefreshIndicator.setBackground(rounded(CARD, BORDER, 16));
        gestureRefreshIndicator.setElevation(dp(16));
        gestureRefreshGlyph = statusIcon(R.drawable.ic_apps_refresh, ORANGE);
        gestureRefreshGlyph.setContentDescription("Refreshing");
        gestureRefreshIndicator.addView(gestureRefreshGlyph,
                new FrameLayout.LayoutParams(dp(24), dp(24), Gravity.CENTER));
        FrameLayout.LayoutParams pullParams = new FrameLayout.LayoutParams(
                dp(50), dp(50), Gravity.TOP | Gravity.CENTER_HORIZONTAL);
        pullParams.topMargin = dp(7);
        host.addView(gestureRefreshIndicator, pullParams);
        gestureRefreshIndicator.setVisibility(View.GONE);
        installRing = new InstallRingView(this);
        installRing.setElevation(dp(12));
        FrameLayout.LayoutParams ringParams = new FrameLayout.LayoutParams(
                dp(58), dp(58), Gravity.END | Gravity.BOTTOM);
        ringParams.setMargins(0, 0, dp(16), dp(16));
        host.addView(installRing, ringParams);
        installRing.setVisibility(View.GONE);
        View line = new View(this);
        line.setBackgroundColor(BORDER);
        shell.addView(line, new LinearLayout.LayoutParams(-1, dp(1)));
        shell.addView(navigation(), new LinearLayout.LayoutParams(-1, dp(70)));
        setContentView(shell);
        selectPage(selectedPage);
        refresh();
        requestCatalog(false);
    }

    private View header() {
        FrameLayout bar = new FrameLayout(this);
        bar.setPadding(dp(20), dp(14), dp(20), dp(8));
        pageTitle = text("Home", 25, WHITE);
        pageTitle.setTypeface(Typeface.DEFAULT_BOLD);
        bar.addView(pageTitle, new FrameLayout.LayoutParams(-2, -2, Gravity.START | Gravity.CENTER_VERTICAL));
        chip = text("Flipper O", 14, WHITE);
        chip.setTypeface(Typeface.DEFAULT_BOLD);
        chip.setPadding(0, dp(7), 0, dp(7));
        FrameLayout.LayoutParams chipParams = new FrameLayout.LayoutParams(-2, -2, Gravity.END | Gravity.CENTER_VERTICAL);
        bar.addView(chip, chipParams);
        updateFlipperHeader(false);
        return bar;
    }

    private View homePage() {
        float heightDp = getResources().getDisplayMetrics().heightPixels /
                getResources().getDisplayMetrics().density;
        compactHome = heightDp <= 810f;
        boolean shortHome = heightDp <= 690f;
        tinyHome = heightDp <= 640f;
        homePadDp = tinyHome ? 44 : shortHome ? 50 : compactHome ? 58 : 62;
        homeDisplayMaxWidthDp = tinyHome ? 220 : shortHome ? 260 : compactHome ? 292 : 410;
        LinearLayout root = pageRoot();
        root.setPadding(dp(16), dp(4), dp(16), dp(8));
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        LinearLayout content = new BoundedColumn(this);
        content.setOrientation(LinearLayout.VERTICAL);
        root.addView(content, new LinearLayout.LayoutParams(-1, -2));

        TextView screenHeading = text("FLIPPER DISPLAY", 11, MUTED);
        screenHeading.setTypeface(Typeface.DEFAULT_BOLD);
        screenHeading.setLetterSpacing(.10f);
        LinearLayout.LayoutParams headingParams = new LinearLayout.LayoutParams(-1, -2);
        headingParams.bottomMargin = dp(compactHome ? 7 : 10);
        content.addView(screenHeading, headingParams);

        FrameLayout screen = new FlipperDisplay(this);
        screen.setBackground(rounded(0xff49300a, 0xffa37b37, 18));
        if (Build.VERSION.SDK_INT >= 21) {
            screen.setOutlineProvider(ViewOutlineProvider.BACKGROUND);
            screen.setClipToOutline(true);
        }
        screen.setContentDescription("Flipper screen");
        flipperScreenImage = new ImageView(this);
        flipperScreenImage.setScaleType(ImageView.ScaleType.FIT_CENTER);
        roundImage(flipperScreenImage, 18);
        flipperScreenImage.setVisibility(View.GONE);
        screen.addView(flipperScreenImage, new FrameLayout.LayoutParams(-1, -1));
        flipperScreenStatus = text("Flipper\ndisconnected", 20, 0xfff6dfad);
        flipperScreenStatus.setGravity(Gravity.CENTER);
        flipperScreenStatus.setTypeface(Typeface.MONOSPACE);
        flipperScreenStatus.setIncludeFontPadding(false);
        FrameLayout.LayoutParams statusParams = new FrameLayout.LayoutParams(-1, -2, Gravity.CENTER);
        statusParams.leftMargin = dp(16);
        statusParams.rightMargin = dp(16);
        screen.addView(flipperScreenStatus, statusParams);
        LinearLayout.LayoutParams screenParams = new LinearLayout.LayoutParams(-1, -2);
        screenParams.gravity = Gravity.CENTER_HORIZONTAL;
        content.addView(screen, screenParams);

        // Center the *whole* controller, including Back and Screen. Centering only
        // the D-pad made the group visually off-axis on every phone size.
        LinearLayout controlRow = new LinearLayout(this);
        controlRow.setOrientation(LinearLayout.HORIZONTAL);
        controlRow.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams controlsParams = new LinearLayout.LayoutParams(-1, dp(homePadDp * 3));
        controlsParams.topMargin = dp(tinyHome ? 10 : compactHome ? 12 : 22);
        content.addView(controlRow, controlsParams);

        LinearLayout pad = new LinearLayout(this);
        pad.setOrientation(LinearLayout.VERTICAL);
        pad.setGravity(Gravity.CENTER);
        controlRow.addView(pad, new LinearLayout.LayoutParams(dp(homePadDp * 3), dp(homePadDp * 3)));
        LinearLayout topPad = new LinearLayout(this);
        topPad.setGravity(Gravity.CENTER);
        topPad.addView(controlButton(R.drawable.ic_home_up, "Up"), touchParams());
        pad.addView(topPad);

        LinearLayout middlePad = new LinearLayout(this);
        middlePad.setGravity(Gravity.CENTER);
        middlePad.addView(controlButton(R.drawable.ic_home_left, "Left"), touchParams());
        TextView okButton = text("OK", 15, WHITE);
        okButton.setTypeface(Typeface.DEFAULT_BOLD);
        okButton.setGravity(Gravity.CENTER);
        okButton.setBackground(controlBackground());
        okButton.setContentDescription("OK");
        bindFlipperKeyTouch(okButton, "OK");
        middlePad.addView(okButton, touchParams());
        middlePad.addView(controlButton(R.drawable.ic_home_right, "Right"), touchParams());
        pad.addView(middlePad);

        LinearLayout bottomPad = new LinearLayout(this);
        bottomPad.setGravity(Gravity.CENTER);
        bottomPad.addView(controlButton(R.drawable.ic_home_down, "Down"), touchParams());
        pad.addView(bottomPad);

        LinearLayout secondary = new LinearLayout(this);
        secondary.setOrientation(LinearLayout.VERTICAL);
        secondary.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams sideParams = new LinearLayout.LayoutParams(dp(Math.min(48, homePadDp - 2)), -1);
        sideParams.leftMargin = dp(tinyHome ? 9 : 14);
        controlRow.addView(secondary, sideParams);
        secondary.addView(homeSideAction(R.drawable.ic_home_back, "Back"));
        LinearLayout.LayoutParams captureParams = new LinearLayout.LayoutParams(-1, -2);
        captureParams.topMargin = dp(compactHome ? 4 : 10);
        secondary.addView(homeSideAction(R.drawable.ic_home_camera, "Capture"), captureParams);

        TextView statusHeading = text("DEVICE STATUS", 11, MUTED);
        statusHeading.setTypeface(Typeface.DEFAULT_BOLD);
        statusHeading.setLetterSpacing(.10f);
        LinearLayout.LayoutParams statusHeadingParams = new LinearLayout.LayoutParams(-1, -2);
        statusHeadingParams.topMargin = dp(tinyHome ? 10 : compactHome ? 14 : 24);
        statusHeadingParams.bottomMargin = dp(tinyHome ? 5 : compactHome ? 7 : 10);
        content.addView(statusHeading, statusHeadingParams);

        LinearLayout batteryCard = card(false);
        batteryCard.setGravity(Gravity.CENTER_VERTICAL);
        batteryCard.setOrientation(LinearLayout.HORIZONTAL);
        batteryCard.setPadding(dp(12), dp(compactHome ? 9 : 13), dp(12), dp(compactHome ? 9 : 13));
        batteryCard.setMinimumHeight(dp(tinyHome ? 50 : compactHome ? 54 : 66));
        batteryIconHome = statusIcon(R.drawable.ic_home_battery_unknown, GREEN);
        batteryCard.addView(batteryIconHome, iconParams(25, 11));
        LinearLayout batteryLabels = new LinearLayout(this);
        batteryLabels.setOrientation(LinearLayout.VERTICAL);
        TextView batteryTitle = text("Battery", 15, WHITE);
        batteryTitle.setTypeface(Typeface.DEFAULT_BOLD);
        batteryLabels.addView(batteryTitle);
        batteryDetailHome = text("—", 12, MUTED);
        batteryDetailHome.setPadding(0, dp(3), 0, 0);
        batteryLabels.addView(batteryDetailHome);
        batteryCard.addView(batteryLabels, new LinearLayout.LayoutParams(0, -2, 1));
        // The percentage is displayed once under the Battery label.
        content.addView(batteryCard, new LinearLayout.LayoutParams(-1, -2));

        LinearLayout statuses = new LinearLayout(this);
        statuses.setOrientation(LinearLayout.HORIZONTAL);
        LinearLayout.LayoutParams statusesParams = new LinearLayout.LayoutParams(-1, -2);
        statusesParams.topMargin = dp(tinyHome ? 6 : compactHome ? 8 : 10);
        content.addView(statuses, statusesParams);
        btValueHome = statusTile(statuses, R.drawable.ic_home_bluetooth, "Bluetooth", 0xff72b4ff, false);
        usbValueHome = statusTile(statuses, R.drawable.ic_home_usb, "USB", 0xffc59bfa, true);
        return scroll(root);
    }

    // Prevent giant elements on tablets while respecting the available width on phones.
    private final class BoundedColumn extends LinearLayout {
        BoundedColumn(Context context) { super(context); }
        @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int width = Math.min(View.MeasureSpec.getSize(widthMeasureSpec), dp(480));
            super.onMeasure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY), heightMeasureSpec);
        }
    }

    private final class FlipperDisplay extends FrameLayout {
        FlipperDisplay(Context context) { super(context); }
        @Override protected void onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
            int width = Math.min(View.MeasureSpec.getSize(widthMeasureSpec), dp(homeDisplayMaxWidthDp));
            // Original Flipper Zero LCD is 128 × 64. Keep the panel at a 2:1 ratio.
            int height = width / 2;
            super.onMeasure(View.MeasureSpec.makeMeasureSpec(width, View.MeasureSpec.EXACTLY),
                    View.MeasureSpec.makeMeasureSpec(height, View.MeasureSpec.EXACTLY));
        }
    }

    private static final class ArchiveExportSession {
        static final class PendingItem {
            final String path;
            final String zipPath;
            final boolean directory;
            PendingItem(String path, String zipPath, boolean directory) {
                this.path = path;
                this.zipPath = zipPath;
                this.directory = directory;
            }
        }

        final Uri destination;
        final ArrayDeque<PendingItem> queue = new ArrayDeque<>();
        final Map<String, String> pathToZip = new HashMap<>();
        ZipOutputStream zip;
        String awaitingListPath;
        String awaitingFilePath;

        ArchiveExportSession(Uri destination) {
            this.destination = destination;
        }
    }

    private LinearLayout.LayoutParams touchParams() {
        return new LinearLayout.LayoutParams(dp(homePadDp), dp(homePadDp));
    }
    private View homeSideAction(int iconRes, String action) {
        LinearLayout wrapper = new LinearLayout(this);
        wrapper.setOrientation(LinearLayout.VERTICAL);
        wrapper.setGravity(Gravity.CENTER);
        int sideDp = Math.min(48, homePadDp - 2);
        ImageButton button = controlButton(iconRes, action);
        wrapper.addView(button, new LinearLayout.LayoutParams(dp(sideDp), dp(sideDp)));
        TextView label = text("Capture".equals(action) ? "Screen" : action, 10, MUTED);
        label.setGravity(Gravity.CENTER);
        label.setPadding(0, dp(2), 0, 0);
        wrapper.addView(label, new LinearLayout.LayoutParams(-1, -2));
        if ("Capture".equals(action)) label.setOnClickListener(v -> captureScreen());
        else bindFlipperKeyTouch(label, action);
        return wrapper;
    }

    private RippleDrawable controlBackground() {
        return new RippleDrawable(ColorStateList.valueOf(0x334b8dff),
                rounded(0xff171719, 0xff33363b, 13), null);
    }

    // Les inputs virtuels suivent PRESS / LONG / REPEAT / SHORT / RELEASE.
    private void bindFlipperKeyTouch(View view, String action) {
        int key = flipperKey(action);
        if (key < 0) return;
        GamepadGesture gesture = new GamepadGesture();
        final boolean[] suppressClick = {false};
        final Runnable[] longTask = new Runnable[1];
        final Runnable[] repeatTask = new Runnable[1];
        repeatTask[0] = () -> {
            int type = gesture.repeat();
            if (type < 0) return;
            sendFlipperInput(key, type);
            handler.postDelayed(repeatTask[0], 150);
        };
        longTask[0] = () -> {
            int type = gesture.hold();
            if (type < 0) return;
            sendFlipperInput(key, type);
            handler.postDelayed(repeatTask[0], 150);
        };
        releaseGamepadOnPause.add(() -> {
            handler.removeCallbacks(longTask[0]);
            handler.removeCallbacks(repeatTask[0]);
            if (gesture.up(true) != -2) sendFlipperInput(key, 1);
            view.setPressed(false);
        });
        view.setOnClickListener(v -> {
            if (!suppressClick[0] && !remotePadLocked()) sendFlipperKey(action);
        });
        view.setOnTouchListener((v, event) -> {
            switch (event.getActionMasked()) {
                case MotionEvent.ACTION_DOWN:
                    if (remotePadLocked()) return true;
                    if (gesture.down() < 0) return true;
                    v.setPressed(true);
                    sendFlipperInput(key, 0);
                    handler.postDelayed(longTask[0], 300);
                    return true;
                case MotionEvent.ACTION_UP:
                case MotionEvent.ACTION_CANCEL:
                    boolean cancel = event.getActionMasked() == MotionEvent.ACTION_CANCEL;
                    int release = gesture.up(cancel);
                    handler.removeCallbacks(longTask[0]);
                    handler.removeCallbacks(repeatTask[0]);
                    v.setPressed(false);
                    if (release < -1) return true;
                    if (release == 2) sendFlipperInput(key, 2);
                    sendFlipperInput(key, 1);
                    if (!cancel) {
                        suppressClick[0] = true;
                        try { v.performClick(); }
                        finally { suppressClick[0] = false; }
                    }
                    return true;
                default:
                    return true;
            }
        });
    }

    private ImageButton controlButton(int iconRes, String action) {
        ImageButton button = new ImageButton(this);
        button.setImageResource(iconRes);
        button.setImageTintList(ColorStateList.valueOf(0xffeeeeee));
        button.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        button.setPadding(dp(14), dp(14), dp(14), dp(14));
        button.setBackground(controlBackground());
        button.setContentDescription(action);
        if ("Capture".equals(action)) button.setOnClickListener(v -> captureScreen());
        else bindFlipperKeyTouch(button, action);
        return button;
    }

    private View controlAction(int iconRes, String action) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        ImageButton button = controlButton(iconRes, action);
        row.addView(button, touchParams());
        TextView caption = text(action, 14, WHITE);
        caption.setGravity(Gravity.CENTER_VERTICAL);
        caption.setPadding(dp(10), 0, 0, 0);
        if ("Capture".equals(action)) caption.setOnClickListener(v -> captureScreen());
        else bindFlipperKeyTouch(caption, action);
        row.addView(caption, new LinearLayout.LayoutParams(0, dp(56), 1));
        return row;
    }

    private ImageView statusIcon(int resource, int color) {
        ImageView icon = new ImageView(this);
        icon.setImageResource(resource);
        icon.setImageTintList(ColorStateList.valueOf(color));
        icon.setScaleType(ImageView.ScaleType.FIT_CENTER);
        return icon;
    }

    private LinearLayout.LayoutParams iconParams(int sizeDp, int rightMarginDp) {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(dp(sizeDp), dp(sizeDp));
        params.rightMargin = dp(rightMarginDp);
        return params;
    }

    private TextView statusTile(LinearLayout parent, int imageResource, String title, int color, boolean second) {
        LinearLayout tile = card(false);
        tile.setOrientation(LinearLayout.HORIZONTAL);
        tile.setGravity(Gravity.CENTER_VERTICAL);
        tile.setPadding(dp(10), dp(compactHome ? 9 : 13), dp(9), dp(compactHome ? 9 : 13));
        tile.setMinimumHeight(dp(tinyHome ? 50 : compactHome ? 54 : 66));
        tile.addView(statusIcon(imageResource, color), iconParams(23, 9));
        LinearLayout labels = new LinearLayout(this);
        labels.setOrientation(LinearLayout.VERTICAL);
        TextView label = text(title, 14, WHITE);
        label.setTypeface(Typeface.DEFAULT_BOLD);
        labels.addView(label);
        TextView value = text("Disconnected", 12, MUTED);
        value.setSingleLine(true);
        value.setTextSize(TypedValue.COMPLEX_UNIT_SP, 11.5f);
        value.setPadding(0, dp(3), 0, 0);
        labels.addView(value);
        tile.addView(labels, new LinearLayout.LayoutParams(0, -2, 1));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, -2, 1);
        if (second) params.leftMargin = dp(8);
        parent.addView(tile, params);
        return value;
    }

    private void updateAppsModeTabs() {
        if (appsModeTabs == null) return;
        for (int i = 0; i < appsModeTabs.length; i++) {
            boolean selected = i == appsMode;
            appsModeTabs[i].setTextColor(selected ? ORANGE : WHITE);
            appsModeTabs[i].setBackground(selected ? rounded(ORANGE_DARK, ORANGE, 14) : rounded(CARD, 0xffd9d9d9, 14));
        }
    }

    private void updateAppsFilterAccent() {
        boolean active = !"All".equals(appsFilterCategory) || appsSortMode != 0;
        int color = active ? ORANGE : MUTED;
        if (appsFilterLabel != null) appsFilterLabel.setTextColor(color);
        if (appsFilterButton != null) appsFilterButton.setImageTintList(ColorStateList.valueOf(color));
    }

    private void showAppsFilterDialog() {
        final String[] categories = {"All", "Games", "USB", "NFC", "RFID", "Tools", "GPIO", "Bluetooth", "iButton", "Sub-GHz", "Infrared", "Other"};
        final String[] sortModes = {"Name (A → Z)", "Name (Z → A)", "Most recent", "Most viewed"};
        final String[] pickedCategory = {appsFilterCategory};
        final int[] pickedSort = {appsSortMode};

        LinearLayout shell = new LinearLayout(this);
        shell.setOrientation(LinearLayout.VERTICAL);
        shell.setPadding(dp(18), dp(18), dp(18), dp(18));
        shell.setBackground(rounded(CARD, 0xffd9d9d9, 18));

        TextView title = text("Search filters", 20, WHITE);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        shell.addView(title);

        TextView catTitle = text("Category", 13, MUTED);
        catTitle.setPadding(0, dp(16), 0, dp(8));
        shell.addView(catTitle);

        LinearLayout chipRows = new LinearLayout(this);
        chipRows.setOrientation(LinearLayout.VERTICAL);
        shell.addView(chipRows);
        List<TextView> chipViews = new ArrayList<>();
        LinearLayout row = null;
        for (int i = 0; i < categories.length; i++) {
            if (i % 3 == 0) {
                row = new LinearLayout(this);
                row.setOrientation(LinearLayout.HORIZONTAL);
                if (i > 0) {
                    LinearLayout.LayoutParams rp = new LinearLayout.LayoutParams(-1, -2);
                    rp.topMargin = dp(8);
                    chipRows.addView(row, rp);
                } else chipRows.addView(row);
            }
            final String category = categories[i];
            TextView chipView = text(category, 12, WHITE);
            chipView.setPadding(dp(12), dp(8), dp(12), dp(8));
            chipView.setOnClickListener(v -> {
                pickedCategory[0] = category;
                for (TextView chip : chipViews) {
                    boolean sel = chip.getText().toString().equals(pickedCategory[0]);
                    chip.setTextColor(sel ? ORANGE : WHITE);
                    chip.setBackground(sel ? rounded(ORANGE_DARK, ORANGE, 12) : rounded(CARD, 0xffbfc3cc, 12));
                }
            });
            boolean selected = category.equals(pickedCategory[0]);
            chipView.setTextColor(selected ? ORANGE : WHITE);
            chipView.setBackground(selected ? rounded(ORANGE_DARK, ORANGE, 12) : rounded(CARD, 0xffbfc3cc, 12));
            LinearLayout.LayoutParams cp = new LinearLayout.LayoutParams(-2, -2);
            if (i % 3 != 0) cp.leftMargin = dp(8);
            row.addView(chipView, cp);
            chipViews.add(chipView);
        }

        TextView sortTitle = text("Sort by", 13, MUTED);
        sortTitle.setPadding(0, dp(18), 0, dp(8));
        shell.addView(sortTitle);
        List<TextView> sortViews = new ArrayList<>();
        for (int i = 0; i < sortModes.length; i++) {
            final int index = i;
            TextView option = text((pickedSort[0] == i ? "● " : "○ ") + sortModes[i], 13, WHITE);
            option.setPadding(0, dp(6), 0, dp(6));
            option.setOnClickListener(v -> {
                pickedSort[0] = index;
                for (int j = 0; j < sortViews.size(); j++) {
                    sortViews.get(j).setText((pickedSort[0] == j ? "● " : "○ ") + sortModes[j]);
                    sortViews.get(j).setTextColor(pickedSort[0] == j ? ORANGE : WHITE);
                }
            });
            option.setTextColor(pickedSort[0] == i ? ORANGE : WHITE);
            shell.addView(option);
            sortViews.add(option);
        }

        LinearLayout buttons = new LinearLayout(this);
        buttons.setOrientation(LinearLayout.HORIZONTAL);
        buttons.setGravity(Gravity.END);
        LinearLayout.LayoutParams bp = new LinearLayout.LayoutParams(-1, -2);
        bp.topMargin = dp(20);
        shell.addView(buttons, bp);

        TextView cancel = text("Cancel", 14, WHITE);
        cancel.setTypeface(Typeface.DEFAULT_BOLD);
        cancel.setGravity(Gravity.CENTER);
        cancel.setPadding(dp(18), dp(12), dp(18), dp(12));
        cancel.setBackground(rounded(CARD, 0xffd9d9d9, 14));
        TextView apply = text("Apply", 14, 0xff151515);
        apply.setTypeface(Typeface.DEFAULT_BOLD);
        apply.setGravity(Gravity.CENTER);
        apply.setPadding(dp(18), dp(12), dp(18), dp(12));
        apply.setBackground(rounded(ORANGE, ORANGE, 14));
        buttons.addView(cancel);
        LinearLayout.LayoutParams ap = new LinearLayout.LayoutParams(-2, -2);
        ap.leftMargin = dp(12);
        buttons.addView(apply, ap);

        AlertDialog dialog = new AlertDialog.Builder(this).setView(shell).create();
        cancel.setOnClickListener(v -> dialog.dismiss());
        apply.setOnClickListener(v -> {
            appsFilterCategory = pickedCategory[0];
            appsSortMode = pickedSort[0];
            appsVisibleLimit = 40;
            updateAppsFilterAccent();
            renderAppsList();
            dialog.dismiss();
        });
        dialog.show();
        if (dialog.getWindow() != null) dialog.getWindow().setBackgroundDrawableResource(android.R.color.transparent);
    }

    private void requestCatalog(boolean force) {
        if (catalogRequestInFlight) return;
        catalogRequestInFlight = true;
        if (force) {
            detailMetadataRequests.clear();
            detailMetadataDone.clear();
            detailDescriptionPending.clear();
            detailDescriptionDone.clear();
            detailDescriptions.clear();
        }
        if (appCatalogClient == null) appCatalogClient = new AppCatalogClient(this);
        final int generation = ++catalogRequestGeneration;
        catalogLoading = true;
        catalogError = null;
        if (selectedPage == APPS) renderAppsList();
        appCatalogClient.load((result, error) -> {
            if (isFinishing() || isDestroyed() || generation != catalogRequestGeneration) return;
            if (!result.isEmpty()) {
                catalogApps.clear();
                catalogApps.addAll(result);
            }
            catalogLoading = false;
            catalogError = error;
            if (selectedPage == APPS) {
                resolveInstalledMetadata();
                renderAppsList();
            }
            if (!result.isEmpty() && !appsRefreshInProgress && activeBadgeJob() == null)
                syncShopCatalogToFlipper();
        }, force, () -> {
            catalogRequestInFlight = false;
            if (appsRefreshInProgress) {
                appsRefreshCatalogDone = true;
                checkAppsRefreshCompletion();
            }
        });
    }

    /** Share real public catalog metadata with the native Flipper Shop over storage RPC. */
    private void syncShopCatalogToFlipper() {
        // A 16 KiB background index upload is not worth delaying a native
        // completion marker or saturating RPC while searching/refreshing.
        if (!flipperIsConnected() || catalogApps.isEmpty() || activeInstallId != null ||
                activeBadgeJob() != null || appsRefreshInProgress) return;
        List<AppCatalogClient.Entry> selected = new ArrayList<>(catalogApps);
        String term = appsSearchInput == null ? "" : appsSearchInput.getText().toString().trim().toLowerCase(Locale.ROOT);
        selected.sort((a, b) -> {
            if (!term.isEmpty()) {
                boolean am = a.title.toLowerCase(Locale.ROOT).contains(term);
                boolean bm = b.title.toLowerCase(Locale.ROOT).contains(term);
                if (am != bm) return am ? -1 : 1;
            }
            return a.title.compareToIgnoreCase(b.title);
        });
        StringBuilder data = new StringBuilder();
        for (AppCatalogClient.Entry e : selected) {
            if (e.id == null || !e.id.matches("[A-Za-z0-9_-]{2,80}")) continue;
            String title = catalogAscii(e.title, 32);
            String cat = catalogAscii(e.category, 20);
            String desc = catalogAscii(e.description, 94);
            if (title.isEmpty()) continue;
            if (cat.isEmpty()) cat = "Other";
            if (desc.isEmpty()) desc = "Available from Flipper catalog";
            String icon = appCatalogClient == null ? null : appCatalogClient.cachedDeviceIconHex(e);
            data.append(e.id).append('\t').append(title).append('\t')
                    .append(cat).append('\t').append(desc);
            data.append('\t').append(icon == null ? "-" : icon);
            String version = catalogAscii(e.version, 32);
            data.append('\t').append(version.isEmpty() ? "-" : version).append('\n');
            // A bounded snapshot avoids saturating the BLE serial link.
            if (data.length() > 16000) break;
        }
        if (data.length() == 0) return;
        int hash = data.toString().hashCode();
        if (shopCatalogSyncedThisConnection && shopCatalogSyncHash == hash) return;
        Intent sync = new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_SYNC_SHOP);
        sync.putExtra("catalog", data.toString());
        try { startService(sync); shopCatalogSyncedThisConnection = true; shopCatalogSyncHash = hash; }
        catch (RuntimeException ignored) { shopCatalogSyncedThisConnection = false; }
    }

    private static String catalogAscii(String input, int max) {
        if (input == null) return "";
        String safe = input.replace('\t', ' ').replace('\n', ' ').replace('\r', ' ')
                .replaceAll("[^\\x20-\\x7E]", " ").replaceAll(" +", " ").trim();
        return safe.substring(0, Math.min(safe.length(), max));
    }

    private void requestCatalogSearch() {
        if (appsSearchInput == null || appsSearchInput.getText() == null) return;
        if (appsRefreshInProgress && !appsRefreshCatalogDone) {
            // Defer official online search until the costly full refresh is done;
            // local matches are still rendered immediately as the user types.
            handler.removeCallbacks(delayedCatalogSearch);
            handler.postDelayed(delayedCatalogSearch, 700);
            return;
        }
        String term = appsSearchInput.getText().toString().trim();
        if (term.length() < 2) return;
        if (appCatalogClient == null) appCatalogClient = new AppCatalogClient(this);
        final int generation = ++catalogSearchGeneration;
        catalogSearchTerm = term;
        catalogSearchLoading = true;
        catalogSearchError = null;
        appCatalogClient.search(term, (result, error) -> {
            if (isFinishing() || isDestroyed() || generation != catalogSearchGeneration) return;
            if (appsSearchInput == null || !term.equals(appsSearchInput.getText().toString().trim())) return;
            catalogSearchLoading = false;
            catalogSearchError = error;
            catalogSearchResults.clear();
            catalogSearchResults.addAll(result);
            // Keep official ranked search results, including matches via metadata not
            // visible in the app title. The local catalog remains as an offline fallback.
            if (selectedPage == APPS && appDetailEntry == null) renderAppsList();
        });
    }

    private static String normalizedAppId(String text) {
        if (text == null) return "";
        return text.toLowerCase(Locale.ROOT).replaceAll("[^a-z0-9]", "");
    }

    private boolean inventoryReady() {
        return "ready".equals(prefs.getString(WeaveService.KEY_INVENTORY_STATUS, ""));
    }

    private Set<String> installedIds() {
        Set<String> ids = new HashSet<>();
        if (!inventoryReady()) return ids;
        try {
            JSONArray names = new JSONArray(prefs.getString(WeaveService.KEY_INVENTORY, "[]"));
            for (int i = 0; i < names.length(); i++) ids.add(normalizedAppId(names.optString(i)));
        } catch (Exception ignored) { }
        return ids;
    }

    private AppCatalogClient.Entry matchingCatalogApp(String id) {
        String normalized = normalizedAppId(id);
        for (AppCatalogClient.Entry entry : catalogApps) {
            if (normalized.equals(normalizedAppId(entry.id))) return entry;
        }
        return null;
    }

    private void resolveInstalledMetadata() {
        if (!inventoryReady() || appCatalogClient == null || catalogLoading) return;
        try {
            JSONArray list = new JSONArray(prefs.getString(WeaveService.KEY_INVENTORY, "[]"));
            for (int i = 0; i < list.length() && i < 150; i++) {
                String id = list.optString(i);
                String key = normalizedAppId(id);
                AppCatalogClient.Entry inCatalog = matchingCatalogApp(id);
                if (key.isEmpty() || (inCatalog != null && inCatalog.iconUrl != null)
                        || installedMetadata.containsKey(key) || !metadataLookups.add(key)) continue;
                appCatalogClient.loadInstalledMetadata(id, meta -> {
                    if (meta != null && normalizedAppId(meta.id).equals(key)) {
                        AppCatalogClient.Entry enriched = new AppCatalogClient.Entry(id, meta.title,
                                meta.category, meta.version, meta.iconUrl, meta.pageUrl,
                                meta.description, meta.sourceUrl, meta.manifestUrl,
                                meta.descriptionFileUrl, meta.screenshots, meta.updated, meta.views, meta.archiveId);
                        installedMetadata.put(key, enriched);
                        for (int index = 0; index < catalogApps.size(); index++) {
                            AppCatalogClient.Entry existing = catalogApps.get(index);
                            if (normalizedAppId(existing.id).equals(key) && existing.iconUrl == null) {
                                catalogApps.set(index, new AppCatalogClient.Entry(existing.id,
                                        existing.title, existing.category, existing.version,
                                        enriched.iconUrl, existing.pageUrl == null ? enriched.pageUrl : existing.pageUrl,
                                        existing.description == null ? enriched.description : existing.description,
                                        existing.sourceUrl == null ? enriched.sourceUrl : existing.sourceUrl,
                                        existing.manifestUrl, existing.descriptionFileUrl,
                                        existing.screenshots.isEmpty() ? enriched.screenshots : existing.screenshots,
                                        existing.updated, existing.views,
                                        existing.archiveId == null ? enriched.archiveId : existing.archiveId));
                                break;
                            }
                        }
                    }
                    if (selectedPage == APPS && appsMode == 1) renderAppsList();
                });
            }
        } catch (Exception ignored) { }
    }

    private List<AppCatalogClient.Entry> localInstalledExtras(Set<String> installed) {
        List<AppCatalogClient.Entry> extras = new ArrayList<>();
        if (!inventoryReady()) return extras;
        try {
            JSONArray names = new JSONArray(prefs.getString(WeaveService.KEY_INVENTORY, "[]"));
            org.json.JSONObject paths = new org.json.JSONObject(
                    prefs.getString(WeaveService.KEY_INVENTORY_PATHS, "{}"));
            Set<String> seenIds = new HashSet<>();
            Set<String> seenPaths = new HashSet<>();
            for (int i = 0; i < names.length(); i++) {
                String id = names.optString(i);
                String key = normalizedAppId(id);
                if (key.isEmpty() || !seenIds.add(key)) continue;
                String path = paths.optString(key, "");
                if (!path.isEmpty() && !seenPaths.add(path)) continue;
                if (matchingCatalogApp(id) != null) continue;
                AppCatalogClient.Entry enriched = installedMetadata.get(normalizedAppId(id));
                org.json.JSONObject nativeInfo = nativeFapInfo(id);
                String realTitle = nativeInfo == null ? id : nativeInfo.optString("title", id);
                String group = installedCategory(id, "Other");
                extras.add(enriched == null ? new AppCatalogClient.Entry(id, realTitle, group, null,
                        null, null, "Installed FAP file", null, null, null,
                        Collections.emptyList(), 0, 0) : enriched);
            }
        } catch (Exception ignored) { }
        return extras;
    }

    /** Service owns installation state; an Activity broadcast can be missed.
     *  Poll the persisted phase once per second while the Apps page is visible.
     *  Only phase transitions rebuild the detail; progress simply updates its bar. */
    private void reconcileInstallDetail() {
        if (selectedPage != APPS || appDetailEntry == null) {
            lastDetailInstallSnapshot = "";
            return;
        }
        String id = appDetailEntry.id;
        org.json.JSONObject queue = catalogQueueInfo(id);
        org.json.JSONObject nativeJob = nativeShopInfo(id);
        String nativePhase = nativeJob == null ? "" : nativeJob.optString("phase", "");
        boolean nativeBusy = "downloading".equals(nativePhase) || "installing".equals(nativePhase);
        String phase = nativeBusy ? "native:" + nativePhase :
                (queue == null ? "idle" : "queued:" + queue.optString("phase", "queued"));
        String signature = id + ":" + phase + ":" +
                (catalogFailureFor(id) == null ? "" : catalogFailureFor(id)) + ":" +
                appStateOf(id);
        if (!signature.equals(lastDetailInstallSnapshot)) {
            lastDetailInstallSnapshot = signature;
            renderAppDetail(appDetailEntry);
        }
        if (detailInstallBar == null || !id.equals(detailInstallId)) return;
        boolean downloading = "native:downloading".equals(phase) ||
                "queued:downloading".equals(phase);
        int percentage = nativeBusy ? ("downloading".equals(nativePhase) ?
                nativeJob.optInt("percent", -1) : -1) :
                (queue == null ? -1 : queue.optInt("percent", -1));
        // Once HTTP has reached 100%, the Flipper still needs to upload,
        // verify, animate and close. Do not show a stuck determinate 100% bar.
        boolean indeterminate = !downloading || percentage < 0 || percentage >= 100;
        detailInstallBar.setIndeterminate(indeterminate);
        if (!indeterminate) detailInstallBar.setProgress(percentage);
        if (detailInstallPercent != null)
            detailInstallPercent.setText(indeterminate ?
                    ("queued:awaiting_home".equals(phase) || "native:installing".equals(phase) ?
                            "Finishing on Flipper…" : "Working…") : percentage + "%");
    }

    private void requestInstalledApps() {
        if (flipperIsConnected()) command(WeaveService.ACTION_REFRESH_APPS);
    }

    private void ensureInstalledApps() {
        String status = prefs.getString(WeaveService.KEY_INVENTORY_STATUS, "");
        if (flipperIsConnected() && !"ready".equals(status) && !"loading".equals(status))
            requestInstalledApps();
    }

    private void renderAppsList() {
        handler.post(this::updateInstallRing);
        visibleAppInstallRings.clear();
        if (appsListHost == null) return;
        if (appDetailEntry != null) {
            renderAppDetail(appDetailEntry);
            return;
        }
        detailInstallBar = null;
        detailInstallPercent = null;
        detailInstallStatus = null;
        detailInstallId = null;
        appsListHost.removeAllViews();
        if ((appsMode == 1 || appsMode == 2) && !flipperIsConnected()) {
            showAppsMessage("Connect your Flipper to view apps.");
            return;
        }
        if (catalogLoading && catalogApps.isEmpty()) {
            showAppsMessage("Loading applications…");
            return;
        }
        if (catalogError != null && catalogApps.isEmpty()) {
            showAppsMessage(catalogError);
            return;
        }
        if ((appsMode == 1 || appsMode == 2) && !inventoryReady()) {
            String inventoryStatus = prefs.getString(WeaveService.KEY_INVENTORY_STATUS, "");
            showAppsMessage("loading".equals(inventoryStatus)
                    ? "Reading installed apps from Flipper…"
                    : "Installed apps unavailable. Tap Refresh to retry.");
            return;
        }
        String query = appsSearchInput == null || appsSearchInput.getText() == null
                ? "" : appsSearchInput.getText().toString().trim().toLowerCase(Locale.ROOT);
        if (appsMode == 1) resolveInstalledMetadata();
        List<AppCatalogClient.Entry> visible = new ArrayList<>();
        boolean officialSearch = appsMode == 0 && query.length() >= 2
                && query.equalsIgnoreCase(catalogSearchTerm)
                && !catalogSearchResults.isEmpty();
        // Eviter le dedup/regex global pendant une recherche Shop locale.
        List<AppCatalogClient.Entry> candidates;
        Set<String> officialIds = new HashSet<>();
        Set<String> installed = appsMode == 0 ? Collections.emptySet() : installedIds();
        if (officialSearch || appsMode != 0) {
            java.util.LinkedHashMap<String, AppCatalogClient.Entry> byId =
                    new java.util.LinkedHashMap<>();
            if (officialSearch) for (AppCatalogClient.Entry item : catalogSearchResults) {
                String key = normalizedAppId(item.id);
                officialIds.add(key);
                byId.put(key, item);
            }
            for (AppCatalogClient.Entry item : catalogApps)
                byId.putIfAbsent(normalizedAppId(item.id), item);
            if (appsMode == 1) for (AppCatalogClient.Entry item : localInstalledExtras(installed))
                byId.putIfAbsent(normalizedAppId(item.id), item);
            candidates = new ArrayList<>(byId.values());
        } else {
            candidates = catalogApps;
        }
        Set<String> shownVerifiedPaths = new HashSet<>();
        Set<String> shownInstalledNames = new HashSet<>();
        for (AppCatalogClient.Entry item : candidates) {
            String realCategory = appsMode == 0 ? item.category : installedCategory(item.id, item.category);
            if (!"All".equals(appsFilterCategory) &&
                    !appsFilterCategory.equalsIgnoreCase(realCategory)) continue;
            if (!query.isEmpty()) {
                boolean localMatch = item.title.toLowerCase(Locale.ROOT).contains(query)
                        || item.id.toLowerCase(Locale.ROOT).contains(query)
                        || item.category.toLowerCase(Locale.ROOT).contains(query)
                        || (item.description != null &&
                            item.description.toLowerCase(Locale.ROOT).contains(query));
                if (!localMatch && (officialIds.isEmpty() ||
                    !officialIds.contains(normalizedAppId(item.id)))) continue;
            }
            if (appsMode == 0) {
                visible.add(item);
                continue;
            }
            String normalizedId = normalizedAppId(item.id);
            if (!installed.contains(normalizedId)) continue;
            if (appsMode == 1) {
                String verifiedPath = verifiedInstalledPath(item.id);
                if (verifiedPath != null && !shownVerifiedPaths.add(verifiedPath)) continue;
                org.json.JSONObject nativeInfo = nativeFapInfo(item.id);
                String identity = nativeInfo == null ? item.title : nativeInfo.optString("title", item.title);
                // Plusieurs noms de catalogue peuvent pointer vers la même FAP native.
                if (!shownInstalledNames.add(normalizedAppId(identity))) continue;
            }
            if (appsMode == 2 && !hasVerifiedUpdate(item.id, item.version)) continue;
            visible.add(item);
        }
        Collections.sort(visible, (a, b) -> {
            if (appsSortMode == 1) return b.title.compareToIgnoreCase(a.title);
            if (appsSortMode == 2 && (a.updated > 0 || b.updated > 0)) return Long.compare(b.updated, a.updated);
            if (appsSortMode == 3 && (a.views > 0 || b.views > 0)) return Long.compare(b.views, a.views);
            if (officialSearch && appsSortMode == 0) return 0; // Keep Flipper Lab's relevance order.
            return a.title.compareToIgnoreCase(b.title);
        });
        if (visible.isEmpty()) {
            if (appsMode == 0 && catalogSearchLoading) {
                showAppsMessage("Searching catalog…");
                return;
            }
            if (appsMode == 0 && catalogSearchError != null) {
                showAppsMessage(catalogSearchError);
                return;
            }
            if (appsMode == 1) showAppsMessage("No .fap files found on the Flipper SD card.");
            else if (appsMode == 2) showAppsMessage("No verified installed apps with published versions. Refresh to retry.");
            else showAppsMessage("No matching apps.");
            return;
        }
        boolean connected = flipperIsConnected();
        // Keep the full catalog searchable while only mounting one page of cards.
        // Thousands of inflated image views otherwise freeze Android during scrolling.
        // Limit card inflation while searching; Load more remains available.
        int pageLimit = appsVisibleLimit;
        int displayCount = Math.min(visible.size(), pageLimit);
        for (int i = 0; i < displayCount; i++) {
            AppCatalogClient.Entry item = visible.get(i);
            LinearLayout card = card(false);
            card.setOrientation(LinearLayout.HORIZONTAL);
            card.setGravity(Gravity.CENTER_VERTICAL);
            card.setPadding(dp(12), dp(12), dp(12), dp(12));
            FrameLayout iconWrap = new FrameLayout(this);
            iconWrap.setBackground(rounded(CARD, BORDER, 12));
            ImageView appIcon = statusIcon(android.R.drawable.ic_menu_gallery, MUTED);
            appIcon.setVisibility(View.INVISIBLE);
            appIcon.setPadding(0, 0, 0, 0);
            roundImage(appIcon);
            iconWrap.addView(appIcon, new FrameLayout.LayoutParams(-1, -1));
            ProgressBar iconLoading = new ProgressBar(this);
            iconLoading.setIndeterminateTintList(ColorStateList.valueOf(ORANGE));
            FrameLayout.LayoutParams spinParams = new FrameLayout.LayoutParams(dp(23), dp(23), Gravity.CENTER);
            iconWrap.addView(iconLoading, spinParams);
            LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(44), dp(44));
            card.addView(iconWrap, iconParams);
            showAppIcon(item, appIcon, iconLoading);

            LinearLayout labels = new LinearLayout(this);
            labels.setOrientation(LinearLayout.VERTICAL);
            LinearLayout.LayoutParams labelsParams = new LinearLayout.LayoutParams(0, -2, 1);
            labelsParams.leftMargin = dp(12);
            card.addView(labels, labelsParams);
            org.json.JSONObject installedFap = appsMode == 0 ? null : nativeFapInfo(item.id);
            String realTitle = installedFap == null ? item.title :
                    installedFap.optString("title", item.title);
            TextView title = text(realTitle, 15, WHITE);
            title.setTypeface(Typeface.DEFAULT_BOLD);
            title.setMaxLines(2);
            labels.addView(title);
            String label = appsMode == 0 ? item.category : installedCategory(item.id, item.category);
            if (installedFap != null) {
                long size = installedFap.optLong("size", 0);
                long build = installedFap.optLong("version", 0);
                if (build > 0) label += " · Build " + build;
                if (size > 0) label += String.format(Locale.US, " · %.1f KiB", size / 1024.0);
            }
            TextView meta = text(label, 12, MUTED);
            meta.setPadding(0, dp(4), 0, 0);
            labels.addView(meta);

            LinearLayout right = new LinearLayout(this);
            right.setGravity(Gravity.END | Gravity.CENTER_VERTICAL);
            right.setOrientation(LinearLayout.HORIZONTAL);
            card.addView(right);

            int state = appStateOf(item.id);
            if (!connected) {
                ImageView warn = statusIcon(android.R.drawable.ic_dialog_alert, ORANGE);
                right.addView(warn, new LinearLayout.LayoutParams(dp(28), dp(28)));
            } else if (state == APP_UPDATE) {
                ImageView update = statusIcon(R.drawable.ic_apps_refresh, ORANGE);
                update.setContentDescription("Update " + item.title);
                update.setOnClickListener(v -> startMockDownload(item));
                right.addView(update, new LinearLayout.LayoutParams(dp(28), dp(28)));
                LinearLayout.LayoutParams playLp = new LinearLayout.LayoutParams(dp(28), dp(28));
                playLp.leftMargin = dp(10);
                right.addView(statusIcon(android.R.drawable.ic_media_play, ORANGE), playLp);
                LinearLayout.LayoutParams deleteLp = new LinearLayout.LayoutParams(dp(28), dp(28));
                deleteLp.leftMargin = dp(10);
                right.addView(statusIcon(android.R.drawable.ic_menu_delete, ORANGE), deleteLp);
            } else if (state == APP_INSTALLED) {
                right.addView(statusIcon(android.R.drawable.ic_media_play, ORANGE), new LinearLayout.LayoutParams(dp(28), dp(28)));
                LinearLayout.LayoutParams deleteLp = new LinearLayout.LayoutParams(dp(28), dp(28));
                deleteLp.leftMargin = dp(10);
                right.addView(statusIcon(android.R.drawable.ic_menu_delete, ORANGE), deleteLp);
            } else if (state == APP_DOWNLOADING || state == APP_QUEUED) {
                CardInstallRingView ring = new CardInstallRingView(item.id);
                right.addView(ring, new LinearLayout.LayoutParams(dp(32), dp(32)));
                ring.setOnClickListener(v -> openAppDetail(item));
                visibleAppInstallRings.put(normalizedAppId(item.id), ring);
                ring.update();
            } else {
                ImageView dl = statusIcon(android.R.drawable.stat_sys_download, ORANGE);
                dl.setContentDescription("Install " + item.title);
                dl.setOnClickListener(v -> startMockDownload(item));
                right.addView(dl, new LinearLayout.LayoutParams(dp(28), dp(28)));
            }
            card.setOnClickListener(v -> openAppDetail(item));
            LinearLayout.LayoutParams rowParams = new LinearLayout.LayoutParams(-1, -2);
            if (i > 0) rowParams.topMargin = dp(10);
            appsListHost.addView(card, rowParams);
        }
        if (displayCount < visible.size()) {
            TextView more = text("Load more  (" + displayCount + " / " + visible.size() + ")", 14, WHITE);
            more.setGravity(Gravity.CENTER);
            more.setPadding(dp(15), dp(18), dp(15), dp(18));
            more.setBackground(rounded(ORANGE_DARK, ORANGE, 12));
            LinearLayout.LayoutParams moreParams = new LinearLayout.LayoutParams(-1, -2);
            moreParams.topMargin = dp(12);
            appsListHost.addView(more, moreParams);
            more.setOnClickListener(v -> {
                int formerScroll = appsScrollView == null ? 0 : appsScrollView.getScrollY();
                appsVisibleLimit = Math.min(visible.size(), appsVisibleLimit + 80);
                renderAppsList();
                if (appsScrollView != null) appsScrollView.post(() -> appsScrollView.scrollTo(0, formerScroll));
            });
        }
    }

    private void showAppsMessage(String message) {
        TextView label = text(message, 14, MUTED);
        label.setGravity(Gravity.CENTER);
        label.setPadding(dp(12), dp(38), dp(12), dp(24));
        appsListHost.addView(label);
    }

    private boolean flipperIsConnected() {
        return prefs.getBoolean(WeaveService.KEY_ENABLED, false)
                && prefs.getBoolean(WeaveService.KEY_ACTIVE, false)
                && prefs.getBoolean(WeaveService.KEY_CONNECTED, false);
    }

    private static boolean isCatalogEntry(AppCatalogClient.Entry entry) {
        return entry != null && (entry.archiveId != null ||
                entry.manifestUrl != null || entry.pageUrl != null);
    }

    private void openAppDetail(AppCatalogClient.Entry entry) {
        appDetailEntry = entry;
        String key = normalizedAppId(entry.id);
        final int epoch = detailRefreshEpochs.getOrDefault(key, 0);
        if (appCatalogClient != null && isCatalogEntry(entry) &&
                !detailMetadataDone.contains(key) && detailMetadataRequests.add(key)) {
            appCatalogClient.enrichMetadata(entry, enriched -> {
                if (epoch != detailRefreshEpochs.getOrDefault(key, 0)) return;
                detailMetadataRequests.remove(key);
                detailMetadataDone.add(key);
                if (isFinishing() || isDestroyed()) return;
                if (enriched == null) {
                    if (appDetailEntry != null && normalizedAppId(appDetailEntry.id).equals(key))
                        renderAppDetail(appDetailEntry);
                    return;
                }
                for (int i = 0; i < catalogApps.size(); i++)
                    if (normalizedAppId(catalogApps.get(i).id).equals(key)) { catalogApps.set(i, enriched); break; }
                for (int i = 0; i < catalogSearchResults.size(); i++)
                    if (normalizedAppId(catalogSearchResults.get(i).id).equals(key)) {
                        catalogSearchResults.set(i, enriched); break;
                    }
                if (appDetailEntry != null && normalizedAppId(appDetailEntry.id).equals(key)) {
                    appDetailEntry = enriched;
                    renderAppDetail(enriched);
                }
            });
        }
        appsStickyBar.setVisibility(View.GONE);
        appsScrollView.scrollTo(0, 0);
        renderAppsList();
        appsListHost.setAlpha(0f);
        appsListHost.setTranslationY(dp(10));
        appsListHost.animate().alpha(1f).translationY(0f).setDuration(190).start();
    }

    private void closeAppDetail() {
        if (appDetailEntry == null) return;
        if (gestureRefreshActive && gestureRefreshPage == APPS &&
                gestureRefreshAppId != null) finishGestureRefresh();
        appDetailEntry = null;
        appsStickyBar.setVisibility(View.VISIBLE);
        appsScrollView.scrollTo(0, 0);
        renderAppsList();
    }

    private void openExternal(String url) {
        if (url == null || !url.startsWith("https://")) return;
        try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(url))); }
        catch (RuntimeException ignored) { toast("No browser available"); }
    }

    private void detailImage(ImageView target, String imageUrl, boolean icon) {
        if (appCatalogClient == null || imageUrl == null) return;
        if (!(target.getParent() instanceof ViewGroup)) return;
        ViewGroup parent = (ViewGroup) target.getParent();
        int at = parent.indexOfChild(target);
        ViewGroup.LayoutParams layout = target.getLayoutParams();
        parent.removeViewAt(at);
        FrameLayout holder = new FrameLayout(this);
        parent.addView(holder, at, layout);
        holder.addView(target, new FrameLayout.LayoutParams(-1, -1));
        target.setVisibility(View.INVISIBLE);
        ProgressBar loading = new ProgressBar(this);
        loading.setIndeterminateTintList(ColorStateList.valueOf(ORANGE));
        holder.addView(loading, new FrameLayout.LayoutParams(dp(24), dp(24), Gravity.CENTER));
        AppCatalogClient.IconCallback ready = bitmap -> {
            loading.setVisibility(View.GONE);
            target.setVisibility(View.VISIBLE);
            if (bitmap == null || target.getParent() == null) return;
            target.setImageTintList(null);
            target.setImageBitmap(bitmap);
            target.setScaleType(ImageView.ScaleType.FIT_CENTER);
        };
        if (icon) appCatalogClient.loadIcon(imageUrl, ready);
        else appCatalogClient.loadScreenshot(imageUrl, ready);
    }

    private TextView detailLink(String label, String url) {
        TextView link = text(label, 14, WHITE);
        link.setCompoundDrawablesWithIntrinsicBounds(0, 0, android.R.drawable.ic_menu_share, 0);
        link.setCompoundDrawableTintList(ColorStateList.valueOf(WHITE));
        link.setCompoundDrawablePadding(dp(8));
        link.getPaint().setUnderlineText(true);
        link.setPadding(0, dp(4), 0, dp(10));
        link.setOnClickListener(v -> openExternal(url));
        return link;
    }

    private LinearLayout linkRow(int iconRes, String label, String url) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        ImageView icon = statusIcon(iconRes, WHITE);
        row.addView(icon, new LinearLayout.LayoutParams(dp(20), dp(20)));
        TextView link = text(label, 14, WHITE);
        link.getPaint().setUnderlineText(true);
        link.setPadding(dp(10), 0, 0, dp(1));
        row.addView(link);
        row.setOnClickListener(v -> openExternal(url));
        link.setOnClickListener(v -> openExternal(url));
        return row;
    }

    private void roundImage(View view) {
        roundImage(view, 18);
    }

    private void roundImage(View view, int radiusDp) {
        if (Build.VERSION.SDK_INT >= 21) {
            final float radius = dp(radiusDp);
            view.setClipToOutline(true);
            view.setOutlineProvider(new ViewOutlineProvider() {
                @Override public void getOutline(View target, Outline outline) {
                    outline.setRoundRect(0, 0, target.getWidth(), target.getHeight(), radius);
                }
            });
        }
    }

    private String knownInstalledVersion(String appId) {
        String path = verifiedInstalledPath(appId);
        if (path == null) return null;
        try {
            org.json.JSONObject versions = new org.json.JSONObject(prefs.getString(
                    WeaveService.KEY_CATALOG_INSTALLED_VERSIONS, "{}"));
            org.json.JSONObject recorded = versions.optJSONObject(normalizedAppId(appId));
            if (recorded == null || !path.equals(recorded.optString("path", "")) ||
                    !recorded.optString("device", "").equals(prefs.getString(
                            WeaveService.KEY_CATALOG_ACTIVE_DEVICE, ""))) return null;
            String version = recorded.optString("version", "");
            return version.isEmpty() ? null : version;
        } catch (org.json.JSONException ignored) { return null; }
    }

    // Catalog versions can use 5.190 as well as 1.2.3; ambiguous tags are not updates.
    private boolean hasVerifiedUpdate(String appId, String published) {
        String installed = knownInstalledVersion(appId);
        if (installed == null || published == null) return false;
        String oldVersion = installed.replaceFirst("^[vV]", "");
        String newVersion = published.replaceFirst("^[vV]", "");
        if (!oldVersion.matches("\\d+(\\.\\d+){1,3}") ||
                !newVersion.matches("\\d+(\\.\\d+){1,3}")) return false;
        String[] oldParts = oldVersion.split("\\.");
        String[] newParts = newVersion.split("\\.");
        try {
            for (int i = 0; i < Math.max(oldParts.length, newParts.length); i++) {
                long a = i < oldParts.length ? Long.parseLong(oldParts[i]) : 0;
                long b = i < newParts.length ? Long.parseLong(newParts[i]) : 0;
                if (a != b) return b > a;
            }
        } catch (NumberFormatException ignored) { return false; }
        return false;
    }

    private String installedVersionOf(AppCatalogClient.Entry entry) {
        return entry == null ? null : knownInstalledVersion(entry.id);
    }

    private View toolAction(int iconRes, String label, int tint, View.OnClickListener onClick) {
        LinearLayout box = new LinearLayout(this);
        box.setOrientation(LinearLayout.VERTICAL);
        box.setGravity(Gravity.CENTER);
        box.setPadding(dp(4), 0, dp(4), 0);
        ImageView icon = statusIcon(iconRes, tint);
        box.addView(icon, new LinearLayout.LayoutParams(dp(24), dp(24)));
        TextView name = text(label, 10, tint);
        name.setGravity(Gravity.CENTER);
        name.setPadding(0, dp(4), 0, 0);
        box.addView(name);
        box.setOnClickListener(onClick);
        return box;
    }

    private CatalogMarkdown catalogMarkdown;

    private void renderCatalogMarkdown(TextView view, String content, AppCatalogClient.Entry entry) {
        if (catalogMarkdown == null) catalogMarkdown = new CatalogMarkdown(this);
        catalogMarkdown.render(view, content, entry);
    }

    private org.json.JSONObject nativeShopInfo(String id) {
        if(id == null) return null;
        try {
            org.json.JSONObject job = new org.json.JSONObject(
                    prefs.getString(WeaveService.KEY_SHOP_JOB, "{}"));
            return normalizedAppId(id).equals(normalizedAppId(job.optString("id"))) ? job : null;
        } catch(org.json.JSONException ignored) { return null; }
    }

    private org.json.JSONObject catalogQueueInfo(String id) {
        if (id == null) return null;
        String key = normalizedAppId(id);
        try {
            JSONArray queue = new JSONArray(prefs.getString(WeaveService.KEY_CATALOG_QUEUE, "[]"));
            for (int i = 0; i < queue.length(); i++) {
                org.json.JSONObject item = queue.optJSONObject(i);
                if (item != null && key.equals(normalizedAppId(item.optString("id")))) {
                    try { item.put("position", i + 1); } catch (org.json.JSONException ignored) { }
                    return item;
                }
            }
        } catch (org.json.JSONException ignored) { }
        return null;
    }
    private String catalogFailureFor(String id) {
        try {
            return new org.json.JSONObject(prefs.getString(WeaveService.KEY_CATALOG_FAILURES, "{}"))
                    .optString(normalizedAppId(id), null);
        } catch (org.json.JSONException ignored) { return null; }
    }
    private int appStateOf(String appId) {
        org.json.JSONObject nativeJob = nativeShopInfo(appId);
        if(nativeJob != null && ("downloading".equals(nativeJob.optString("phase")) ||
                "installing".equals(nativeJob.optString("phase")))) return APP_DOWNLOADING;
        org.json.JSONObject queued = catalogQueueInfo(appId);
        if (queued != null) return "queued".equals(queued.optString("phase")) ? APP_QUEUED : APP_DOWNLOADING;
        if (appId.equals(activeInstallId)) return APP_DOWNLOADING;
        if (inventoryReady() && installedIds().contains(normalizedAppId(appId))) {
            AppCatalogClient.Entry app = matchingCatalogApp(appId);
            return app != null && hasVerifiedUpdate(appId, app.version) ? APP_UPDATE : APP_INSTALLED;
        }
        Integer local = appStates.get(appId);
        return local != null && local == APP_INSTALLED ? APP_INSTALLED : APP_NOT_INSTALLED;
    }

    // Utilise le manifeste et la taille lus par le firmware, pas les métadonnées Lab.
    private org.json.JSONObject nativeFapInfo(String id) {
        String path = verifiedInstalledPath(id);
        if (path == null) return null;
        String active = prefs.getString(WeaveService.KEY_CATALOG_ACTIVE_DEVICE, "");
        if (active.isEmpty() || !active.equals(
                prefs.getString(WeaveService.KEY_FAP_METADATA_DEVICE, ""))) return null;
        try { return new org.json.JSONObject(prefs.getString(WeaveService.KEY_FAP_METADATA, "{}"))
                .optJSONObject(path); }
        catch (org.json.JSONException ignored) { return null; }
    }

    private Bitmap nativeFapIcon(String id) {
        org.json.JSONObject info = nativeFapInfo(id);
        if (info == null) return null;
        String hex = info.optString("icon", "");
        if (!hex.matches("[0-9a-fA-F]{64}")) return null;
        String key = verifiedInstalledPath(id) + ":" + hex;
        Bitmap cached = nativeFapIcons.get(key);
        if (cached != null) return cached;
        boolean[] bits = FapIconCodec.decode(hex);
        if (bits == null) return null;
        int[] pixels = new int[10 * 10];
        for (int i = 0; i < bits.length; i++)
            pixels[i] = bits[i] ? ORANGE : Color.TRANSPARENT;
        Bitmap raw = Bitmap.createBitmap(pixels, 10, 10, Bitmap.Config.ARGB_8888);
        // Agrandissement sans filtrage pour respecter les pixels 1-bit du Flipper.
        Bitmap result = Bitmap.createScaledBitmap(raw, 80, 80, false);
        raw.recycle();
        nativeFapIcons.put(key, result);
        return result;
    }

    private String installedCategory(String id, String fallback) {
        String path = verifiedInstalledPath(id);
        if (path == null) return fallback == null ? "Other" : fallback;
        int end = path.lastIndexOf('/');
        int start = path.lastIndexOf('/', end - 1);
        return start >= 0 ? path.substring(start + 1, end) : fallback;
    }

    private int categoryIcon(String id, String fallback) {
        String category = installedCategory(id, fallback).toLowerCase(Locale.ROOT);
        if (category.contains("usb")) return R.drawable.weave_cat_usb_16x16;
        if (category.contains("nfc")) return R.drawable.weave_cat_nfc_16x16;
        if (category.contains("rfid")) return R.drawable.weave_cat_rfid_16x16;
        if (category.contains("sub") || category.contains("radio")) return R.drawable.weave_cat_subghz_16x16;
        if (category.contains("infrared") || category.equals("ir")) return R.drawable.weave_cat_infrared_16x16;
        if (category.contains("bluetooth") || category.equals("ble")) return R.drawable.weave_cat_bluetooth_16x16;
        if (category.contains("gpio")) return R.drawable.weave_cat_gpio_16x16;
        if (category.contains("ibutton")) return R.drawable.weave_cat_ibutton_16x16;
        if (category.contains("game")) return R.drawable.weave_cat_games_16x16;
        if (category.contains("tool")) return R.drawable.weave_cat_tools_16x16;
        return R.drawable.weave_cat_other_16x16;
    }

    private void showAppIcon(AppCatalogClient.Entry item, ImageView image, ProgressBar loading) {
        Bitmap local = nativeFapIcon(item.id);
        if (local != null) {
            loading.setVisibility(View.GONE);
            image.setVisibility(View.VISIBLE);
            image.setImageTintList(null);
            image.setImageBitmap(local);
            image.setScaleType(ImageView.ScaleType.FIT_CENTER);
            return;
        }
        image.setImageResource(categoryIcon(item.id, item.category));
        image.setImageTintList(null);
        image.setScaleType(ImageView.ScaleType.FIT_CENTER);
        image.setVisibility(View.VISIBLE);
        if (appCatalogClient == null) { loading.setVisibility(View.GONE); return; }
        Bitmap ready = appCatalogClient.peekAppIcon(item);
        if (ready != null) {
            image.setImageBitmap(ready);
            loading.setVisibility(View.GONE);
            return;
        }
        appCatalogClient.loadIcon(item, bitmap -> {
            if (image.getParent() == null) return;
            loading.setVisibility(View.GONE);
            if (bitmap != null) {
                image.setImageBitmap(bitmap);
                image.setScaleType(ImageView.ScaleType.FIT_CENTER);
                if (flipperIsConnected() &&
                        System.currentTimeMillis() - lastShopIconSyncAt > 15000) {
                    handler.removeCallbacks(delayedShopIconSync);
                    handler.postDelayed(delayedShopIconSync, 4000);
                }
            }
        });
    }

    private String verifiedInstalledPath(String appId) {
        if (!inventoryReady()) return null;
        try {
            String path = new org.json.JSONObject(prefs.getString(WeaveService.KEY_INVENTORY_PATHS, "{}"))
                    .optString(normalizedAppId(appId), null);
            return path != null && path.startsWith("/ext/apps/") && path.endsWith(".fap") ? path : null;
        } catch (org.json.JSONException ignored) { return null; }
    }
    private void launchInstalledApp(AppCatalogClient.Entry entry) {
        if (!flipperIsConnected()) { toast("Flipper disconnected"); return; }
        String path = verifiedInstalledPath(entry.id);
        if (path == null) {
            toast("App path not verified. Refresh Installed apps to retry.");
            requestInstalledApps();
            return;
        }
        startService(new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_LAUNCH_APP)
                .putExtra("path", path));
    }
    private int appProgressOf(String appId) {
        org.json.JSONObject active = catalogQueueInfo(appId);
        // Once the HTTP FAP download reaches 100%, the horizontal bar loops
        // until the native Flipper GUI confirms its exit to desktop.
        if (active != null) return "downloading".equals(active.optString("phase")) ?
                active.optInt("percent", -1) : -1;
        Integer progress = appProgress.get(appId);
        return progress == null ? 0 : progress;
    }

    /** Uninstall only a path that the Flipper itself reported in its verified inventory. */
    private void confirmUninstallApp(AppCatalogClient.Entry entry) {
        if (!flipperIsConnected()) { toast("Flipper disconnected"); return; }
        if (pendingUninstall || activeInstallId != null ||
                !"[]".equals(prefs.getString(WeaveService.KEY_CATALOG_QUEUE, "[]"))) {
            toast("Wait for the installation queue to finish"); return;
        }
        if (!inventoryReady()) {
            requestInstalledApps();
            toast("Checking the installed .fap files…");
            return;
        }
        String path = null;
        try {
            org.json.JSONObject paths = new org.json.JSONObject(
                    prefs.getString(WeaveService.KEY_INVENTORY_PATHS, "{}"));
            path = paths.optString(normalizedAppId(entry.id), null);
        } catch (Exception ignored) { }
        if (path == null || !path.startsWith("/ext/apps/") || !path.endsWith(".fap")
                || path.contains("/../") || path.endsWith("/..") || path.contains("//")) {
            toast("App file not found on Flipper. Refresh Installed.");
            requestInstalledApps();
            return;
        }
        final String verifiedPath = path;
        new AlertDialog.Builder(this)
                .setTitle("Uninstall " + entry.title + "?")
                .setMessage("Remove this .fap from the Flipper SD card? App data will be kept.")
                .setNegativeButton("Cancel", null)
                .setPositiveButton("Uninstall", (dialog, button) -> {
                    if (!flipperIsConnected()) { toast("Flipper disconnected"); return; }
                    pendingUninstallId = entry.id;
                    pendingUninstall = true;
                    sendDeviceCommand(WeaveService.ACTION_DELETE_FILE, "path", verifiedPath);
                }).show();
    }

    /** All catalog installations and reinstalls are queued in WeaveService. */
    private void startMockDownload(AppCatalogClient.Entry entry) {
        if (!flipperIsConnected()) { toast("Flipper disconnected"); return; }
        if (entry == null || entry.id == null || !entry.id.matches("[A-Za-z0-9_-]{2,100}")) {
            toast("This app has no verified catalog identity"); return;
        }
        if (catalogQueueInfo(entry.id) != null) { toast("Already queued"); return; }
        Intent queue = new Intent(this, WeaveService.class)
                .setAction(WeaveService.ACTION_QUEUE_CATALOG_APP)
                .putExtra("id", entry.id)
                .putExtra("title", entry.title)
                .putExtra("category", entry.category)
                .putExtra("version", entry.version)
                .putExtra("archiveId", entry.archiveId);
        startService(queue);
        toast("Starting FAP download…");
    }

    private void resetAppDownload(AppCatalogClient.Entry entry) {
        activeInstallId = null;
        appStates.remove(entry.id);
        appProgress.remove(entry.id);
        if (appDetailEntry == entry) renderAppDetail(entry);
    }

    private void showOfficialBuildPicker(AppCatalogClient.Entry entry, List<String> apis, String verifiedApi) {
        if (isFinishing() || !entry.id.equals(activeInstallId)) return;
        String[] choices = new String[apis.size()];
        for (int i = 0; i < apis.size(); i++) {
            String api = apis.get(i);
            choices[i] = "Firmware API " + api + (api.equals(verifiedApi) ? " (device)" : "");
        }
        new AlertDialog.Builder(this)
                .setTitle("Choose a published .fap build")
                .setItems(choices, (dialog, index) -> {
                    String chosen = apis.get(index);
                    if (verifiedApi != null && !chosen.equals(verifiedApi)) {
                        new AlertDialog.Builder(this)
                                .setTitle("Compatibility not guaranteed")
                                .setMessage("Build API " + chosen + ", Flipper API " + verifiedApi +
                                        ". Install anyway? The firmware checks compatibility when launching.")
                                .setPositiveButton("Download", (d, which) -> performFapDownload(entry, chosen))
                                .setNegativeButton("Cancel", (d, which) -> resetAppDownload(entry))
                                .show();
                    } else performFapDownload(entry, chosen);
                })
                .setNegativeButton("Cancel", (dialog, which) -> resetAppDownload(entry))
                .setOnCancelListener(dialog -> resetAppDownload(entry))
                .show();
    }

    private static boolean isFapBinary(byte[] bytes) {
        return bytes != null && bytes.length > 32 && (bytes[0] & 0xff) == 0x7f
                && bytes[1] == 'E' && bytes[2] == 'L' && bytes[3] == 'F';
    }

    private void showFapImport() {
        if (!flipperIsConnected()) { toast("Connect your Flipper to import a .fap"); return; }
        new AlertDialog.Builder(this)
                .setTitle("Import .fap")
                .setItems(new String[]{"Choose a .fap file", "Download from an HTTPS URL"}, (dialog, option) -> {
                    if (option == 0) {
                        Intent select = new Intent(Intent.ACTION_OPEN_DOCUMENT);
                        select.setType("*/*");
                        select.addCategory(Intent.CATEGORY_OPENABLE);
                        try { startActivityForResult(select, REQUEST_FAP_IMPORT); }
                        catch (ActivityNotFoundException error) { toast("No file picker available"); }
                    } else showExternalFapUrlPrompt();
                }).show();
    }

    private void showExternalFapUrlPrompt() {
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setInputType(InputType.TYPE_CLASS_TEXT | InputType.TYPE_TEXT_VARIATION_URI);
        input.setHint("https://example.org/app.fap");
        input.setPadding(dp(20), dp(12), dp(20), dp(12));
        new AlertDialog.Builder(this)
                .setTitle("External .fap URL")
                .setMessage("Only use sources you trust. An external binary may be incompatible with the Flipper firmware.")
                .setView(input)
                .setPositiveButton("Download", (dialog, which) -> {
                    String url = input.getText().toString().trim();
                    appCatalogClient.downloadExternalFap(url, (bytes, error) -> {
                        if (bytes == null) toast(error == null ? "External download failed" : error);
                        else confirmExternalFap("external_app.fap", bytes);
                    });
                })
                .setNegativeButton("Cancel", null).show();
    }

    private void confirmExternalFap(String fileName, byte[] bytes) {
        if (!isFapBinary(bytes)) { toast("Not a valid ELF .fap file"); return; }
        if (!flipperIsConnected()) { toast("Flipper disconnected"); return; }
        if (activeInstallId != null) { toast("Another install is running"); return; }
        String base = fileName.replaceAll("(?i)\\.fap$", "").replaceAll("[^A-Za-z0-9_-]", "_");
        if (base.length() > 80) base = base.substring(0, 80);
        if (base.isEmpty()) base = "imported_app";
        final String safeBase = base;
        String[] folders = {"Tools", "Games", "GPIO", "USB", "NFC", "RFID", "Infrared", "Sub-GHz", "Bluetooth", "Media"};
        new AlertDialog.Builder(this)
                .setTitle("Install external .fap")
                .setMessage("The file is an ELF binary. Its origin and API compatibility are not verified by Weave. " +
                        "Choose the destination category; Flipper will check compatibility when launching.")
                .setItems(folders, (dialog, which) -> {
                    if (!flipperIsConnected()) { toast("Flipper disconnected"); return; }
                    activeInstallId = "manual:" + safeBase;
                    String cacheName = "fap_" + safeBase + ".bin";
                    try (FileOutputStream out = new FileOutputStream(new File(getCacheDir(), cacheName))) {
                        out.write(bytes);
                        startService(new Intent(MainActivity.this, WeaveService.class)
                                .setAction(WeaveService.ACTION_UPLOAD_FAP)
                                .putExtra("filename", cacheName)
                                .putExtra("path", "/ext/apps/" + folders[which] + "/" + safeBase + ".fap"));
                        toast("Uploading " + safeBase + ".fap…");
                    } catch (IOException | RuntimeException error) {
                        activeInstallId = null;
                        toast("Unable to prepare external .fap upload");
                    }
                }).show();
    }

    private void performFapDownload(AppCatalogClient.Entry entry, String deviceApi) {
        appCatalogClient.downloadAvailableOfficialFap(entry, deviceApi, (bytes, publishedApi, compatible, error) -> {
            if (!entry.id.equals(activeInstallId)) return;
            if (bytes == null) {
                resetAppDownload(entry);
                toast(error == null ? "Published .fap download unavailable" : error);
                return;
            }
            // The build may be for a different SDK. Do not interrupt each download
            // with speculative API-status notifications; firmware checks at launch.
            String folder = entry.category == null ? "Tools" : entry.category.replaceAll("[^A-Za-z0-9_-]", "");
            if (folder.isEmpty()) folder = "Tools";
            String baseName = entry.id.replaceAll("[^A-Za-z0-9_-]", "_");
            String cacheName = "fap_" + baseName + ".bin";
            try (FileOutputStream out = new FileOutputStream(new File(getCacheDir(), cacheName))) {
                out.write(bytes);
                Intent upload = new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_UPLOAD_FAP);
                upload.putExtra("filename", cacheName);
                upload.putExtra("path", "/ext/apps/" + folder + "/" + baseName + ".fap");
                startService(upload);
            } catch (IOException | RuntimeException failure) {
                resetAppDownload(entry);
                toast("FAP downloaded but could not be transferred to Flipper");
            }
        });
    }

    private void cancelMockDownload(String appId, boolean rerender) {
        if (appId.equals(activeInstallId))
            toast("The Flipper transfer cannot be canceled safely while writing");
    }

    private LinearLayout detailMetricCell(int iconRes, String iconLabel, String value) {
        LinearLayout cell = new LinearLayout(this);
        cell.setOrientation(LinearLayout.VERTICAL);
        cell.setGravity(Gravity.CENTER);
        ImageView icon = statusIcon(iconRes, WHITE);
        LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(34), dp(34));
        cell.addView(icon, iconParams);
        TextView top = text(iconLabel, 11, MUTED);
        top.setGravity(Gravity.CENTER);
        top.setPadding(0, dp(6), 0, 0);
        cell.addView(top);
        if (value != null && !value.isEmpty()) {
            TextView bottom = text(value, 12, WHITE);
            bottom.setGravity(Gravity.CENTER);
            bottom.setPadding(0, dp(2), 0, 0);
            cell.addView(bottom);
        }
        return cell;
    }

    private ImageButton detailActionButton(int iconRes, String label, View.OnClickListener action) {
        ImageButton button = new ImageButton(this);
        button.setImageResource(iconRes);
        button.setImageTintList(ColorStateList.valueOf(ORANGE));
        button.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        button.setBackgroundColor(Color.TRANSPARENT);
        button.setPadding(dp(8), dp(8), dp(8), dp(8));
        button.setContentDescription(label);
        button.setOnClickListener(action);
        return button;
    }

    private void renderAppDetail(AppCatalogClient.Entry entry) {
        handler.post(this::updateInstallRing);
        if (appsListHost == null) return;
        detailInstallBar = null;
        detailInstallPercent = null;
        detailInstallStatus = null;
        detailInstallId = null;
        appsListHost.removeAllViews();
        LinearLayout body = appsListHost;

        LinearLayout backRow = new LinearLayout(this);
        backRow.setGravity(Gravity.END);
        backRow.addView(toolAction(R.drawable.ic_apps_back, "Back", WHITE, v -> closeAppDetail()),
                new LinearLayout.LayoutParams(dp(56), dp(52)));
        body.addView(backRow);

        LinearLayout summary = new LinearLayout(this);
        summary.setOrientation(LinearLayout.VERTICAL);
        summary.setGravity(Gravity.CENTER_HORIZONTAL);
        LinearLayout.LayoutParams summaryParams = new LinearLayout.LayoutParams(-1, -2);
        summaryParams.topMargin = dp(0);
        body.addView(summary, summaryParams);

        LinearLayout topInfo = new LinearLayout(this);
        topInfo.setOrientation(LinearLayout.HORIZONTAL);
        topInfo.setGravity(Gravity.CENTER_VERTICAL);
        summary.addView(topInfo, new LinearLayout.LayoutParams(-1, -2));

        String installedVersion = flipperIsConnected() ? installedVersionOf(entry) : null;
        String latestVersion = entry.version == null ? null : entry.version.replaceFirst("^[vV]", "");
        String versionBlock = null;
        if (installedVersion != null && latestVersion != null) {
            versionBlock = installedVersion.equals(latestVersion) ? installedVersion : installedVersion + "\n↓\n" + latestVersion;
        } else if (latestVersion != null) {
            versionBlock = latestVersion;
        }
        org.json.JSONObject installedInfo = nativeFapInfo(entry.id);
        if (installedInfo != null && installedInfo.optLong("version", 0) > 0 &&
                installedVersion == null) {
            String nativeBuild = "Build " + installedInfo.optLong("version");
            versionBlock = latestVersion == null ? nativeBuild :
                    nativeBuild + "\nCatalog v" + latestVersion;
        }
        LinearLayout leftMetric = detailMetricCell(R.drawable.ic_nav_settings, "Version",
                versionBlock == null || versionBlock.trim().isEmpty() ? "Checking…" : versionBlock);
        topInfo.addView(leftMetric, new LinearLayout.LayoutParams(0, -2, 1));

        LinearLayout centerStack = new LinearLayout(this);
        centerStack.setOrientation(LinearLayout.VERTICAL);
        centerStack.setGravity(Gravity.CENTER_HORIZONTAL);
        topInfo.addView(centerStack, new LinearLayout.LayoutParams(0, -2, 1));
        ImageView icon = statusIcon(android.R.drawable.ic_menu_gallery, 0xffc5c5c5);
        icon.setVisibility(View.INVISIBLE);
        icon.setPadding(0, 0, 0, 0);
        icon.setBackground(rounded(CARD, BORDER, 14));
        roundImage(icon);
        FrameLayout detailIconWrap = new FrameLayout(this);
        centerStack.addView(detailIconWrap, new LinearLayout.LayoutParams(dp(80), dp(80)));
        detailIconWrap.addView(icon, new FrameLayout.LayoutParams(-1, -1));
        ProgressBar detailIconLoading = new ProgressBar(this);
        detailIconLoading.setIndeterminateTintList(ColorStateList.valueOf(ORANGE));
        detailIconWrap.addView(detailIconLoading,
                new FrameLayout.LayoutParams(dp(30), dp(30), Gravity.CENTER));
        showAppIcon(entry, icon, detailIconLoading);
        TextView title = text(entry.title, 19, WHITE);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);
        title.setPadding(0, dp(12), 0, 0);
        centerStack.addView(title);
        TextView cat = text(entry.category, 13, MUTED);
        cat.setGravity(Gravity.CENTER);
        cat.setPadding(0, dp(4), 0, 0);
        centerStack.addView(cat);

        long actualFapBytes = installedInfo == null ? 0 : installedInfo.optLong("size", 0);
        String actualSize = actualFapBytes > 0 ? String.format(Locale.US, "%.1f KiB",
                actualFapBytes / 1024.0) : "—";
        LinearLayout rightMetric = detailMetricCell(R.drawable.ic_apps_size, "Size", actualSize);
        topInfo.addView(rightMetric, new LinearLayout.LayoutParams(0, -2, 1));
        TextView rightMetricValue = null;
        if (rightMetric.getChildCount() > 0 && rightMetric.getChildAt(rightMetric.getChildCount() - 1) instanceof TextView)
            rightMetricValue = (TextView) rightMetric.getChildAt(rightMetric.getChildCount() - 1);
        final TextView sizeValueView = rightMetricValue;
        if (actualFapBytes <= 0 && appCatalogClient != null && sizeValueView != null)
            appCatalogClient.loadReleaseSize(entry, sizeText -> {
            if (appDetailEntry == entry && sizeValueView.getParent() != null && sizeText != null && !sizeText.isEmpty())
                sizeValueView.setText(sizeText);
        });

        LinearLayout.LayoutParams actionWrapParams = new LinearLayout.LayoutParams(-1, -2);
        actionWrapParams.topMargin = dp(10);
        LinearLayout actionWrap = new LinearLayout(this);
        actionWrap.setOrientation(LinearLayout.VERTICAL);
        actionWrap.setGravity(Gravity.CENTER);
        // A fixed action area prevents Repository/Manifest/Flipper Lab links from
        // jumping when the install button changes into a progress indicator.
        actionWrap.setMinimumHeight(dp(88));
        body.addView(actionWrap, actionWrapParams);
        // Historical RPC errors belong in Diagnostics, not the app detail.
        // Showing them permanently makes a fresh install look failed before it starts.
        org.json.JSONObject nativeJob = nativeShopInfo(entry.id);

        if (!flipperIsConnected()) {
            LinearLayout warningRow = new LinearLayout(this);
            warningRow.setGravity(Gravity.CENTER);
            ImageView warn = statusIcon(android.R.drawable.ic_dialog_alert, ORANGE);
            warningRow.addView(warn, new LinearLayout.LayoutParams(dp(24), dp(24)));
            TextView warning = text("Flipper disconnected", 15, ORANGE);
            warning.setTypeface(Typeface.DEFAULT_BOLD);
            warning.setPadding(dp(10), 0, 0, 0);
            warningRow.addView(warning);
            actionWrap.addView(warningRow);
        } else {
            int state = appStateOf(entry.id);
            if (state == APP_DOWNLOADING || state == APP_QUEUED) {
                org.json.JSONObject queued = catalogQueueInfo(entry.id);
                boolean nativeActive = nativeJob != null &&
                        ("downloading".equals(nativeJob.optString("phase")) ||
                         "installing".equals(nativeJob.optString("phase")));
                String phase = nativeActive ? nativeJob.optString("phase") :
                        queued == null ? "downloading" : queued.optString("phase", "queued");
                int nativeProgress = nativeActive && "downloading".equals(phase) ?
                        nativeJob.optInt("percent", -1) : -1;
                long nativeEta = nativeActive ? nativeJob.optLong("eta", -1) : -1;
                String label = nativeActive ? ("installing".equals(phase) ?
                        "Finishing on Flipper…" : "Downloading FAP…") : state == APP_QUEUED ? "Queued (#" + queued.optInt("position", 1) + ")" :
                        "awaiting_home".equals(phase) ? "Finishing on Flipper…" :
                        "verifying".equals(phase) ? "Verifying on Flipper…" :
                        "uploading".equals(phase) ? "Transferring to Flipper…" :
                        "opening_screen".equals(phase) ? "Opening install screen on Flipper…" :
                        "waiting".equals(phase) || "preparing".equals(phase) ? "Waiting for RPC…" : "Downloading…";
                TextView downloading = text(label, 13, ORANGE);
                downloading.setTypeface(Typeface.DEFAULT_BOLD);
                detailInstallStatus = downloading;
                LinearLayout.LayoutParams dlp = new LinearLayout.LayoutParams(-2, -2);
                dlp.gravity = Gravity.START;
                actionWrap.addView(downloading, dlp);
                LinearLayout progressRow = new LinearLayout(this);
                progressRow.setOrientation(LinearLayout.HORIZONTAL);
                progressRow.setGravity(Gravity.CENTER_VERTICAL);
                LinearLayout.LayoutParams pr = new LinearLayout.LayoutParams(-1, -2);
                pr.topMargin = dp(8);
                actionWrap.addView(progressRow, pr);
                ProgressBar progress = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
                progress.setIndeterminate(nativeActive ? nativeProgress < 0 || nativeProgress >= 100 :
                        !"downloading".equals(phase) || appProgressOf(entry.id) < 0 ||
                        appProgressOf(entry.id) >= 100);
                progress.setMax(100);
                progress.setProgress(nativeActive ? Math.max(0, nativeProgress) : appProgressOf(entry.id));
                detailInstallBar = progress;
                detailInstallId = entry.id;
                LinearLayout.LayoutParams pp = new LinearLayout.LayoutParams(0, dp(10), 1);
                progressRow.addView(progress, pp);
                TextView cancel = text("✕", 18, ORANGE);
                cancel.setPadding(dp(12), 0, 0, 0);
                cancel.setContentDescription("Cancel installation of " + entry.title);
                cancel.setOnClickListener(v -> {
                    if (nativeActive) { toast("Flipper-initiated job is managed by the device"); return; }
                    startService(new Intent(this, WeaveService.class)
                            .setAction(WeaveService.ACTION_CANCEL_QUEUED_APP)
                            .putExtra("id", entry.id));
                });
                progressRow.addView(cancel);
                TextView percent = text(nativeActive ? (nativeProgress < 0 ? "Finishing on Flipper…" :
                        nativeProgress + "%" + (nativeEta > 0 ? String.format(Locale.US,
                        " · ETA %02d:%02d", nativeEta / 60, nativeEta % 60) : "")) :
                        "downloading".equals(phase) ? (appProgressOf(entry.id) < 0 ?
                                "Fetching verified .fap…" : appProgressOf(entry.id) + "%") :
                        state == APP_QUEUED ? "Starting download…" :
                        "awaiting_home".equals(phase) ? "Finishing on Flipper…" :
                        "opening_screen".equals(phase) ? "Waiting for native Flipper display" :
                        ("waiting".equals(phase) || "preparing".equals(phase) ||
                         "uploading".equals(phase) || "backing_up".equals(phase) ||
                         "committing".equals(phase) || "verifying".equals(phase)) ?
                                "Finishing on Flipper…" :
                        appProgressOf(entry.id) < 0 ? "Finishing on Flipper…" :
                                appProgressOf(entry.id) + "%", 12, ORANGE);
                detailInstallPercent = percent;
                LinearLayout.LayoutParams pct = new LinearLayout.LayoutParams(-2, -2);
                pct.topMargin = dp(6);
                pct.gravity = Gravity.START;
                actionWrap.addView(percent, pct);
            } else if (state == APP_INSTALLED || state == APP_UPDATE) {
                LinearLayout actions = new LinearLayout(this);
                actions.setGravity(Gravity.CENTER);
                // Only display Update when an earlier Weave install was verified on
                // this exact path and the official catalog publishes a newer version.
                if (hasVerifiedUpdate(entry.id, entry.version)) {
                    ImageButton update = detailActionButton(android.R.drawable.ic_popup_sync,
                            "Update to " + entry.version, v -> startMockDownload(entry));
                    actions.addView(update, new LinearLayout.LayoutParams(dp(42), dp(42)));
                }
                ImageButton play = detailActionButton(android.R.drawable.ic_media_play, "Launch", v -> launchInstalledApp(entry));
                LinearLayout.LayoutParams playParams = new LinearLayout.LayoutParams(dp(42), dp(42));
                if (actions.getChildCount() > 0) playParams.leftMargin = dp(12);
                actions.addView(play, playParams);
                ImageButton delete = detailActionButton(android.R.drawable.ic_menu_delete, "Uninstall", v -> confirmUninstallApp(entry));
                LinearLayout.LayoutParams deleteParams = new LinearLayout.LayoutParams(dp(42), dp(42));
                deleteParams.leftMargin = dp(12);
                actions.addView(delete, deleteParams);
                actionWrap.addView(actions);
            } else {
                ImageButton download = detailActionButton(R.drawable.ic_apps_import, "Install", v -> startMockDownload(entry));
                actionWrap.addView(download, new LinearLayout.LayoutParams(dp(44), dp(44)));
            }
        }

        TextView galleryTitle = text("Screenshots", 15, WHITE);
        galleryTitle.setTypeface(Typeface.DEFAULT_BOLD);
        LinearLayout.LayoutParams gp = new LinearLayout.LayoutParams(-1, -2);
        gp.topMargin = dp(14);
        gp.bottomMargin = dp(10);
        body.addView(galleryTitle, gp);
        if (entry.screenshots.isEmpty()) {
            boolean waiting = isCatalogEntry(entry) &&
                    !detailMetadataDone.contains(normalizedAppId(entry.id));
            if (waiting) {
                ProgressBar spinner = new ProgressBar(this);
                spinner.setIndeterminateTintList(ColorStateList.valueOf(ORANGE));
                LinearLayout.LayoutParams sp = new LinearLayout.LayoutParams(dp(24), dp(24));
                sp.topMargin = dp(6);
                body.addView(spinner, sp);
            } else {
                TextView none = text("No screenshots available.", 13, MUTED);
                none.setPadding(0, dp(8), 0, dp(8));
                body.addView(none);
            }
        } else {
            HorizontalScrollView gallery = new HorizontalScrollView(this);
            gallery.setHorizontalScrollBarEnabled(false);
            LinearLayout thumbs = new LinearLayout(this);
            thumbs.setOrientation(LinearLayout.HORIZONTAL);
            gallery.addView(thumbs);
            for (String screenshot : entry.screenshots) {
                ImageView picture = statusIcon(android.R.drawable.ic_menu_gallery, MUTED);
                picture.setScaleType(ImageView.ScaleType.CENTER_CROP);
                picture.setBackground(rounded(CARD, BORDER, 12));
                roundImage(picture);
                LinearLayout.LayoutParams pp = new LinearLayout.LayoutParams(dp(224), dp(116));
                pp.rightMargin = dp(10);
                thumbs.addView(picture, pp);
                detailImage(picture, screenshot, false);
                final int shotIndex = entry.screenshots.indexOf(screenshot);
                picture.setOnClickListener(v -> openImageGallery(entry.screenshots, shotIndex < 0 ? 0 : shotIndex));
            }
            body.addView(gallery, new LinearLayout.LayoutParams(-1, dp(116)));
        }

        TextView descTitle = text("Description", 15, WHITE);
        descTitle.setTypeface(Typeface.DEFAULT_BOLD);
        LinearLayout.LayoutParams dpt = new LinearLayout.LayoutParams(-1, -2);
        dpt.topMargin = dp(26);
        dpt.bottomMargin = dp(8);
        body.addView(descTitle, dpt);
        TextView description = text("", 13, 0xffcccccc);
        description.setLineSpacing(dp(3), 1f);
        body.addView(description);
        String detailKey = normalizedAppId(entry.id);
        String cachedDescription = detailDescriptions.get(detailKey);
        String initialDescription = cachedDescription != null ? cachedDescription : entry.description;
        if (initialDescription != null && !initialDescription.trim().isEmpty()) {
            renderCatalogMarkdown(description, initialDescription, entry);
        } else if (isCatalogEntry(entry) && (!detailDescriptionDone.contains(detailKey) ||
                !detailMetadataDone.contains(detailKey))) {
            description.setText("Loading description…");
        } else {
            renderCatalogMarkdown(description, null, entry);
        }
        if (appCatalogClient != null && isCatalogEntry(entry) &&
                !detailDescriptionDone.contains(detailKey) &&
                detailDescriptionPending.add(detailKey)) {
            final int epoch = detailRefreshEpochs.getOrDefault(detailKey, 0);
            appCatalogClient.loadDescription(entry, desc -> {
                if (epoch != detailRefreshEpochs.getOrDefault(detailKey, 0)) return;
                detailDescriptionPending.remove(detailKey);
                detailDescriptionDone.add(detailKey);
                if (desc != null && !desc.trim().isEmpty()) detailDescriptions.put(detailKey, desc);
                // Met à jour la fiche active, même si les métadonnées ont remplacé la vue.
                if (appDetailEntry != null && normalizedAppId(appDetailEntry.id).equals(detailKey) &&
                        !isFinishing() && !isDestroyed()) renderAppDetail(appDetailEntry);
            });
        }

        LinearLayout footer = new LinearLayout(this);
        footer.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams fp = new LinearLayout.LayoutParams(-1, -2);
        fp.topMargin = dp(22);
        body.addView(footer, fp);
        if (entry.sourceUrl != null) footer.addView(linkRow(R.drawable.ic_link_github, "Repository", entry.sourceUrl));
        String manifest = entry.manifestUrl == null ? entry.pageUrl : entry.manifestUrl;
        if (manifest != null && !manifest.trim().isEmpty()) {
            LinearLayout.LayoutParams mp = new LinearLayout.LayoutParams(-1, -2);
            mp.topMargin = dp(10);
            View manifestRow = linkRow(R.drawable.ic_link_github, "Manifest", manifest);
            footer.addView(manifestRow, mp);
        }
        if (entry.archiveId != null && entry.archiveId.matches("[a-fA-F0-9]{24}") &&
                entry.pageUrl != null && entry.pageUrl.startsWith("https://catalog.flipperzero.one/application/")) {
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, -2);
            lp.topMargin = dp(10);
            LinearLayout official = new LinearLayout(this);
            official.setGravity(Gravity.CENTER_VERTICAL);
            ImageView officialIcon = new ImageView(this);
            officialIcon.setImageResource(R.mipmap.ic_launcher);
            official.addView(officialIcon, new LinearLayout.LayoutParams(dp(20), dp(20)));
            TextView officialText = text("Official catalog", 14, WHITE);
            officialText.getPaint().setUnderlineText(true);
            officialText.setPadding(dp(10), 0, 0, dp(1));
            official.addView(officialText);
            official.setOnClickListener(v -> openExternal(entry.pageUrl));
            footer.addView(official, lp);
        }
    }

    private void addDetailMetric(LinearLayout row, String label, String value) {
        LinearLayout cell = new LinearLayout(this);
        cell.setOrientation(LinearLayout.VERTICAL);
        cell.setGravity(Gravity.CENTER);
        row.addView(cell, new LinearLayout.LayoutParams(0, -2, 1));
        TextView head = text(label, 12, MUTED);
        cell.addView(head);
        TextView field = text(value, 15, WHITE);
        field.setTypeface(Typeface.DEFAULT_BOLD);
        field.setPadding(0, dp(5), 0, 0);
        cell.addView(field);
    }

    private void renderGalleryFrame(ImageView preview, FrameLayout imageFrame,
                                    TextView counter, List<String> urls, int index, int direction) {
        String url = urls.get(index);
        counter.setText((index + 1) + " / " + urls.size());
        // One visible image only: asynchronous callbacks from older swipes are ignored.
        preview.setTag(url);
        preview.animate().cancel();
        preview.setImageDrawable(null);
        for (int j = imageFrame.getChildCount() - 1; j >= 0; j--)
            if (imageFrame.getChildAt(j) instanceof ProgressBar) imageFrame.removeViewAt(j);
        ProgressBar imageLoading = new ProgressBar(this);
        imageLoading.setIndeterminateTintList(ColorStateList.valueOf(ORANGE));
        imageFrame.addView(imageLoading,
                new FrameLayout.LayoutParams(dp(32), dp(32), Gravity.CENTER));
        preview.setAlpha(0.55f);
        preview.setTranslationX(direction * dp(12));
        if (appCatalogClient != null) appCatalogClient.loadScreenshot(url, bitmap -> {
            if (!url.equals(preview.getTag()) || preview.getParent() == null) return;
            imageFrame.removeView(imageLoading);
            if (bitmap == null || bitmap.getWidth() <= 0 || bitmap.getHeight() <= 0) {
                preview.setImageResource(android.R.drawable.ic_menu_gallery);
                return;
            }
            // Size the image surface to the actual screenshot aspect ratio.
            android.util.DisplayMetrics metrics = getResources().getDisplayMetrics();
            int maxWidth = Math.max(dp(100), metrics.widthPixels - dp(26));
            int maxHeight = Math.max(dp(120), metrics.heightPixels - dp(150));
            double scale = Math.min((double) maxWidth / bitmap.getWidth(),
                    (double) maxHeight / bitmap.getHeight());
            FrameLayout.LayoutParams size = (FrameLayout.LayoutParams) imageFrame.getLayoutParams();
            size.width = Math.max(1, (int) Math.round(bitmap.getWidth() * scale));
            size.height = Math.max(1, (int) Math.round(bitmap.getHeight() * scale));
            imageFrame.setLayoutParams(size);
            preview.setImageTintList(null);
            preview.setImageBitmap(bitmap);
            preview.animate().alpha(1f).translationX(0f).setDuration(190).start();
        });
    }

    private void openImageGallery(List<String> urls, int startIndex) {
        if (urls == null || urls.isEmpty()) return;
        Dialog dialog = new Dialog(this);
        FrameLayout overlay = new FrameLayout(this);
        overlay.setBackgroundColor(0xed050505);
        overlay.setOnClickListener(v -> dialog.dismiss());

        FrameLayout imageFrame = new FrameLayout(this);
        imageFrame.setClickable(true); // Tapping the image must not close the gallery.
        FrameLayout.LayoutParams frameLp = new FrameLayout.LayoutParams(
                dp(240), dp(150), Gravity.CENTER);
        overlay.addView(imageFrame, frameLp);
        ImageView preview = statusIcon(android.R.drawable.ic_menu_gallery, MUTED);
        preview.setScaleType(ImageView.ScaleType.FIT_CENTER);
        preview.setBackgroundColor(Color.TRANSPARENT);
        imageFrame.addView(preview, new FrameLayout.LayoutParams(-1, -1));

        // Navigation arrows flank the screenshot, independent of its dimensions.
        TextView previous = text("‹", 38, WHITE);
        previous.setGravity(Gravity.CENTER);
        previous.setContentDescription("Previous screenshot");
        previous.setBackground(rounded(0xaa171719, 0x00000000, 22));
        FrameLayout.LayoutParams previousLp = new FrameLayout.LayoutParams(
                dp(42), dp(54), Gravity.START | Gravity.CENTER_VERTICAL);
        previousLp.leftMargin = dp(4);
        overlay.addView(previous, previousLp);
        TextView next = text("›", 38, WHITE);
        next.setGravity(Gravity.CENTER);
        next.setContentDescription("Next screenshot");
        next.setBackground(rounded(0xaa171719, 0x00000000, 22));
        FrameLayout.LayoutParams nextLp = new FrameLayout.LayoutParams(
                dp(42), dp(54), Gravity.END | Gravity.CENTER_VERTICAL);
        nextLp.rightMargin = dp(4);
        overlay.addView(next, nextLp);
        TextView counter = text("", 13, WHITE);
        counter.setGravity(Gravity.CENTER);
        FrameLayout.LayoutParams counterLp = new FrameLayout.LayoutParams(
                -2, dp(36), Gravity.BOTTOM | Gravity.CENTER_HORIZONTAL);
        counterLp.bottomMargin = dp(20);
        overlay.addView(counter, counterLp);
        final int[] current = {Math.max(0, Math.min(startIndex, urls.size() - 1))};
        Runnable refreshArrows = () -> {
            previous.setAlpha(current[0] == 0 ? .3f : 1f);
            next.setAlpha(current[0] == urls.size() - 1 ? .3f : 1f);
        };
        java.util.function.IntConsumer navigate = direction -> {
            int target = current[0] + direction;
            if (target < 0 || target >= urls.size()) return;
            current[0] = target;
            renderGalleryFrame(preview, imageFrame, counter, urls, target, direction);
            refreshArrows.run();
        };
        previous.setOnClickListener(v -> navigate.accept(-1));
        next.setOnClickListener(v -> navigate.accept(1));
        final float[] touchStart = new float[2];
        preview.setOnTouchListener((v, event) -> {
            if (event.getActionMasked() == android.view.MotionEvent.ACTION_DOWN) {
                touchStart[0] = event.getX();
                touchStart[1] = event.getY();
                return true;
            }
            if (event.getActionMasked() == android.view.MotionEvent.ACTION_UP) {
                float dx = event.getX() - touchStart[0];
                float dy = event.getY() - touchStart[1];
                if (Math.abs(dx) > dp(45) && Math.abs(dx) > Math.abs(dy) * 1.3f)
                    navigate.accept(dx < 0 ? 1 : -1); // Exactly one image per gesture.
                return true;
            }
            return true;
        });
        dialog.setContentView(overlay);
        if (dialog.getWindow() != null) {
            dialog.getWindow().setBackgroundDrawable(new ColorDrawable(Color.TRANSPARENT));
        }
        dialog.show();
        if (dialog.getWindow() != null) dialog.getWindow().setLayout(
                android.view.WindowManager.LayoutParams.MATCH_PARENT,
                android.view.WindowManager.LayoutParams.MATCH_PARENT);
        renderGalleryFrame(preview, imageFrame, counter, urls, current[0], 0);
        refreshArrows.run();
    }

    private int flipperKey(String label) {
        switch (label) {
            case "Up": return 0;
            case "Down": return 1;
            case "Right": return 2;
            case "Left": return 3;
            case "OK": return 4;
            case "Back": return 5;
            default: return -1;
        }
    }

    private void sendFlipperKey(String label) {
        if (remotePadLocked()) return;
        if (!flipperIsConnected()) { toast("Connect your Flipper first"); return; }
        int key = flipperKey(label);
        if (key >= 0) sendDeviceCommand(WeaveService.ACTION_PRESS_KEY, "key", key);
    }

    private void sendFlipperInput(int key, int type) {
        if (!flipperIsConnected() || (type != 1 && remotePadLocked())) return;
        Intent event = new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_KEY_EVENT);
        event.putExtra("key", key);
        event.putExtra("type", type);
        try { startService(event); }
        catch (RuntimeException unavailable) { /* Service ferme. */ }
    }

    private void captureScreen() {
        if (lastScreenBitmap == null || !flipperIsConnected()) {
            toast("No live Flipper screen available"); return;
        }
        Intent save = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        save.addCategory(Intent.CATEGORY_OPENABLE);
        save.setType("image/png");
        save.putExtra(Intent.EXTRA_TITLE, "flipper-screen.png");
        try { startActivityForResult(save, REQUEST_SCREEN_EXPORT); }
        catch (ActivityNotFoundException missingFileManager) { toast("No file manager available"); }
    }

    private void sendDeviceCommand(String action, String key, Object value) {
        if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false)) return;
        Intent intent = new Intent(this, WeaveService.class).setAction(action);
        if (value instanceof Integer) intent.putExtra(key, (int)value);
        else if (value instanceof String) intent.putExtra(key, (String)value);
        else if (value instanceof Boolean) intent.putExtra(key, (boolean)value);
        try { startService(intent); }
        catch (RuntimeException error) { toast("Flipper service unavailable"); }
    }

    private View appsPage() {
        LinearLayout root = pageRoot();
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        root.setPadding(dp(16), dp(6), dp(16), 0);

        // Search and tabs stay OUTSIDE the scrolling list, fixed below the app title.
        LinearLayout top = new BoundedColumn(this);
        appsStickyBar = top;
        top.setOrientation(LinearLayout.VERTICAL);
        root.addView(top, new LinearLayout.LayoutParams(-1, -2));

        LinearLayout searchBox = new LinearLayout(this);
        searchBox.setGravity(Gravity.CENTER_VERTICAL);
        searchBox.setOrientation(LinearLayout.HORIZONTAL);
        searchBox.setPadding(dp(12), dp(5), dp(5), dp(5));
        searchBox.setBackground(rounded(CARD, BORDER, 14));
        top.addView(searchBox, new LinearLayout.LayoutParams(-1, -2));

        searchBox.addView(statusIcon(android.R.drawable.ic_menu_search, MUTED), iconParams(20, 6));
        appsSearchInput = new EditText(this);
        appsSearchInput.setHint("Search app...");
        appsSearchInput.setHintTextColor(MUTED);
        appsSearchInput.setTextColor(WHITE);
        appsSearchInput.setTextSize(14);
        appsSearchInput.setBackgroundColor(Color.TRANSPARENT);
        appsSearchInput.setSingleLine(true);
        appsSearchInput.setInputType(InputType.TYPE_CLASS_TEXT);
        appsSearchInput.setPadding(0, dp(6), 0, dp(6));
        appsSearchInput.addTextChangedListener(new TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) { }
            @Override public void afterTextChanged(Editable s) {
                handler.removeCallbacks(delayedCatalogSearch);
                ++catalogSearchGeneration; // Cancel stale remote replies.
                if (appCatalogClient != null) appCatalogClient.cancelSearch();
                catalogSearchResults.clear();
                catalogSearchTerm = s == null ? "" : s.toString().trim();
                appsVisibleLimit = catalogSearchTerm.isEmpty() ? 40 : 24;
                catalogSearchLoading = appsMode == 0 && catalogSearchTerm.length() >= 2;
                catalogSearchError = null;
                // Keep IME input immediate; build cards only once typing settles.
                handler.removeCallbacks(delayedAppsFilter);
                handler.postDelayed(delayedAppsFilter, 240);
                if (catalogSearchLoading) handler.postDelayed(delayedCatalogSearch, 850);
            }
        });
        searchBox.addView(appsSearchInput, new LinearLayout.LayoutParams(0, dp(44), 1));

        // Filter is part of the search bar, never a separate side card.
        LinearLayout filter = new LinearLayout(this);
        filter.setOrientation(LinearLayout.VERTICAL);
        filter.setGravity(Gravity.CENTER);
        filter.setPadding(dp(4), 0, dp(4), 0);
        searchBox.addView(filter, new LinearLayout.LayoutParams(dp(56), dp(52)));
        appsFilterButton = new ImageButton(this);
        appsFilterButton.setImageResource(R.drawable.ic_apps_filter);
        appsFilterButton.setImageTintList(ColorStateList.valueOf(MUTED));
        appsFilterButton.setScaleType(ImageView.ScaleType.CENTER_INSIDE);
        appsFilterButton.setBackgroundColor(Color.TRANSPARENT);
        appsFilterButton.setPadding(dp(6), dp(6), dp(6), dp(6));
        filter.addView(appsFilterButton, new LinearLayout.LayoutParams(dp(29), dp(29)));
        appsFilterLabel = text("Filter", 10, MUTED);
        appsFilterLabel.setGravity(Gravity.CENTER);
        appsFilterLabel.setPadding(0, dp(4), 0, 0);
        filter.addView(appsFilterLabel);
        filter.setOnClickListener(v -> showAppsFilterDialog());
        appsFilterButton.setOnClickListener(v -> showAppsFilterDialog());

        LinearLayout options = new LinearLayout(this);
        options.setOrientation(LinearLayout.HORIZONTAL);
        options.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams optionsParams = new LinearLayout.LayoutParams(-1, -2);
        optionsParams.topMargin = dp(12);
        top.addView(options, optionsParams);

        LinearLayout tabsRow = new LinearLayout(this);
        tabsRow.setOrientation(LinearLayout.HORIZONTAL);
        options.addView(tabsRow, new LinearLayout.LayoutParams(0, -2, 1));
        appsModeTabs = new TextView[3];
        String[] names = {"Shop", "Installed", "Maj"};
        for (int i = 0; i < names.length; i++) {
            int index = i;
            TextView tab = text(names[i], 12, WHITE);
            tab.setTypeface(Typeface.DEFAULT_BOLD);
            tab.setGravity(Gravity.CENTER);
            tab.setPadding(dp(10), dp(12), dp(10), dp(12));
            LinearLayout.LayoutParams tabParams = new LinearLayout.LayoutParams(0, -2, 1);
            if (i > 0) tabParams.leftMargin = dp(6);
            tabsRow.addView(tab, tabParams);
            tab.setOnClickListener(v -> {
                appsMode = index;
                appsVisibleLimit = 40;
                updateAppsModeTabs();
                if (index > 0 && flipperIsConnected()) {
                    long now = android.os.SystemClock.elapsedRealtime();
                    if (now - lastInventoryUiRequestAt > 25000) {
                        lastInventoryUiRequestAt = now;
                        requestInstalledApps();
                    }
                }
                // Switching tabs should use the existing catalog, not start a
                // full network refresh on every return to Shop.
                if (index == 0 && catalogApps.isEmpty() && !catalogLoading)
                    requestCatalog(false);
                renderAppsList();
            });
            appsModeTabs[i] = tab;
        }
        LinearLayout.LayoutParams toolParams = new LinearLayout.LayoutParams(dp(56), dp(52));
        toolParams.leftMargin = dp(6);
        View refreshTab = toolAction(R.drawable.ic_apps_refresh, "Refresh", MUTED,
                v -> requestFullAppsRefresh());
        appsRefreshIcon = (ImageView) ((ViewGroup) refreshTab).getChildAt(0);
        options.addView(refreshTab, toolParams);
        if (appsRefreshInProgress) startAppsRefreshIconOnly();
        LinearLayout.LayoutParams toolParams2 = new LinearLayout.LayoutParams(dp(56), dp(52));
        toolParams2.leftMargin = dp(6);
        View importTab = toolAction(R.drawable.ic_apps_import, "Import", MUTED,
                v -> showFapImport());
        options.addView(importTab, toolParams2);

        // Only this area moves; header remains visible while browsing applications.
        ScrollView listScroller = new ScrollView(this);
        appsScrollView = listScroller;
        listScroller.setFillViewport(true);
        listScroller.setVerticalScrollBarEnabled(false);
        LinearLayout.LayoutParams scrollingParams = new LinearLayout.LayoutParams(-1, 0, 1);
        scrollingParams.topMargin = dp(6);
        root.addView(listScroller, scrollingParams);
        LinearLayout listOuter = new LinearLayout(this);
        listOuter.setOrientation(LinearLayout.VERTICAL);
        listOuter.setGravity(Gravity.CENTER_HORIZONTAL);
        listOuter.setPadding(0, 0, 0, dp(28));
        listScroller.addView(listOuter);
        appsListHost = new BoundedColumn(this);
        appsListHost.setOrientation(LinearLayout.VERTICAL);
        listOuter.addView(appsListHost, new LinearLayout.LayoutParams(-1, -2));

        updateAppsModeTabs();
        updateAppsFilterAccent();
        return root;
    }

    private View consolePage() {
        LinearLayout root = pageRoot();
        root.setGravity(Gravity.CENTER_HORIZONTAL);
        root.setPadding(dp(16), dp(8), dp(16), dp(14));

        BoundedColumn content = new BoundedColumn(this);
        content.setOrientation(LinearLayout.VERTICAL);
        root.addView(content, new LinearLayout.LayoutParams(-1, 0, 1));

        // Native buttons and bundled Lucide vector icons, matching the Terminal mockups.
        LinearLayout toolbar = new LinearLayout(this);
        toolbar.setOrientation(LinearLayout.HORIZONTAL);
        toolbar.setGravity(Gravity.CENTER_VERTICAL);
        content.addView(toolbar, new LinearLayout.LayoutParams(-1, dp(52)));

        LinearLayout modes = new LinearLayout(this);
        modes.setOrientation(LinearLayout.HORIZONTAL);
        modes.setGravity(Gravity.CENTER_VERTICAL);
        toolbar.addView(modes, new LinearLayout.LayoutParams(0, dp(46), 1));

        terminalCliTab = terminalModeButton("CLI", R.drawable.ic_terminal_cli);
        terminalLogsTab = terminalModeButton("LOGS", R.drawable.ic_terminal_logs);
        modes.addView(terminalCliTab, new LinearLayout.LayoutParams(0, dp(44), 1));
        LinearLayout.LayoutParams logsTabParams = new LinearLayout.LayoutParams(0, dp(44), 1);
        logsTabParams.leftMargin = dp(5);
        modes.addView(terminalLogsTab, logsTabParams);
        terminalCliTab.setOnClickListener(v -> setTerminalMode(0));
        terminalLogsTab.setOnClickListener(v -> setTerminalMode(1));

        LinearLayout.LayoutParams copyParams = new LinearLayout.LayoutParams(dp(50), dp(52));
        copyParams.leftMargin = dp(6);
        toolbar.addView(toolAction(R.drawable.ic_terminal_copy, "Copy", MUTED, v -> copyTerminal()), copyParams);
        LinearLayout.LayoutParams exportParams = new LinearLayout.LayoutParams(dp(50), dp(52));
        toolbar.addView(toolAction(R.drawable.ic_terminal_export, "Export", MUTED, v -> exportTerminal()), exportParams);
        LinearLayout.LayoutParams clearParams = new LinearLayout.LayoutParams(dp(50), dp(52));
        toolbar.addView(toolAction(R.drawable.ic_terminal_clear, "Clear", MUTED, v -> clearTerminal()), clearParams);

        terminalPanel = new FrameLayout(this);
        GradientDrawable panelBorder = rounded(0xff111111, BORDER, 28);
        panelBorder.setStroke(dp(2), BORDER);
        terminalPanel.setBackground(panelBorder);
        terminalPanel.setClipToOutline(true);
        terminalPanel.setPadding(dp(15), dp(16), dp(15), dp(16));
        LinearLayout.LayoutParams panelParams = new LinearLayout.LayoutParams(-1, 0, 1);
        panelParams.topMargin = dp(11);
        content.addView(terminalPanel, panelParams);
        updateTerminalTabs();
        return root;
    }

    private TextView terminalModeButton(String label, int iconRes) {
        TextView button = text(label, 12, WHITE);
        button.setTypeface(Typeface.DEFAULT_BOLD);
        button.setGravity(Gravity.CENTER);
        button.setCompoundDrawablesWithIntrinsicBounds(iconRes, 0, 0, 0);
        button.setCompoundDrawablePadding(dp(4));
        button.setPadding(dp(5), 0, dp(5), 0);
        button.setContentDescription(label + " tab");
        return button;
    }

    private void connectNativeCli() {
        if (cliBleConnected || cliUsbConnected) return;
        if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false)) { toast("Enable Service Weave first"); return; }
        if (flipperIsConnected()) {
            command(WeaveService.ACTION_CLI_START);
            return;
        }
        UsbDevice device = FlipperUsbCli.detect(this);
        if (device == null) { toast("Connect Flipper Zero with a USB cable"); return; }
        UsbManager manager = (UsbManager)getSystemService(USB_SERVICE);
        if (manager.hasPermission(device)) {
            command(WeaveService.ACTION_CLI_START);
            return;
        }
        if (cliPermissionPending) return;
        cliPermissionPending = true;
        pendingUsb = device;
        pendingCliPermission = true;
        PendingIntent permission = PendingIntent.getBroadcast(this, 0,
                new Intent(USB_PERMISSION).setPackage(getPackageName()),
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
        manager.requestPermission(device, permission);
        handler.postDelayed(() -> cliPermissionPending = false, 2500);
    }

    private void sendCliCommand(String command) {
        if (!cliUsbConnected && !cliBleConnected) { connectNativeCli(); return; }
        if (command == null || command.trim().isEmpty()) return;
        sendDeviceCommand(WeaveService.ACTION_CLI_SEND, "command", command);
    }

    private void setTerminalMode(int mode) {
        if (mode == terminalMode) return;
        terminalMode = mode;
        updateTerminalTabs();
        renderTerminalPanel(true);
    }

    private void updateTerminalTabs() {
        if (terminalCliTab == null || terminalLogsTab == null) return;
        TextView[] buttons = {terminalCliTab, terminalLogsTab};
        for (int i = 0; i < buttons.length; i++) {
            boolean active = terminalMode == i;
            TextView button = buttons[i];
            button.setTextColor(active ? ORANGE : WHITE);
            button.setCompoundDrawableTintList(ColorStateList.valueOf(active ? ORANGE : WHITE));
            GradientDrawable bg = rounded(active ? ORANGE_DARK : CARD, active ? ORANGE : 0xffbcbcbc, 11);
            bg.setStroke(dp(2), active ? ORANGE : 0xffbcbcbc);
            button.setBackground(bg);
        }
    }

    private String getTerminalLogs() {
        return prefs.getString(WeaveService.KEY_LOG, "");
    }

    private void copyTerminal() {
        String payload = terminalMode == 1 ? getTerminalLogs() : cliTranscript.toString();
        if (payload.trim().isEmpty()) {
            toast(terminalMode == 1 ? "No logs to copy" : "No CLI output available");
            return;
        }
        ClipboardManager clipboard = (ClipboardManager) getSystemService(CLIPBOARD_SERVICE);
        if (clipboard != null) {
            clipboard.setPrimaryClip(ClipData.newPlainText("Weave terminal", payload));
            toast("Terminal copied");
        }
    }

    private void exportTerminal() {
        String payload = terminalMode == 1 ? getTerminalLogs() : cliTranscript.toString();
        if (payload.trim().isEmpty()) {
            toast(terminalMode == 1 ? "No logs to export" : "No CLI output available");
            return;
        }
        Intent save = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        save.addCategory(Intent.CATEGORY_OPENABLE);
        save.setType("text/plain");
        save.putExtra(Intent.EXTRA_TITLE, terminalMode == 1 ? "weave-logs.txt" : "flipper-cli.txt");
        try { startActivityForResult(save, REQUEST_TERMINAL_EXPORT); }
        catch (ActivityNotFoundException missingFileManager) { toast("No file manager available"); }
    }

    @Override protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == REQUEST_ENABLE_BLUETOOTH) {
            if (resultCode == RESULT_OK && prefs.getBoolean(WeaveService.KEY_ENABLED, false)) {
                command(WeaveService.ACTION_SCAN);
                finishServiceSetup();
            }
            return;
        }
        if (requestCode == REQUEST_UPDATE_APK_PERMISSION) {
            apkUpdateRequest = 0;
            if (pendingUpdateApk == null) { clearApkUpdate(); return; }
            if (getPackageManager().canRequestPackageInstalls()) {
                apkReadyToInstall = true;
                if (appForeground) openApkInstaller();
            } else offerApkRecovery("Installation permission not granted.");
            return;
        }
        if (requestCode == REQUEST_UPDATE_APK_INSTALL) {
            apkUpdateRequest = 0;
            if (resultCode == RESULT_OK) {
                clearApkUpdate(); // Android usually restarts the updated process.
                toast("Installer completed");
            } else offerApkRecovery("Installation cancelled or refused by Android.");
            return;
        }
        if (requestCode == REQUEST_UPDATE_APK_SAVE) {
            File apk = pendingUpdateApk;
            apkUpdateRequest = 0;
            if (resultCode != RESULT_OK || data == null || data.getData() == null ||
                    apk == null || !apk.isFile()) { clearApkUpdate(); return; }
            final Uri destination = data.getData();
            new Thread(() -> {
                try (InputStream source = new java.io.FileInputStream(apk);
                     OutputStream output = getContentResolver().openOutputStream(destination, "w")) {
                    if (output == null) throw new IOException("Cannot save selected APK");
                    byte[] buffer = new byte[32768];
                    int count;
                    while ((count = source.read(buffer)) != -1) output.write(buffer, 0, count);
                    output.flush();
                    handler.post(() -> {
                        clearApkUpdate();
                        if (isFinishing() || isDestroyed()) return;
                        new AlertDialog.Builder(this)
                                .setTitle("APK saved")
                                .setMessage("Open the saved APK from Files in this Android profile. " +
                                        "If the signing key is incompatible, back up your settings, " +
                                        "uninstall Weave from this profile only, then open the saved APK. " +
                                        "Do not choose Uninstall for all users. Uninstalling erases this profile's app data. " +
                                        "Android may refuse a different signing key if Weave remains installed in another profile.")
                                .setPositiveButton("OK", null).show();
                    });
                } catch (Exception failure) {
                    handler.post(() -> {
                        clearApkUpdate();
                        toast("APK save failed: " + failure.getMessage());
                    });
                }
            }, "Weave-apk-export").start();
            return;
        }
        if (requestCode == REQUEST_FILE_EXPORT) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null && exportedFile != null) {
                try (OutputStream output = getContentResolver().openOutputStream(data.getData())) {
                    if (output == null) throw new IOException("Cannot create file");
                    output.write(exportedFile);
                    toast("File exported");
                } catch (IOException error) { toast("Export failed"); }
            }
            exportedFile = null;
            return;
        }
        if (requestCode == REQUEST_FILE_EXPORT_ARCHIVE) {
            if (resultCode == RESULT_OK && data != null && data.getData() != null && archiveExportSession != null) {
                startArchiveExport(data.getData());
            } else archiveExportSession = null;
            return;
        }
        if (requestCode == REQUEST_FAP_IMPORT) {
            if (resultCode != RESULT_OK || data == null || data.getData() == null) return;
            Uri uri = data.getData();
            String name = "imported_app.fap";
            try (Cursor cursor = getContentResolver().query(uri,
                    new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) name = cursor.getString(0);
            } catch (RuntimeException ignored) { }
            if (name == null || !name.toLowerCase(Locale.ROOT).endsWith(".fap")) {
                toast("Choose a .fap file"); return;
            }
            try (InputStream in = getContentResolver().openInputStream(uri)) {
                if (in == null) throw new IOException("File unavailable");
                ByteArrayOutputStream out = new ByteArrayOutputStream();
                byte[] buffer = new byte[8192];
                int n;
                while ((n = in.read(buffer)) != -1) {
                    if (out.size() + n > 2_000_000) throw new IOException("File exceeds 2 MiB");
                    out.write(buffer, 0, n);
                }
                confirmExternalFap(name, out.toByteArray());
            } catch (IOException | RuntimeException error) {
                toast("Cannot import .fap: " + error.getMessage());
            }
            return;
        }
        if (requestCode == REQUEST_FILE_IMPORT) {
            if (resultCode != RESULT_OK || data == null || data.getData() == null) return;
            Uri uri = data.getData();
            String name = null;
            try (Cursor cursor = getContentResolver().query(uri,
                    new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
                if (cursor != null && cursor.moveToFirst()) name = cursor.getString(0);
            } catch (RuntimeException ignored) { }
            if (!FilesBrowserState.validName(name)) { toast("Invalid file name"); return; }
            String safeName = "upload_" + System.currentTimeMillis() + ".bin";
            try (InputStream input = getContentResolver().openInputStream(uri);
                 FileOutputStream output = new FileOutputStream(new File(getCacheDir(), safeName))) {
                if (input == null) throw new IOException("Cannot open file");
                byte[] block = new byte[8192];
                int total = 0, count;
                while ((count = input.read(block)) != -1) {
                    total += count;
                    if (total > 2_000_000) throw new IOException("File exceeds 2 MiB");
                    output.write(block, 0, count);
                }
                if (total == 0) throw new IOException("Empty file");
                Intent upload = new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_UPLOAD_FILE)
                        .putExtra("filename", safeName)
                        .putExtra("path", filesState.path + "/" + name);
                startService(upload);
            } catch (IOException | RuntimeException error) { toast("Unable to import file: " + error.getMessage()); }
            return;
        }
        if (requestCode == REQUEST_SCREEN_EXPORT) {
            if (resultCode != RESULT_OK || data == null || data.getData() == null || lastScreenBitmap == null) return;
            try (OutputStream out = getContentResolver().openOutputStream(data.getData())) {
                if (out == null || !lastScreenBitmap.compress(Bitmap.CompressFormat.PNG, 100, out))
                    throw new IOException("Unable to save screenshot");
                toast("Flipper screen saved");
            } catch (IOException error) { toast("Screenshot export failed"); }
            return;
        }
        if (requestCode != REQUEST_TERMINAL_EXPORT || resultCode != RESULT_OK || data == null || data.getData() == null) return;
        String log = terminalMode == 1 ? getTerminalLogs() : cliTranscript.toString();
        try (OutputStream output = getContentResolver().openOutputStream(data.getData())) {
            if (output == null) throw new IOException("Unable to create file");
            output.write(log.getBytes(StandardCharsets.UTF_8));
            toast("Logs exported");
        } catch (IOException error) {
            toast("Export failed");
        }
    }

    private void clearTerminal() {
        if (terminalMode == 1) prefs.edit().putString(WeaveService.KEY_LOG, "").apply();
        else cliTranscript.setLength(0);
        renderTerminalPanel(true);
    }

    private void renderTerminalPanel(boolean force) {
        if (terminalPanel == null) return;
        boolean connected = flipperIsConnected();
        String logs = getTerminalLogs();
        if (!force && connected == terminalLastConnected && terminalLastMode == terminalMode
                && (terminalMode == 0 || logs.equals(terminalLastLog))) return;
        terminalLastConnected = connected;
        terminalLastLog = logs;
        terminalLastMode = terminalMode;
        terminalPanel.removeAllViews();
        if (!connected && terminalMode == 1) {
            LinearLayout offline = new LinearLayout(this);
            offline.setOrientation(LinearLayout.VERTICAL);
            offline.setGravity(Gravity.CENTER);
            ImageView icon = statusIcon(R.drawable.ic_terminal_warning, 0xffd0d0d0);
            offline.addView(icon, new LinearLayout.LayoutParams(dp(64), dp(64)));
            TextView message = text("Flipper\ndisconnected", 23, 0xffeeeeee);
            message.setGravity(Gravity.CENTER);
            message.setTypeface(Typeface.DEFAULT_BOLD);
            LinearLayout.LayoutParams messageParams = new LinearLayout.LayoutParams(-1, -2);
            messageParams.topMargin = dp(14);
            offline.addView(message, messageParams);
            terminalPanel.addView(offline, new FrameLayout.LayoutParams(-1, -2, Gravity.CENTER));
            return;
        }
        if (terminalMode == 0) {
            LinearLayout cli = new LinearLayout(this);
            cli.setOrientation(LinearLayout.VERTICAL);
            TextView status = text(cliBleConnected ? "Flipper BLE CLI" :
                    (cliUsbConnected ? "Flipper USB CLI" : "Connecting to Flipper CLI…"), 13,
                    (cliUsbConnected || cliBleConnected) ? ORANGE : MUTED);
            status.setTypeface(Typeface.MONOSPACE);
            cli.addView(status);
            ScrollView consoleScroll = new PinchScrollView(this);
            consoleScroll.setFillViewport(true);
            LinearLayout.LayoutParams slp = new LinearLayout.LayoutParams(-1, 0, 1);
            slp.topMargin = dp(10);
            cli.addView(consoleScroll, slp);
            cliOutput = text(cliTranscript.length() == 0
                    ? "$ Connecting to your Flipper terminal…"
                    : cliTranscript.toString(), 12, 0xffeeeeee);
            cliOutput.setTypeface(Typeface.MONOSPACE);
            cliOutput.setTextSize(TypedValue.COMPLEX_UNIT_SP, terminalFontSp);
            cliOutput.setTextIsSelectable(true);
            cliOutput.setLineSpacing(dp(2), 1f);
            consoleScroll.addView(cliOutput);
            LinearLayout promptRow = new LinearLayout(this);
            promptRow.setGravity(Gravity.CENTER_VERTICAL);
            GradientDrawable promptBg = rounded(0xff0b0b0b, 0xff2d2d2d, 12);
            promptBg.setStroke(dp(1), 0xff2d2d2d);
            promptRow.setBackground(promptBg);
            promptRow.setPadding(dp(10), 0, dp(10), 0);
            LinearLayout.LayoutParams inputRow = new LinearLayout.LayoutParams(-1, dp(44));
            inputRow.topMargin = dp(8);
            cli.addView(promptRow, inputRow);
            TextView prefix = text("$", 14, ORANGE);
            prefix.setTypeface(Typeface.MONOSPACE);
            promptRow.addView(prefix);
            EditText input = new EditText(this);
            input.setSingleLine(true);
            input.setBackgroundColor(Color.TRANSPARENT);
            input.setTextColor(WHITE);
            input.setHintTextColor(MUTED);
            input.setHint(cliUsbConnected || cliBleConnected ? "Enter a Flipper command" : "CLI reconnecting…");
            input.setTextSize(13);
            input.setTypeface(Typeface.MONOSPACE);
            input.setEnabled(cliUsbConnected || cliBleConnected);
            input.setImeOptions(android.view.inputmethod.EditorInfo.IME_ACTION_SEND);
            LinearLayout.LayoutParams inputParams = new LinearLayout.LayoutParams(0, -1, 1);
            inputParams.leftMargin = dp(8);
            promptRow.addView(input, inputParams);
            input.setOnEditorActionListener((view, actionId, event) -> {
                String value = input.getText().toString();
                input.setText("");
                sendCliCommand(value);
                return true;
            });
            input.setOnFocusChangeListener((view, hasFocus) -> {
                if (hasFocus && !(cliUsbConnected || cliBleConnected)) connectNativeCli();
            });
            terminalPanel.addView(cli, new FrameLayout.LayoutParams(-1, -1));
            return;
        }
        ScrollView scroll = new PinchScrollView(this);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(true);
        LinearLayout rows = new LinearLayout(this);
        rows.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(rows);
        terminalPanel.addView(scroll, new FrameLayout.LayoutParams(-1, -1));
        String[] lines = logs.split("\r?\n");
        int total = 0;
        for (int i = lines.length - 1; i >= 0; i--) {
            String line = lines[i].trim();
            if (line.isEmpty()) continue;
            addTerminalLogRow(rows, line);
            if (++total == 100) break;
        }
        if (total == 0) {
            TextView empty = text("No logs yet", 14, MUTED);
            empty.setGravity(Gravity.CENTER);
            rows.addView(empty, new LinearLayout.LayoutParams(-1, -1));
        }
    }

    private void addTerminalLogRow(LinearLayout rows, String entry) {
        Matcher match = Pattern.compile("^(\\d{2}:\\d{2}:\\d{2})\\s+(.*)$").matcher(entry);
        String time = match.matches() ? match.group(1) : "";
        String content = match.matches() ? match.group(2) : entry;
        String lower = content.toLowerCase(Locale.ROOT);
        boolean warning = lower.contains("failed") || lower.contains("error") || lower.contains("permission required")
                || lower.contains("no weave") || lower.contains("unavailable") || lower.contains("disconnected")
                || lower.contains("denied");
        LinearLayout row = new LinearLayout(this);
        row.setOrientation(LinearLayout.HORIZONTAL);
        row.setPadding(dp(3), dp(11), dp(3), dp(11));
        rows.addView(row, new LinearLayout.LayoutParams(-1, -2));

        LinearLayout meta = new LinearLayout(this);
        meta.setOrientation(LinearLayout.VERTICAL);
        meta.setGravity(Gravity.START);
        row.addView(meta, new LinearLayout.LayoutParams(dp(78), -2));
        TextView severity = text(warning ? "WARN" : "INFO", 10, warning ? ORANGE : WHITE);
        severity.setGravity(Gravity.CENTER);
        GradientDrawable pill = rounded(0xff111111, warning ? ORANGE : 0xff9a9a9a, 5);
        severity.setBackground(pill);
        severity.setPadding(dp(5), dp(2), dp(5), dp(2));
        meta.addView(severity, new LinearLayout.LayoutParams(dp(51), -2));
        TextView timestamp = text(time, 10, MUTED);
        timestamp.setTypeface(Typeface.MONOSPACE);
        timestamp.setPadding(0, dp(7), 0, 0);
        meta.addView(timestamp);

        TextView message = text(content, 12, 0xffeeeeee);
        message.setTypeface(Typeface.MONOSPACE);
        message.setTextSize(TypedValue.COMPLEX_UNIT_SP, terminalFontSp);
        message.setTag(Boolean.TRUE);
        message.setTextIsSelectable(true);
        message.setLineSpacing(dp(3), 1f);
        row.addView(message, new LinearLayout.LayoutParams(0, -2, 1));
        View rule = new View(this);
        rule.setBackgroundColor(0xff4b4b4b);
        rows.addView(rule, new LinearLayout.LayoutParams(-1, dp(1)));
    }

    private void showFirmwareRecoveryGuide() {
        new AlertDialog.Builder(this)
                .setTitle("Firmware Recovery (DFU)")
                .setMessage("On the Flipper: Settings > Power > Reboot > Firmware Upgrade. " +
                        "Connect USB to your PC and repair with qFlipper.\n\n" +
                        "If the firmware is frozen: hold LEFT + BACK for 5 seconds, " +
                        "then release BACK while holding LEFT until the blue LED lights up.\n\n" +
                        "DFU disconnects Bluetooth. Firmware repair erases internal settings " +
                        "but preserves the microSD card.")
                .setNegativeButton("Close", null)
                .setNeutralButton("Enter DFU", (dialog, which) -> {
                    if (!flipperIsConnected()) {
                        toast("Connect the Flipper first, or use the physical DFU buttons");
                        return;
                    }
                    new AlertDialog.Builder(this)
                            .setTitle("Enter DFU on Flipper?")
                            .setMessage("This will reboot the Flipper into DFU and immediately " +
                                    "disconnect Bluetooth/USB RPC. A PC with USB and qFlipper is " +
                                    "needed to repair the firmware. Do not start during a FAP transfer.")
                            .setNegativeButton("Cancel", null)
                            .setPositiveButton("Reboot to DFU", (confirm, whichButton) ->
                                    command(WeaveService.ACTION_ENTER_DFU))
                            .show();
                })
                .setPositiveButton("Official guide", (dialog, which) -> {
                    try { startActivity(new Intent(Intent.ACTION_VIEW, Uri.parse(
                            "https://docs.flipper.net/zero/basics/firmware-update/firmware-recovery"))); }
                    catch (RuntimeException error) { toast("Guide unavailable"); }
                }).show();
    }

    private View settingsPage() {
        settingsHost = new FrameLayout(this);
        showSettingsMain();
        return settingsHost;
    }

    private void showSettingsMain() {
        settingsInfoOpen = false;
        settingsLastConnected = flipperIsConnected();
        settingsLastTransport = prefs.getString(WeaveService.KEY_TRANSPORT, "");
        settingsLastFirmware = prefs.getString(WeaveService.KEY_FIRMWARE, "");
        if (settingsHost == null) return;
        settingsHost.removeAllViews();
        LinearLayout root = pageRoot();
        root.setPadding(dp(18), dp(6), dp(18), dp(24));

        LinearLayout settings = card(false);
        settings.setPadding(dp(15), dp(10), dp(15), dp(10));
        root.addView(settings, cardParams());
        serviceSwitch = settingsToggle(settings, R.drawable.ic_nav_settings,
                "Service Weave", "Allow Flipper to access the Internet", (button, on) -> {
                    if (syncing) return;
                    prefs.edit().putBoolean(WeaveService.KEY_GATEWAY, on).apply();
                    toggleService(on);
                });
        divider(settings);
        bootSwitch = settingsToggle(settings, R.drawable.ic_settings_power,
                "Start on boot", "Start the Weave service at startup", (button, on) -> {
                    if (syncing) return;
                    prefs.edit().putBoolean(WeaveService.KEY_BOOT, on).apply();
                    if (on) {
                        if (!hasBlePermission()) requestBlePermission();
                        else if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false)) showBatteryGuidance();
                        if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false))
                            toast("Enable Service Weave as well for automatic startup");
                    }
                });
        divider(settings);
        // Opt-in only. Android notification access is required for incoming
        // calls and message events; no message body or contact is forwarded.
        Switch alertSwitch = settingsToggle(settings, R.drawable.ic_settings_alert_bell,
                "Calls & SMS alerts", "Mirror call and SMS alerts to Flipper", (button, on) -> {
                    if (syncing) return;
                    prefs.edit().putBoolean("alerts_bridge_enabled", on).apply();
                    if (on) {
                        toast("Allow notification access and phone state for call-end detection");
                        java.util.ArrayList<String> missing = new java.util.ArrayList<>();
                        if(Build.VERSION.SDK_INT >= 23 &&
                                checkSelfPermission(Manifest.permission.READ_PHONE_STATE) !=
                                PackageManager.PERMISSION_GRANTED)
                            missing.add(Manifest.permission.READ_PHONE_STATE);
                        if(Build.VERSION.SDK_INT >= 23 &&
                                checkSelfPermission(Manifest.permission.RECEIVE_SMS) !=
                                PackageManager.PERMISSION_GRANTED)
                            missing.add(Manifest.permission.RECEIVE_SMS);
                        if(!missing.isEmpty())
                            requestPermissions(missing.toArray(new String[0]), REQUEST_PHONE_STATE);
                        else startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS));
                    }
                });
        syncing = true;
        alertSwitch.setChecked(prefs.getBoolean("alerts_bridge_enabled", false));
        syncing = false;
        divider(settings);
        LinearLayout finder = new LinearLayout(this);
        finder.setGravity(Gravity.CENTER_VERTICAL);
        finder.setPadding(0, dp(15), 0, dp(15));
        boolean finderAvailable = flipperIsConnected() &&
                "Bluetooth".equalsIgnoreCase(prefs.getString(WeaveService.KEY_TRANSPORT, ""));
        finder.addView(statusIcon(R.drawable.ic_settings_find_flipper,
                finderAvailable ? WHITE : MUTED),
                new LinearLayout.LayoutParams(dp(27), dp(27)));
        LinearLayout finderLabels = new LinearLayout(this);
        finderLabels.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams finderParams = new LinearLayout.LayoutParams(0, -2, 1);
        finderParams.leftMargin = dp(14);
        finder.addView(finderLabels, finderParams);
        TextView finderTitle = text("Find my Flipper", 15, WHITE);
        finderTitle.setTypeface(Typeface.DEFAULT_BOLD);
        finderLabels.addView(finderTitle);
        finderLabels.addView(text(finderAvailable
                ? "Ring and vibrate your connected Flipper"
                : flipperIsConnected() ? "Requires Flipper Bluetooth RPC"
                : "Connect a Flipper to enable", 11, MUTED));
        finder.addView(statusIcon(R.drawable.ic_files_chevron_right, MUTED),
                new LinearLayout.LayoutParams(dp(20), dp(20)));
        finder.setEnabled(finderAvailable);
        finder.setAlpha(finderAvailable ? 1f : 0.44f);
        finder.setOnClickListener(v -> {
            if (!flipperIsConnected()) return;
            new android.app.AlertDialog.Builder(this)
                    .setTitle("Find my Flipper")
                    .setMessage("Play a loud ring and vibrate the connected Flipper?")
                    .setNegativeButton("Cancel", null)
                    .setPositiveButton("Ring", (d, which) -> command(WeaveService.ACTION_FIND_FLIPPER))
                    .show();
        });
        settings.addView(finder);
        divider(settings);
        action(settings, R.drawable.ic_settings_recovery, "Firmware Recovery", "DFU repair via USB and qFlipper",
                v -> showFirmwareRecoveryGuide());
        divider(settings);
        LinearLayout info = new LinearLayout(this);
        info.setGravity(Gravity.CENTER_VERTICAL);
        info.setPadding(0, dp(15), 0, dp(15));
        ImageView infoIcon = statusIcon(R.drawable.ic_settings_info, WHITE);
        info.addView(infoIcon, new LinearLayout.LayoutParams(dp(27), dp(27)));
        LinearLayout labels = new LinearLayout(this);
        labels.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams infoLabels = new LinearLayout.LayoutParams(0, -2, 1);
        infoLabels.leftMargin = dp(14);
        info.addView(labels, infoLabels);
        TextView infoTitle = text("Info", 15, WHITE);
        infoTitle.setTypeface(Typeface.DEFAULT_BOLD);
        labels.addView(infoTitle);
        TextView infoSubtitle = text("Some additional information", 11, MUTED);
        infoSubtitle.setPadding(0, dp(5), 0, 0);
        labels.addView(infoSubtitle);
        info.addView(statusIcon(R.drawable.ic_files_chevron_right, MUTED),
                new LinearLayout.LayoutParams(dp(20), dp(20)));
        info.setOnClickListener(v -> showSettingsInfo());
        settings.addView(info);

        LinearLayout updatesHeader = new LinearLayout(this);
        updatesHeader.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout.LayoutParams updatesHeaderParams = new LinearLayout.LayoutParams(-1, dp(39));
        updatesHeaderParams.topMargin = dp(12);
        root.addView(updatesHeader, updatesHeaderParams);
        TextView updatesTitle = text("Updates", 15, WHITE);
        updatesTitle.setTypeface(Typeface.DEFAULT_BOLD);
        updatesHeader.addView(updatesTitle, new LinearLayout.LayoutParams(0, -2, 1));
        ImageView updatesRefresh = statusIcon(R.drawable.ic_apps_refresh, ORANGE);
        updatesRefresh.setContentDescription("Refresh app and firmware updates");
        updatesRefresh.setPadding(dp(8), dp(8), dp(8), dp(8));
        updatesRefresh.setOnClickListener(v -> beginGestureRefresh());
        updatesHeader.addView(updatesRefresh, new LinearLayout.LayoutParams(dp(44), dp(44)));

        Switch previewUpdates = new Switch(this);
        previewUpdates.setText("Include pre-releases");
        previewUpdates.setTextColor(WHITE);
        previewUpdates.setTextSize(12);
        previewUpdates.setChecked(prefs.getBoolean(WeaveService.KEY_PREVIEW_UPDATES, false));
        previewUpdates.setOnCheckedChangeListener((button, checked) -> {
            boolean previous = prefs.getBoolean(WeaveService.KEY_PREVIEW_UPDATES, false);
            if (checked == previous) return;
            if (FirmwareUpdatePolicy.activePhase(prefs.getString("firmware_update_phase", "")) ||
                    updateControls[UPDATE_APP].busy || updateControls[UPDATE_FIRMWARE].busy) {
                button.setChecked(previous);
                toast("Wait for the current update operation to finish");
                return;
            }
            prefs.edit().putBoolean(WeaveService.KEY_PREVIEW_UPDATES, checked).apply();
            for (int i = 0; i < 2; i++) {
                updateControls[i].release = null;
                updateControls[i].lastCheck = 0;
                checkUpdate(i);
            }
        });
        root.addView(previewUpdates, new LinearLayout.LayoutParams(-1, -2));
        restoreFirmwareUpdateUi();
        LinearLayout updates = new LinearLayout(this);
        updates.setOrientation(LinearLayout.HORIZONTAL);
        LinearLayout.LayoutParams updatesParams = new LinearLayout.LayoutParams(-1, -2);
        updatesParams.topMargin = dp(22);
        root.addView(updates, updatesParams);
        LinearLayout app = settingsUpdateTile(UPDATE_APP);
        updates.addView(app, new LinearLayout.LayoutParams(0, dp(201), 1));
        LinearLayout firmware = settingsUpdateTile(UPDATE_FIRMWARE);
        LinearLayout.LayoutParams fwParams = new LinearLayout.LayoutParams(0, dp(201), 1);
        fwParams.leftMargin = dp(12);
        updates.addView(firmware, fwParams);

        ScrollView scroller = scroll(root);
        settingsHost.addView(scroller, new FrameLayout.LayoutParams(-1, -1));
        syncing = true;
        serviceSwitch.setChecked(prefs.getBoolean(WeaveService.KEY_ENABLED, false));
        if (bootSwitch != null) bootSwitch.setChecked(prefs.getBoolean(WeaveService.KEY_BOOT, false));
        syncing = false;
        // Refresh GitHub status on entry. Failed checks are throttled until a manual tap.
        for (int kind = 0; kind < 2; kind++) {
            UpdateControl control = updateControls[kind];
            if (!control.busy && (control.lastCheck == 0 ||
                    ("unknown".equals(control.state) && System.currentTimeMillis() - control.lastCheck > 60000L) ||
                    System.currentTimeMillis() - control.lastCheck > 300000L)) {
                checkUpdate(kind);
            }
        }
    }

    private Switch settingsToggle(LinearLayout parent, int iconId, String title, String detail,
                                  CompoundButton.OnCheckedChangeListener listener) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(15), 0, dp(15));
        row.addView(statusIcon(iconId, WHITE), new LinearLayout.LayoutParams(dp(27), dp(27)));
        LinearLayout labels = new LinearLayout(this);
        labels.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams textParams = new LinearLayout.LayoutParams(0, -2, 1);
        textParams.leftMargin = dp(14);
        row.addView(labels, textParams);
        TextView name = text(title, 15, WHITE);
        name.setTypeface(Typeface.DEFAULT_BOLD);
        labels.addView(name);
        TextView subtitle = text(detail, 11, MUTED);
        subtitle.setPadding(0, dp(5), 0, 0);
        labels.addView(subtitle);
        Switch toggle = new Switch(this);
        toggle.setShowText(false);
        toggle.setSwitchMinWidth(dp(54));
        toggle.setMinimumWidth(dp(54));
        toggle.setTextColor(WHITE);
        toggle.setTextSize(11);
        toggle.setTypeface(Typeface.DEFAULT_BOLD);
        toggle.setThumbTintList(new ColorStateList(new int[][]{
                new int[]{android.R.attr.state_checked}, new int[]{}},
                new int[]{WHITE, 0xffbbbbbb}));
        android.graphics.drawable.StateListDrawable vividTrack = new android.graphics.drawable.StateListDrawable();
        vividTrack.addState(new int[]{android.R.attr.state_checked}, rounded(ORANGE, ORANGE, 24));
        vividTrack.addState(new int[]{}, rounded(0xff444444, 0xff555555, 24));
        toggle.setTrackDrawable(vividTrack);
        toggle.setOnCheckedChangeListener(listener);
        row.addView(toggle);
        row.setOnClickListener(v -> toggle.setChecked(!toggle.isChecked()));
        parent.addView(row);
        return toggle;
    }

    private static String installedFirmwareVersion(String raw) {
        if (raw == null || raw.trim().isEmpty()) return null;
        String value = raw.trim();
        // Never substitute the Android app version for a missing Flipper
        // version: both components can be updated independently.
        java.util.regex.Matcher version = java.util.regex.Pattern.compile(
                "(?i)(?:^|\\b)v?(\\d+\\.\\d+\\.\\d+)(?:$|\\b)").matcher(value);
        return version.find() ? version.group(1) : null;
    }

    private String updateCurrentVersion(int kind) {
        if (kind == UPDATE_APP) return BuildConfig.VERSION_NAME;
        return flipperIsConnected() ? installedFirmwareVersion(
                prefs.getString(WeaveService.KEY_FIRMWARE, "")) : null;
    }

    private boolean hasInternetAccess() {
        try {
            android.net.ConnectivityManager manager =
                    (android.net.ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
            if (manager == null) return false;
            android.net.Network network = manager.getActiveNetwork();
            if (network == null) return false;
            android.net.NetworkCapabilities caps = manager.getNetworkCapabilities(network);
            return caps != null && caps.hasCapability(android.net.NetworkCapabilities.NET_CAPABILITY_VALIDATED);
        } catch (SecurityException ignored) { return false; }
    }

    private void restoreFirmwareUpdateUi() {
        String phase = prefs.getString("firmware_update_phase", "");
        UpdateControl control = updateControls[UPDATE_FIRMWARE];
        if (FirmwareUpdatePolicy.activePhase(phase)) {
            if (control.checkingRelease) {
                ++control.releaseCheckEpoch;
                control.checkingRelease = false;
                List<Runnable> callbacks = new ArrayList<>(control.pendingCheckListeners);
                control.pendingCheckListeners.clear();
                for (Runnable callback : callbacks) handler.post(callback);
            }
            control.firmwarePhase = phase;
            control.state = "installing";
            control.busy = true;
            control.percent = prefs.getInt("firmware_update_percent", 0);
        } else if (control.busy && "installing".equals(control.state) &&
                ("error".equals(phase) || "ready".equals(phase))) {
            control.busy = false;
            control.state = "ready".equals(phase) ? "current" : "unknown";
            control.error = prefs.getString("firmware_update_message", "");
        }
    }
    private void drawUpdateControl(int kind) {
        UpdateControl control = updateControls[kind];
        String current = updateCurrentVersion(kind);
        boolean connected = flipperIsConnected();
        boolean firmware = kind == UPDATE_FIRMWARE;
        boolean canCompare = current != null && control.release != null &&
                (!firmware || connected);
        String status = control.checkingRelease ? "checking" : control.state;
        // The firmware version can arrive *after* GitHub responds. Reconcile
        // a successful release check with the latest verified device version;
        // never retain the initial unknown/waiting status indefinitely.
        if (canCompare && !control.busy && control.lastCheck > 0 &&
                (control.error == null || control.error.isEmpty())) {
            status = WeaveUpdates.compare(current, control.release.version) < 0
                    ? "available" : "current";
            // Make the click handler agree with the icon without another
            // network request when the device version arrived after GitHub.
            control.state = status;
            control.error = "";
        }
        else if (firmware && !connected && !control.busy) status = "disconnected";
        else if (!canCompare && !control.busy && "current".equals(status)) status = "unknown";
        String label = firmware && !connected ? "Connect Flipper" :
                current == null && firmware && "waiting_version".equals(control.state)
                        ? "Reading Flipper version…" :
                current == null ? "Version unavailable" : "v" + current;
        if (canCompare && WeaveUpdates.compare(current, control.release.version) < 0 &&
                ("available".equals(status) || control.busy))
            label += " → v" + control.release.version;
        if (control.busy && firmware && "installing".equals(control.state)) {
            switch (control.firmwarePhase) {
                case "waiting": label = "Waiting for RPC…"; break;
                case "downloading": label = "Downloading firmware…"; break;
                case "staging": label = "Checking firmware bundle…"; break;
                case "uploading": label = "Transferring via " + currentTransportLabel(); break;
                case "preparing": label = "Preparing update…"; break;
                case "rebooting": label = "Waiting for reboot…"; break;
                default: label = "Installing on Flipper"; break;
            }
        }
        else if (control.busy && !firmware && "installing".equals(control.state))
            label = "Confirm Android installation…";
        else if (!control.busy && "pending".equals(status))
            label = "v" + control.pendingVersion + " build pending";
        else if (!control.busy && control.error != null && !control.error.isEmpty())
            label = firmware && !connected ? "Connect Flipper" :
                    (control.error.contains("not published yet") ? "Release build pending" :
                     control.error.contains("rate limit") ? "GitHub limit · retry later" :
                     !hasInternetAccess() ? "Offline · check connection" :
                            "Check failed · tap to retry");
        if (control.versionView != null) control.versionView.setText(label);
        // The busy state shows progress, not an outdated check/update icon.
        boolean verifiedCurrent = !control.busy && "current".equals(status);
        boolean available = !control.busy && "available".equals(status);
        boolean unknownOffline = !verifiedCurrent && !available && !hasInternetAccess() && !connected;
        String symbol = control.busy ? (control.checkingRelease ? "Checking…" :
                "installing".equals(control.state) ? "" + control.percent + "%" :
                "↓ " + control.percent + "%") :
                verifiedCurrent || available ? "" : unknownOffline ? "?" :
                "pending".equals(status) ? "Pending" :
                firmware && connected && "waiting_version".equals(control.state)
                        ? "Reading…" : "Retry";
        if (control.statusIcon != null) {
            if (verifiedCurrent || available) {
                control.statusIcon.setImageResource(verifiedCurrent ?
                        R.drawable.ic_update_current : R.drawable.ic_update_available);
                control.statusIcon.setVisibility(View.VISIBLE);
            } else control.statusIcon.setVisibility(View.GONE);
        }
        if (control.buttonText != null) {
            control.buttonText.setText(symbol);
            control.buttonText.setContentDescription(control.busy ? "Checking or installing update" :
                    available ? "Install newer version" : verifiedCurrent ? "Up to date" :
                    "pending".equals(status) ? "Release assets are still being built; tap to recheck" :
                    firmware && !connected ? "Connect a Flipper to check firmware" : "Retry update check");
        }
        if (control.progressTrack != null) {
            control.progressTrack.setBackground(rounded(control.busy ? 0xff733500 : ORANGE, ORANGE, 11));
            control.progressTrack.setClipToOutline(true);
            if (control.progressFill != null) {
                FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams)control.progressFill.getLayoutParams();
                int width = control.progressTrack.getWidth();
                if (width > 0) { lp.width = Math.round(width * control.percent / 100f);
                    control.progressFill.setLayoutParams(lp); }
                control.progressFill.setVisibility(control.busy ? View.VISIBLE : View.GONE);
            }
            control.progressTrack.setEnabled(!control.busy && !verifiedCurrent && (!firmware || connected));
            control.progressTrack.setAlpha(verifiedCurrent ? 0.80f : 1f);
        }
    }

    private void checkUpdate(int kind) { checkUpdate(kind, null, false); }

    private void finishUpdateCheck(int kind) {
        UpdateControl control = updateControls[kind];
        control.busy = false;
        control.checkingRelease = false;
        drawUpdateControl(kind);
        // Settings swipe can attach to a check already started on page entry.
        List<Runnable> listeners = new ArrayList<>(control.pendingCheckListeners);
        control.pendingCheckListeners.clear();
        for (Runnable listener : listeners) listener.run();
    }

    private void checkUpdate(int kind, Runnable afterCheck, boolean force) {
        if (kind == UPDATE_FIRMWARE) restoreFirmwareUpdateUi();
        UpdateControl control = updateControls[kind];
        if (control.busy && (!force || !control.checkingRelease)) {
            if (afterCheck != null) {
                if (control.checkingRelease) control.pendingCheckListeners.add(afterCheck);
                else afterCheck.run(); // A download is not a release check.
            }
            return;
        }
        // A Settings refresh always checks BOTH GitHub assets. Reading the
        // installed firmware version over RPC is independent and may finish
        // after this HTTPS request (or after a Flipper reconnect).
        control.busy = true;
        control.checkingRelease = true;
        final int epoch = ++control.releaseCheckEpoch;
        if (afterCheck != null) control.pendingCheckListeners.add(afterCheck);
        control.percent = 0;
        drawUpdateControl(kind);
        final boolean preview = prefs.getBoolean(WeaveService.KEY_PREVIEW_UPDATES, false);
        new Thread(() -> {
            try {
                WeaveUpdates.Release release = WeaveUpdates.latest(kind == UPDATE_FIRMWARE, preview);
                handler.post(() -> {
                    if (epoch != control.releaseCheckEpoch) return; // Superseded by a manual refresh.
                    control.release = release;
                    control.pendingVersion = "";
                    // Never compare against a stale device version captured
                    // before the network request, nor claim current on null.
                    String installed = updateCurrentVersion(kind);
                    control.state = installed == null ?
                            kind == UPDATE_FIRMWARE && firmwareVersionReadFailed ?
                                    "unknown" : "waiting_version" :
                            WeaveUpdates.compare(installed, release.version) < 0
                                    ? "available" : "current";
                    control.error = kind == UPDATE_FIRMWARE && firmwareVersionReadFailed ?
                            "Flipper firmware version unavailable over RPC" : "";
                    android.util.Log.d("WeaveUpdates", (kind == UPDATE_APP ? "App" : "Firmware") +
                            " release v" + release.version + ", installed " + installed);
                    control.lastCheck = System.currentTimeMillis();
                    finishUpdateCheck(kind);
                });
            } catch (WeaveUpdates.AssetPendingException pending) {
                handler.post(() -> {
                    if (epoch != control.releaseCheckEpoch) return;
                    control.state = "pending";
                    control.pendingVersion = pending.version;
                    control.error = pending.getMessage();
                    control.release = null;
                    control.lastCheck = System.currentTimeMillis();
                    finishUpdateCheck(kind);
                });
            } catch (Exception e) {
                handler.post(() -> {
                    if (epoch != control.releaseCheckEpoch) return;
                    control.state = "unknown"; control.error = e.getMessage();
                    control.pendingVersion = "";
                    android.util.Log.w("WeaveUpdates", "GitHub update check failed", e);
                    control.release = null; // Do not reuse stale release metadata after an error.
                    control.lastCheck = System.currentTimeMillis();
                    finishUpdateCheck(kind);
                });
            }
        }, "Weave-release-check").start();
    }

    private void tapUpdate(int kind) {
        UpdateControl control = updateControls[kind];
        if (control.busy) return;
        if (kind == UPDATE_FIRMWARE && updateControls[UPDATE_APP].busy && !updateControls[UPDATE_APP].checkingRelease ||
                kind == UPDATE_APP && (updateControls[UPDATE_FIRMWARE].busy && !updateControls[UPDATE_FIRMWARE].checkingRelease ||
                        FirmwareUpdatePolicy.activePhase(prefs.getString("firmware_update_phase", "")))) {
            toast("Finish the active update first"); return;
        }
        if (kind == UPDATE_FIRMWARE && !flipperIsConnected()) {
            control.state = "unknown"; toast("Flipper disconnected"); drawUpdateControl(kind); return;
        }
        if ("current".equals(control.state) && control.release != null &&
                WeaveUpdates.compare(updateCurrentVersion(kind), control.release.version) >= 0) return;
        if (kind == UPDATE_FIRMWARE && updateCurrentVersion(kind) == null &&
                control.release != null) {
            requestInstalledFirmwareVersion(); return;
        }
        if (!"available".equals(control.state) || control.release == null) {
            if (kind == UPDATE_FIRMWARE) requestInstalledFirmwareVersion();
            checkUpdate(kind); return;
        }
        if (WeaveUpdates.compare(updateCurrentVersion(kind), control.release.version) >= 0) {
            checkUpdate(kind); return;
        }
        control.busy = true; control.percent = 0;
        if (kind == UPDATE_FIRMWARE) {
            control.state = "installing";
            control.firmwarePhase = "waiting";
        }
        drawUpdateControl(kind);
        if (kind == UPDATE_FIRMWARE) {
            // Keep downloading and flashing alive while Settings is backgrounded.
            startService(new Intent(this, WeaveService.class)
                    .setAction(WeaveService.ACTION_DOWNLOAD_FIRMWARE)
                    .putExtra("version", control.release.version));
            return;
        }
        final WeaveUpdates.Release release = control.release;
        new Thread(() -> {
            try {
                File file = WeaveUpdates.download(this, release, false,
                        percent -> handler.post(() -> { control.percent = percent; drawUpdateControl(kind); }));
                validateDownloadedApk(file, release.version);
                handler.post(() -> {
                    if (isFinishing() || isDestroyed()) return;
                    pendingUpdateApk = file;
                    pendingApkVersion = release.version;
                    apkReadyToInstall = true;
                    control.state = "installing";
                    control.percent = 100;
                    drawUpdateControl(kind);
                    if (appForeground) openApkInstaller();
                });
            } catch (Exception e) {
                handler.post(() -> {
                    control.busy = false;
                    control.state = "unknown";
                    control.error = e.getMessage();
                    drawUpdateControl(kind);
                    toast("Update download failed: " + e.getMessage());
                });
            }
        }, "Weave-release-download").start();
    }

    private void validateDownloadedApk(File apk, String version) throws IOException {
        android.content.pm.PackageInfo info = getPackageManager().getPackageArchiveInfo(apk.getAbsolutePath(), 0);
        if (info == null || !getPackageName().equals(info.packageName) || !version.equals(info.versionName))
            throw new IOException("APK package or version mismatch");
        long code = Build.VERSION.SDK_INT >= 28 ? info.getLongVersionCode() : info.versionCode;
        if (code <= BuildConfig.VERSION_CODE) throw new IOException("APK is not a newer build");
    }

    private void clearApkUpdate() {
        pendingUpdateApk = null;
        pendingApkVersion = null;
        apkReadyToInstall = false;
        apkUpdateRequest = 0;
        UpdateControl control = updateControls[UPDATE_APP];
        control.busy = false;
        control.state = "unknown";
        control.error = "";
        drawUpdateControl(UPDATE_APP);
    }

    private void openApkInstaller() {
        if (!apkReadyToInstall || apkUpdateRequest != 0 || !appForeground || isFinishing() || isDestroyed()) return;
        apkReadyToInstall = false;
        if (pendingUpdateApk == null || !pendingUpdateApk.isFile()) {
            clearApkUpdate(); toast("Downloaded APK unavailable"); return;
        }
        if (FirmwareUpdatePolicy.activePhase(prefs.getString("firmware_update_phase", ""))) {
            offerApkRecovery("Finish the firmware update before updating Android."); return;
        }
        try {
            if (!getPackageManager().canRequestPackageInstalls()) {
                apkUpdateRequest = REQUEST_UPDATE_APK_PERMISSION;
                startActivityForResult(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                        Uri.parse("package:" + getPackageName())), apkUpdateRequest);
                return;
            }
            // Installateur système du profil courant, aucune opération cross-user.
            // Android contrôle la signature et exige le consentement de l'utilisateur.
            Uri uri = Uri.parse("content://" + getPackageName() + ".updates/weave-app.apk");
            Intent install = new Intent(Intent.ACTION_INSTALL_PACKAGE);
            install.setDataAndType(uri, "application/vnd.android.package-archive");
            install.setClipData(ClipData.newRawUri("Weave APK", uri));
            install.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            install.putExtra(Intent.EXTRA_RETURN_RESULT, true);
            apkUpdateRequest = REQUEST_UPDATE_APK_INSTALL;
            startActivityForResult(install, apkUpdateRequest);
        } catch (RuntimeException e) {
            apkUpdateRequest = 0;
            offerApkRecovery("Android installer unavailable: " + e.getMessage());
        }
    }

    private void offerApkRecovery(String message) {
        if (pendingUpdateApk == null || !pendingUpdateApk.isFile() || isFinishing() || isDestroyed()) {
            clearApkUpdate(); return;
        }
        new AlertDialog.Builder(this)
                .setTitle("App update")
                .setMessage(message + "\n\nYou can save the APK before a manual reinstall. A different signing key " +
                        "may require removing the old app from this profile. App data would be lost.")
                .setPositiveButton("Retry", (dialog, which) -> {
                    apkReadyToInstall = true;
                    openApkInstaller();
                })
                .setNeutralButton("Save APK", (dialog, which) -> saveUpdateApk())
                .setNegativeButton("Close", (dialog, which) -> clearApkUpdate())
                .setOnCancelListener(dialog -> clearApkUpdate()).show();
    }

    private void saveUpdateApk() {
        Intent save = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        save.setType("application/vnd.android.package-archive");
        save.addCategory(Intent.CATEGORY_OPENABLE);
        save.putExtra(Intent.EXTRA_TITLE, "Weave-" + pendingApkVersion + "-android.apk");
        try {
            apkUpdateRequest = REQUEST_UPDATE_APK_SAVE;
            startActivityForResult(save, apkUpdateRequest);
        } catch (RuntimeException e) {
            clearApkUpdate(); toast("No document picker available to save the APK");
        }
    }

    private LinearLayout settingsUpdateTile(int kind) {
        boolean isApp = kind == UPDATE_APP;
        String name = isApp ? "Update App" : "Update Firmware";
        UpdateControl control = updateControls[kind];
        LinearLayout tile = card(false);
        tile.setGravity(Gravity.CENTER_HORIZONTAL | Gravity.CENTER_VERTICAL);
        tile.setPadding(dp(10), dp(15), dp(10), dp(17));
        ImageView image = new ImageView(this);
        image.setImageResource(isApp ? R.mipmap.ic_launcher : R.drawable.ic_settings_flipper);
        image.setImageTintList(null);
        image.setScaleType(ImageView.ScaleType.FIT_CENTER);
        tile.addView(image, new LinearLayout.LayoutParams(dp(76), dp(66)));
        if (!isApp) WeaveUpdates.loadCachedImage(this, handler, "flipper-qflipper",
                "https://cdn.flipper.net/qFlipper_macOS_256px_ugly.png",
                bitmap -> { image.setImageBitmap(bitmap); image.setImageTintList(null); });
        TextView title = text(name, 14, WHITE);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setGravity(Gravity.CENTER);
        LinearLayout.LayoutParams titleParams = new LinearLayout.LayoutParams(-1, -2);
        titleParams.topMargin = dp(10);
        tile.addView(title, titleParams);
        TextView version = text("", 11, MUTED);
        version.setGravity(Gravity.CENTER);
        version.setMaxLines(2);
        LinearLayout.LayoutParams versionParams = new LinearLayout.LayoutParams(-1, -2);
        versionParams.topMargin = dp(6);
        tile.addView(version, versionParams);
        control.versionView = version;

        FrameLayout progress = new FrameLayout(this);
        progress.setClipToOutline(true);
        progress.setBackground(rounded(ORANGE, ORANGE, 11));
        progress.setOutlineProvider(new ViewOutlineProvider() {
            @Override public void getOutline(View v, Outline outline) {
                outline.setRoundRect(0, 0, v.getWidth(), v.getHeight(), dp(11));
            }
        });
        View fill = new View(this);
        fill.setBackground(rounded(ORANGE, ORANGE, 11));
        progress.addView(fill, new FrameLayout.LayoutParams(0, -1, Gravity.START));
        TextView button = text("?", 19, WHITE);
        button.setGravity(Gravity.CENTER);
        button.setTypeface(Typeface.DEFAULT_BOLD);
        progress.addView(button, new FrameLayout.LayoutParams(-1, -1));
        ImageView statusIcon = new ImageView(this);
        statusIcon.setScaleType(ImageView.ScaleType.FIT_CENTER);
        FrameLayout.LayoutParams iconPosition = new FrameLayout.LayoutParams(dp(24), dp(24), Gravity.CENTER);
        progress.addView(statusIcon, iconPosition);
        control.statusIcon = statusIcon;
        LinearLayout.LayoutParams buttonParams = new LinearLayout.LayoutParams(-1, dp(39));
        buttonParams.topMargin = dp(13);
        tile.addView(progress, buttonParams);
        control.progressTrack = progress;
        control.progressFill = fill;
        control.buttonText = button;
        progress.post(() -> drawUpdateControl(kind));
        progress.setOnClickListener(v -> tapUpdate(kind));
        drawUpdateControl(kind);
        return tile;
    }

    private void showSettingsInfo() {
        settingsInfoOpen = true;
        if (settingsHost == null) return;
        settingsHost.removeAllViews();
        LinearLayout root = pageRoot();
        root.setPadding(dp(19), dp(5), dp(19), dp(20));

        LinearLayout back = new LinearLayout(this);
        back.setGravity(Gravity.END);
        back.addView(toolAction(R.drawable.ic_apps_back, "Back", WHITE, v -> showSettingsMain()),
                new LinearLayout.LayoutParams(dp(60), dp(54)));
        root.addView(back);

        LinearLayout identity = new LinearLayout(this);
        identity.setGravity(Gravity.CENTER_HORIZONTAL);
        identity.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams idParams = new LinearLayout.LayoutParams(-1, -2);
        idParams.topMargin = dp(4);
        root.addView(identity, idParams);
        TextView appTitle = text("Weave", 22, WHITE);
        appTitle.setGravity(Gravity.CENTER);
        appTitle.setTypeface(Typeface.DEFAULT_BOLD);
        identity.addView(appTitle);
        ImageView icon = new ImageView(this);
        icon.setImageResource(R.mipmap.ic_launcher);
        icon.setScaleType(ImageView.ScaleType.FIT_CENTER);
        LinearLayout.LayoutParams iconParams = new LinearLayout.LayoutParams(dp(86), dp(86));
        iconParams.topMargin = dp(12);
        identity.addView(icon, iconParams);
        TextView description = text("Firmware for Flipper Zero with a shared network API.", 13, WHITE);
        description.setGravity(Gravity.CENTER);
        description.setPadding(dp(16), dp(14), dp(16), dp(12));
        identity.addView(description);

        View divider = new View(this);
        divider.setBackgroundColor(BORDER);
        LinearLayout.LayoutParams dividerParams = new LinearLayout.LayoutParams(-1, dp(1));
        dividerParams.topMargin = dp(12);
        root.addView(divider, dividerParams);

        TextView flipNetTitle = text("From FlipNet", 17, WHITE);
        flipNetTitle.setTypeface(Typeface.DEFAULT_BOLD);
        flipNetTitle.setPadding(0, dp(20), 0, dp(10));
        root.addView(flipNetTitle);
        LinearLayout story = new LinearLayout(this);
        story.setGravity(Gravity.CENTER_VERTICAL);
        root.addView(story);
        TextView storyText = text("First prototype. It gave Flipper Zero Internet access through a PC or Android gateway over USB or Bluetooth.", 13, 0xffcccccc);
        storyText.setLineSpacing(dp(4), 1.05f);
        story.addView(storyText, new LinearLayout.LayoutParams(-1, -2));
        View divider2 = new View(this);
        divider2.setBackgroundColor(BORDER);
        LinearLayout.LayoutParams divider2Params = new LinearLayout.LayoutParams(-1, dp(1));
        divider2Params.topMargin = dp(24);
        divider2Params.bottomMargin = dp(17);
        root.addView(divider2, divider2Params);

        LinearLayout creator = new LinearLayout(this);
        creator.setGravity(Gravity.CENTER_VERTICAL);
        root.addView(creator);
        ImageView mark = new ImageView(this);
        mark.setScaleType(ImageView.ScaleType.CENTER_CROP);
        mark.setBackground(rounded(CARD, BORDER, 18));
        roundImage(mark);
        creator.addView(mark, new LinearLayout.LayoutParams(dp(40), dp(40)));
        mark.setImageResource(R.drawable.ic_link_github);
        WeaveUpdates.loadCachedImage(this, handler, "github-avatar",
                "https://avatars.githubusercontent.com/u/326523721", avatar -> {
            mark.setImageTintList(null);
            mark.setImageBitmap(avatar);
            mark.setScaleType(ImageView.ScaleType.CENTER_CROP);
        });
        LinearLayout person = new LinearLayout(this);
        person.setOrientation(LinearLayout.VERTICAL);
        LinearLayout.LayoutParams personParams = new LinearLayout.LayoutParams(0, -2, 1);
        personParams.leftMargin = dp(11);
        creator.addView(person, personParams);
        TextView name = text("Nullmess", 15, WHITE);
        name.setTypeface(Typeface.DEFAULT_BOLD);
        person.addView(name);
        person.addView(text("Project Manager", 11, MUTED));

        LinearLayout links = new LinearLayout(this);
        links.setGravity(Gravity.CENTER);
        creator.addView(links);
        links.addView(settingsInfoLink(R.drawable.ic_link_github, "GitHub",
                "https://github.com/Nullmess"));
        View separate = new View(this);
        separate.setBackgroundColor(BORDER);
        LinearLayout.LayoutParams separateParams = new LinearLayout.LayoutParams(dp(1), dp(44));
        separateParams.leftMargin = dp(10);
        separateParams.rightMargin = dp(10);
        links.addView(separate, separateParams);
        links.addView(settingsInfoLink(R.drawable.ic_settings_kofi, "Ko-fi", "https://ko-fi.com/macxzew"));
        settingsHost.addView(scroll(root), new FrameLayout.LayoutParams(-1, -1));
    }

    private LinearLayout settingsInfoLink(int iconId, String label, String target) {
        LinearLayout button = new LinearLayout(this);
        button.setOrientation(LinearLayout.VERTICAL);
        button.setGravity(Gravity.CENTER);
        ImageView icon = statusIcon(iconId, WHITE);
        button.addView(icon, new LinearLayout.LayoutParams(dp(23), dp(23)));
        TextView caption = text(label, 10, WHITE);
        caption.setPadding(0, dp(3), 0, 0);
        button.addView(caption);
        button.setContentDescription(label);
        button.setOnClickListener(v -> openExternal(target));
        return button;
    }

    /** File UI is only a browser of verified device entries; it never invents file contents. */
    private View filesPage() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(14), dp(6), dp(14), dp(12));

        // Two aligned rows: unboxed Flipper Zero storage and SD search on the
        // LEFT, four aligned file actions on the RIGHT of each row.
        filesToolbar = new LinearLayout(this);
        filesToolbar.setOrientation(LinearLayout.VERTICAL);
        root.addView(filesToolbar, new LinearLayout.LayoutParams(-1, dp(120)));

        final String[] names = {"New", "Import", "Export", "Delete", "Rename", "Move", "Copy", "Paste"};
        final int[] icons = {R.drawable.ic_files_new, R.drawable.ic_files_upload,
                R.drawable.ic_terminal_export, R.drawable.ic_terminal_clear,
                R.drawable.ic_files_rename, R.drawable.ic_files_move,
                R.drawable.ic_terminal_copy, R.drawable.ic_files_paste};
        for (int rowIndex = 0; rowIndex < 2; rowIndex++) {
            LinearLayout bar = new LinearLayout(this);
            bar.setOrientation(LinearLayout.HORIZONTAL);
            bar.setGravity(Gravity.CENTER_VERTICAL);
            filesToolbar.addView(bar, new LinearLayout.LayoutParams(-1, 0, 1));

            LinearLayout actions = new LinearLayout(this);
            actions.setGravity(Gravity.CENTER_VERTICAL);
            // Attach actions after the leading storage/search component, so
            // both rows have the same left-first order and aligned buttons.
            for (int colIndex = 0; colIndex < 4; colIndex++) {
                final int action = rowIndex * 4 + colIndex;
                LinearLayout button = new LinearLayout(this);
                button.setOrientation(LinearLayout.VERTICAL);
                button.setGravity(Gravity.CENTER);
                button.setPadding(0, dp(2), 0, dp(1));
                actions.addView(button, new LinearLayout.LayoutParams(0, -1, 1));
                ImageView glyph = statusIcon(icons[action], WHITE);
                button.addView(glyph, new LinearLayout.LayoutParams(dp(21), dp(21)));
                TextView label = text(names[action], 9, MUTED);
                label.setGravity(Gravity.CENTER);
                label.setSingleLine(true);
                label.setPadding(0, dp(3), 0, 0);
                button.addView(label, new LinearLayout.LayoutParams(-1, -2));
                filesTools[action] = label;
                button.setTag(glyph);
                button.setOnClickListener(v -> filesAction(action));
                button.setContentDescription(names[action]);
            }

            if (rowIndex == 0) {
                LinearLayout storage = new LinearLayout(this);
                storage.setOrientation(LinearLayout.VERTICAL);
                storage.setGravity(Gravity.CENTER_VERTICAL);
                storage.setPadding(dp(2), dp(5), dp(8), dp(5));
                // Not a selectable drive and not a card: simple inline label.
                LinearLayout.LayoutParams storageParams = new LinearLayout.LayoutParams(0, dp(54), 1f);
                storageParams.rightMargin = dp(6);
                bar.addView(storage, storageParams);
                LinearLayout storageTop = new LinearLayout(this);
                storageTop.setGravity(Gravity.CENTER_VERTICAL);
                storage.addView(storageTop, new LinearLayout.LayoutParams(-1, 0, 1));
                ImageView drive = statusIcon(R.drawable.ic_files_drive, WHITE);
                storageTop.addView(drive, new LinearLayout.LayoutParams(dp(18), dp(18)));
                LinearLayout storageNames = new LinearLayout(this);
                storageNames.setOrientation(LinearLayout.VERTICAL);
                LinearLayout.LayoutParams namesParams = new LinearLayout.LayoutParams(0, -2, 1);
                namesParams.leftMargin = dp(5);
                storageTop.addView(storageNames, namesParams);
                filesStorageName = text("Flipper Zero", 11, WHITE);
                filesStorageName.setTypeface(Typeface.DEFAULT_BOLD);
                filesStorageName.setSingleLine(true);
                filesStorageName.setEllipsize(android.text.TextUtils.TruncateAt.END);
                storageNames.addView(filesStorageName);
                filesStorageUsage = text("—", 8, MUTED);
                filesStorageUsage.setSingleLine(true);
                filesStorageUsage.setEllipsize(android.text.TextUtils.TruncateAt.END);
                storageNames.addView(filesStorageUsage);
                filesStorageBar = new ProgressBar(this, null, android.R.attr.progressBarStyleHorizontal);
                filesStorageBar.setMax(1000);
                filesStorageBar.setProgress(0);
                filesStorageBar.setProgressTintList(ColorStateList.valueOf(ORANGE));
                filesStorageBar.setProgressBackgroundTintList(ColorStateList.valueOf(0xff343434));
                storage.addView(filesStorageBar, new LinearLayout.LayoutParams(-1, dp(3)));
                // This is an indicator for /ext, not a drive selector.
            } else {
                LinearLayout searchBox = new LinearLayout(this);
                searchBox.setGravity(Gravity.CENTER_VERTICAL);
                searchBox.setPadding(dp(7), 0, dp(3), 0);
                searchBox.setBackground(rounded(CARD, BORDER, 12));
                LinearLayout.LayoutParams searchParams = new LinearLayout.LayoutParams(0, dp(46), 1f);
                searchParams.rightMargin = dp(6);
                bar.addView(searchBox, searchParams);
                ImageView magnifier = statusIcon(android.R.drawable.ic_menu_search, MUTED);
                searchBox.addView(magnifier, new LinearLayout.LayoutParams(dp(17), dp(17)));
                filesSearchInput = new EditText(this);
                filesSearchInput.setSingleLine(true);
                filesSearchInput.setTextSize(TypedValue.COMPLEX_UNIT_SP, 12);
                filesSearchInput.setTextColor(WHITE);
                filesSearchInput.setHintTextColor(MUTED);
                filesSearchInput.setHint("Search...");
                filesSearchInput.setPadding(dp(5), 0, 0, 0);
                filesSearchInput.setBackgroundColor(Color.TRANSPARENT);
                filesSearchInput.setInputType(InputType.TYPE_CLASS_TEXT);
                filesSearchInput.setImeOptions(android.view.inputmethod.EditorInfo.IME_ACTION_SEARCH);
                searchBox.addView(filesSearchInput, new LinearLayout.LayoutParams(0, -1, 1));
                filesSearchClear = text("×", 20, MUTED);
                filesSearchClear.setGravity(Gravity.CENTER);
                filesSearchClear.setVisibility(View.GONE);
                filesSearchClear.setContentDescription("Clear file search");
                filesSearchClear.setOnClickListener(v -> filesSearchInput.setText(""));
                searchBox.addView(filesSearchClear, new LinearLayout.LayoutParams(dp(25), -1));
                filesSearchInput.addTextChangedListener(new TextWatcher() {
                    @Override public void beforeTextChanged(CharSequence text, int start, int count, int after) { }
                    @Override public void onTextChanged(CharSequence text, int start, int before, int count) { }
                    @Override public void afterTextChanged(Editable text) {
                        filesSearchTerm = text.toString().trim();
                        // An older browse request may have been superseded by the search.
                        // Permit a fresh listing when the query is cleared.
                        requestedFilesPath = null;
                        filesSearchClear.setVisibility(filesSearchTerm.isEmpty() ? View.GONE : View.VISIBLE);
                        handler.removeCallbacks(delayedFilesSearch);
                        handler.removeCallbacks(nextFilesSearchStep);
                        handler.removeCallbacks(filesSearchFolderTimeout);
                        filesGlobalSearch = null;
                        if (!filesSearchTerm.isEmpty()) handler.postDelayed(delayedFilesSearch, 420);
                        updateFilesPanel();
                    }
                });
                filesSearchInput.setOnEditorActionListener((view, actionId, event) -> {
                    if (actionId != android.view.inputmethod.EditorInfo.IME_ACTION_SEARCH) return false;
                    handler.removeCallbacks(delayedFilesSearch);
                    startFilesGlobalSearch();
                    return true;
                });
            }
            bar.addView(actions, new LinearLayout.LayoutParams(0, -1, 1.32f));
        }

        LinearLayout listPanel = new LinearLayout(this);
        listPanel.setOrientation(LinearLayout.VERTICAL);
        listPanel.setBackground(rounded(0xff111111, BORDER, 26));
        listPanel.setPadding(dp(12), dp(10), dp(12), dp(10));
        LinearLayout.LayoutParams panelParams = new LinearLayout.LayoutParams(-1, 0, 1);
        panelParams.topMargin = dp(7);
        root.addView(listPanel, panelParams);
        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        scroll.setVerticalScrollBarEnabled(true);
        scroll.setScrollbarFadingEnabled(false);
        listPanel.addView(scroll, new LinearLayout.LayoutParams(-1, -1));
        filesList = new LinearLayout(this);
        filesList.setOrientation(LinearLayout.VERTICAL);
        scroll.addView(filesList, new ScrollView.LayoutParams(-1, -1));
        filesPanelReady = true;
        filesLastConnected = flipperIsConnected();
        updateFilesPanel();
        return root;
    }

    private void startFilesGlobalSearch() {
        handler.removeCallbacks(delayedFilesSearch);
        handler.removeCallbacks(nextFilesSearchStep);
        handler.removeCallbacks(filesSearchFolderTimeout);
        filesGlobalSearch = null;
        if (filesSearchTerm.isEmpty() || selectedPage != FILES || !flipperIsConnected()) {
            if (selectedPage == FILES) updateFilesPanel();
            return;
        }
        if (archiveExportSession != null || activeBadgeJob() != null) {
            toast("File search unavailable during a transfer");
            return;
        }
        filesGlobalSearch = new FilesGlobalSearch(filesSearchTerm);
        filesSearchStartedAt = android.os.SystemClock.uptimeMillis();
        updateFilesPanel();
        requestNextFilesSearchFolder();
    }

    private void requestNextFilesSearchFolder() {
        FilesGlobalSearch search = filesGlobalSearch;
        if (search == null || selectedPage != FILES || !flipperIsConnected()) return;
        if (android.os.SystemClock.uptimeMillis() - filesSearchStartedAt > 60000) {
            search.stopEarly();
            updateFilesPanel();
            return;
        }
        String next = search.nextDirectory();
        if (next == null) {
            updateFilesPanel();
            return;
        }
        sendDeviceCommand(WeaveService.ACTION_LIST_FILES, "path", next);
        handler.removeCallbacks(filesSearchFolderTimeout);
        handler.postDelayed(filesSearchFolderTimeout, 14000);
    }

    private long filesSearchStartedAt;

    private boolean receiveFilesSearchListing(Intent intent, String path, boolean ok) {
        FilesGlobalSearch search = filesGlobalSearch;
        if (search == null || !search.isWaitingFor(path)) return false;
        // BLE may abort the previous request when a new search replaces it.
        if (!ok && "New directory selected".equals(intent.getStringExtra("message"))) return true;
        handler.removeCallbacks(filesSearchFolderTimeout);
        search.accept(path, ok, intent.getStringArrayExtra("names"),
                intent.getBooleanArrayExtra("directories"), intent.getLongArrayExtra("sizes"));
        if (selectedPage == FILES) updateFilesPanel();
        handler.removeCallbacks(nextFilesSearchStep);
        if (!search.isFinished()) handler.postDelayed(nextFilesSearchStep, 70);
        return true;
    }

    private void renderFilesSearchResults() {
        FilesGlobalSearch search = filesGlobalSearch;
        if (search == null || !filesSearchTerm.equals(search.query())) {
            TextView searching = text("Searching the SD card…", 13, MUTED);
            searching.setPadding(dp(7), dp(15), dp(7), dp(15));
            filesList.addView(searching);
            return;
        }
        TextView status = text(search.status(), 11, MUTED);
        status.setPadding(dp(7), dp(4), dp(7), dp(9));
        filesList.addView(status, new LinearLayout.LayoutParams(-1, -2));
        fileRule(filesList);
        for (FilesGlobalSearch.Result result : search.matches()) {
            LinearLayout row = new LinearLayout(this);
            row.setGravity(Gravity.CENTER_VERTICAL);
            row.setPadding(dp(11), dp(5), dp(5), dp(5));
            filesList.addView(row, new LinearLayout.LayoutParams(-1, dp(63)));
            ImageView icon = statusIcon(result.directory ? R.drawable.ic_files_folder
                    : R.drawable.ic_files_document, WHITE);
            row.addView(icon, new LinearLayout.LayoutParams(dp(23), dp(23)));
            LinearLayout details = new LinearLayout(this);
            details.setOrientation(LinearLayout.VERTICAL);
            LinearLayout.LayoutParams detailParams = new LinearLayout.LayoutParams(0, -2, 1);
            detailParams.leftMargin = dp(10);
            row.addView(details, detailParams);
            TextView name = text(result.name, 14, WHITE);
            name.setSingleLine(true);
            name.setEllipsize(android.text.TextUtils.TruncateAt.MIDDLE);
            details.addView(name);
            TextView location = text(result.parent, 10, MUTED);
            location.setSingleLine(true);
            location.setEllipsize(android.text.TextUtils.TruncateAt.START);
            details.addView(location);
            row.setOnClickListener(v -> revealFilesSearchResult(result));
            fileRule(filesList);
        }
        if (search.matches().isEmpty() && search.isFinished()) {
            TextView empty = text("No matches found", 13, MUTED);
            empty.setGravity(Gravity.CENTER);
            empty.setPadding(0, dp(25), 0, 0);
            filesList.addView(empty);
        }
    }

    private void revealFilesSearchResult(FilesGlobalSearch.Result result) {
        // Navigate inside the verified /ext tree; ordinary file actions still
        // operate only on fresh directory listings, never on search snippets.
        String destination = result.directory ? result.path : result.parent;
        if (!filesState.goToDirectory(destination)) return;
        filesSearchRevealName = result.directory ? null : result.name;
        requestedFilesPath = null;
        lastFilesError = null;
        filesSearchInput.setText(""); // Also cancels the active RPC search.
        updateFilesPanel();
    }

    private void requestCurrentFiles() {
        if (!flipperIsConnected() || filesState.hasListing()) return;
        if (filesState.path.equals(requestedFilesPath)) return;
        requestedFilesPath = filesState.path;
        lastFilesError = null;
        sendDeviceCommand(WeaveService.ACTION_LIST_FILES, "path", filesState.path);
    }

    private static String formatStorageBytes(long bytes) {
        if (bytes < 0) return "—";
        String[] units = {"B", "KiB", "MiB", "GiB", "TiB"};
        double amount = bytes;
        int unit = 0;
        while (amount >= 1024 && unit < units.length - 1) { amount /= 1024; unit++; }
        return (unit == 0 ? String.format(Locale.ROOT, "%.0f", amount)
                : String.format(Locale.ROOT, amount < 10 ? "%.2f" : amount < 100 ? "%.1f" : "%.0f", amount))
                + " " + units[unit];
    }

    private void updateFilesStorageUsage() {
        if (filesStorageUsage == null || filesStorageBar == null) return;
        if (!flipperIsConnected()) {
            filesStorageUsage.setText("Not connected");
            filesStorageBar.setProgress(0);
            return;
        }
        long total = prefs.getLong(WeaveService.KEY_STORAGE_EXT_TOTAL, -1);
        long free = prefs.getLong(WeaveService.KEY_STORAGE_EXT_FREE, -1);
        if (total <= 0 || free < 0 || free > total) {
            String error = prefs.getString(WeaveService.KEY_STORAGE_EXT_ERROR, null);
            filesStorageUsage.setText(error == null ? "Reading storage…" : "Storage unavailable");
            filesStorageBar.setProgress(0);
            return;
        }
        long used = total - free;
        filesStorageUsage.setText(formatStorageBytes(used) + " / " + formatStorageBytes(total));
        filesStorageBar.setProgress((int)Math.max(0, Math.min(1000,
                Math.round((double) used * 1000 / total))));
    }

    private void updateFilesPanel() {
        if (!filesPanelReady || filesList == null) return;
        boolean connected = flipperIsConnected();
        filesStorageName.setText("Flipper Zero"); // Only the microSD (/ext) is browsable.
        updateFilesStorageUsage();
        filesList.removeAllViews();
        if (connected && filesSearchTerm.isEmpty() && !filesState.hasListing() &&
                selectedPage == FILES && lastFilesError == null)
            requestCurrentFiles();

        if (connected && !filesSearchTerm.isEmpty()) {
            renderFilesSearchResults();
        } else if (!connected) {
            LinearLayout warning = new LinearLayout(this);
            warning.setOrientation(LinearLayout.VERTICAL);
            warning.setGravity(Gravity.CENTER);
            filesList.addView(warning, new LinearLayout.LayoutParams(-1, -1));
            ImageView icon = statusIcon(R.drawable.ic_terminal_warning, ORANGE);
            warning.addView(icon, new LinearLayout.LayoutParams(dp(54), dp(54)));
            TextView label = text("Flipper disconnected", 21, ORANGE);
            label.setGravity(Gravity.CENTER);
            label.setTypeface(Typeface.DEFAULT_BOLD);
            label.setPadding(dp(5), dp(14), dp(5), 0);
            warning.addView(label);
        } else if (!filesState.hasListing()) {
            LinearLayout notice = new LinearLayout(this);
            notice.setOrientation(LinearLayout.VERTICAL);
            notice.setGravity(Gravity.CENTER);
            filesList.addView(notice, new LinearLayout.LayoutParams(-1, -1));
            ImageView icon = statusIcon(R.drawable.ic_files_folder, MUTED);
            notice.addView(icon, new LinearLayout.LayoutParams(dp(39), dp(39)));
            filesNotice = text(lastFilesError == null ? "Reading Flipper files…" : lastFilesError, 13, MUTED);
            filesNotice.setGravity(Gravity.CENTER);
            filesNotice.setPadding(dp(8), dp(15), dp(8), 0);
            notice.addView(filesNotice);
        } else {
            if (!filesState.isAtRoot()) {
                TextView parent = text("↑   ..", 16, WHITE);
                parent.setPadding(dp(10), dp(16), dp(10), dp(16));
                filesList.addView(parent, new LinearLayout.LayoutParams(-1, -2));
                parent.setOnClickListener(v -> {
                    filesState.goParent(); requestedFilesPath = null; lastFilesError = null; updateFilesPanel();
                });
                fileRule(filesList);
            }
            for (FilesBrowserState.Entry file : filesState.entries()) {
                LinearLayout row = new LinearLayout(this);
                row.setOrientation(LinearLayout.HORIZONTAL);
                row.setGravity(Gravity.CENTER_VERTICAL);
                row.setPadding(dp(12), dp(10), dp(8), dp(10));
                LinearLayout.LayoutParams rowParam = new LinearLayout.LayoutParams(-1, dp(59));
                filesList.addView(row, rowParam);
                boolean selected = filesState.isSelected(file.name);
                row.setBackground(selected ? rounded(ORANGE_DARK, ORANGE, 10)
                        : rounded(0xff111111, 0xff111111, 10));
                ImageView fileIcon = statusIcon(file.directory ? R.drawable.ic_files_folder : R.drawable.ic_files_document,
                        selected ? ORANGE : WHITE);
                row.addView(fileIcon, new LinearLayout.LayoutParams(dp(26), dp(26)));
                TextView name = text(file.name, 15, selected ? ORANGE : WHITE);
                name.setPadding(dp(12), 0, 0, 0);
                name.setSingleLine(true);
                name.setEllipsize(android.text.TextUtils.TruncateAt.MIDDLE);
                row.addView(name, new LinearLayout.LayoutParams(0, -2, 1));
                if (file.directory) {
                    ImageView arrow = statusIcon(R.drawable.ic_files_chevron_right, MUTED);
                    row.addView(arrow, new LinearLayout.LayoutParams(dp(16), dp(16)));
                }
                row.setOnClickListener(v -> {
                    if (filesState.selectionCount() > 0) {
                        filesState.toggle(file.name);
                        updateFilesPanel();
                    } else if (file.directory) {
                        filesState.enterDirectory(file.name);
                        requestedFilesPath = null; lastFilesError = null;
                        updateFilesPanel();
                    } else {
                        filesState.toggle(file.name);
                        updateFilesPanel();
                    }
                });
                row.setOnLongClickListener(v -> {
                    filesState.toggle(file.name);
                    updateFilesPanel();
                    return true;
                });
                fileRule(filesList);
            }
            if (filesState.entries().isEmpty()) {
                TextView empty = text("Empty directory", 13, MUTED);
                empty.setGravity(Gravity.CENTER);
                empty.setPadding(0, dp(22), 0, 0);
                filesList.addView(empty);
            }
        }
        updateFilesActions(connected);
    }

    private void fileRule(LinearLayout list) {
        View line = new View(this);
        line.setBackgroundColor(0xff333333);
        list.addView(line, new LinearLayout.LayoutParams(-1, dp(1)));
    }

    private void updateFilesActions(boolean connected) {
        for (int i = 0; i < filesTools.length; i++) {
            if (filesTools[i] == null) continue;
            boolean supported = true;
            boolean enabled = connected && filesSearchTerm.isEmpty() && supported &&
                    filesState.hasListing() && filesState.allows(i);
            int color = enabled ? WHITE : 0xff5c6064;
            filesTools[i].setTextColor(enabled ? WHITE : color);
            View container = (View) filesTools[i].getParent();
            if (container instanceof LinearLayout) {
                View glyph = ((LinearLayout)container).getChildAt(0);
                if (glyph instanceof ImageView)
                    ((ImageView)glyph).setImageTintList(ColorStateList.valueOf(color));
                container.setAlpha(enabled ? 1f : 0.48f);
                container.setEnabled(enabled);
            }
        }
    }

    private void filesAction(int action) {
        if (!flipperIsConnected() || !filesState.hasListing() || !filesState.allows(action)) return;
        List<String> chosen = filesState.selectedNames();
        if (action == FilesBrowserState.COPY || action == FilesBrowserState.MOVE) {
            if (action == FilesBrowserState.COPY) filesState.prepareCopy();
            else filesState.prepareMove();
            toast((action == FilesBrowserState.COPY ? "Copied" : "Move") +
                    " selection. Open the destination and tap Paste.");
            updateFilesPanel();
        } else if (action == FilesBrowserState.PASTE) {
            beginPaste();
        } else if (action == FilesBrowserState.IMPORT) {
            Intent select = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            select.addCategory(Intent.CATEGORY_OPENABLE);
            select.setType("*/*");
            try { startActivityForResult(select, REQUEST_FILE_IMPORT); }
            catch (ActivityNotFoundException missingFileManager) { toast("No file manager available"); }
        } else if (action == FilesBrowserState.EXPORT && !chosen.isEmpty()) {
            startFilesExport(chosen);
        } else if (action == FilesBrowserState.NEW) {
            EditText name = new EditText(this);
            name.setSingleLine(true);
            name.setHint("New folder");
            new AlertDialog.Builder(this).setTitle("Create folder").setView(name)
                    .setNegativeButton("Cancel", null).setPositiveButton("Create", (dialog, which) -> {
                        String value = name.getText().toString().trim();
                        if (!FilesBrowserState.validName(value)) { toast("Invalid folder name"); return; }
                        sendDeviceCommand(WeaveService.ACTION_MKDIR, "path", filesState.path + "/" + value);
                    }).show();
        } else if (action == FilesBrowserState.RENAME && chosen.size() == 1) {
            String original = chosen.get(0);
            EditText name = new EditText(this);
            name.setSingleLine(true);
            name.setText(original);
            new AlertDialog.Builder(this).setTitle("Rename").setView(name)
                    .setNegativeButton("Cancel", null).setPositiveButton("Rename", (dialog, which) -> {
                        String value = name.getText().toString().trim();
                        if (!FilesBrowserState.validName(value)) { toast("Invalid name"); return; }
                        Intent command = new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_RENAME)
                                .putExtra("path", filesState.path + "/" + original)
                                .putExtra("newPath", filesState.path + "/" + value);
                        startService(command);
                    }).show();
        } else if (action == FilesBrowserState.DELETE && !chosen.isEmpty()) {
            new AlertDialog.Builder(this).setTitle("Delete " + chosen.size() + " item(s)?")
                    .setMessage("This will delete files on your Flipper. This cannot be undone.")
                    .setNegativeButton("Cancel", null).setPositiveButton("Delete", (dialog, which) -> {
                        // Queue destructive actions; never interleave RPC writes for different files.
                        String[] paths = new String[chosen.size()];
                        for (int i = 0; i < chosen.size(); i++)
                            paths[i] = filesState.path + "/" + chosen.get(i);
                        Intent command = new Intent(this, WeaveService.class)
                                .setAction(WeaveService.ACTION_DELETE_BATCH).putExtra("paths", paths);
                        startService(command);
                    }).show();
        } else {
            toast("This transfer action is not yet available over Weave RPC");
        }
    }

    private void startFilesExport(List<String> chosen) {
        if (chosen == null || chosen.isEmpty()) return;
        List<FilesBrowserState.Entry> selectedEntries = new ArrayList<>();
        for (String name : chosen) {
            for (FilesBrowserState.Entry entry : filesState.entries()) {
                if (entry.name.equals(name)) {
                    selectedEntries.add(entry);
                    break;
                }
            }
        }
        if (selectedEntries.isEmpty()) return;
        boolean singleFile = selectedEntries.size() == 1 && !selectedEntries.get(0).directory;
        Intent save = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        save.addCategory(Intent.CATEGORY_OPENABLE);
        if (singleFile) {
            FilesBrowserState.Entry entry = selectedEntries.get(0);
            save.setType("application/octet-stream");
            save.putExtra(Intent.EXTRA_TITLE, entry.name);
            exportedFile = null;
            sendDeviceCommand(WeaveService.ACTION_READ_FILE, "path", filesState.path + "/" + entry.name);
            return;
        }
        String title = selectedEntries.size() == 1 ? selectedEntries.get(0).name + ".zip" : "flipper-export.zip";
        save.setType("application/zip");
        save.putExtra(Intent.EXTRA_TITLE, title);
        try {
            startActivityForResult(save, REQUEST_FILE_EXPORT_ARCHIVE);
            archiveExportSession = new ArchiveExportSession(null);
            // Queue is filled only after the user has chosen the destination.
            archiveExportSession.queue.clear();
            for (FilesBrowserState.Entry entry : selectedEntries) {
                String fullPath = filesState.path + "/" + entry.name;
                archiveExportSession.queue.add(new ArchiveExportSession.PendingItem(fullPath, entry.name, entry.directory));
                archiveExportSession.pathToZip.put(fullPath, entry.name);
            }
        } catch (ActivityNotFoundException missingFileManager) {
            toast("No file manager available");
            archiveExportSession = null;
        }
    }

    private String nextDuplicateName(String originalName, Set<String> reservedNames) {
        String base = originalName;
        String extension = "";
        int dot = originalName.lastIndexOf('.');
        if (dot > 0) {
            base = originalName.substring(0, dot);
            extension = originalName.substring(dot);
        }
        int index = 1;
        String candidate;
        do {
            candidate = base + "_" + index + extension;
            index++;
        } while (reservedNames.contains(candidate));
        return candidate;
    }

    private void startArchiveExport(Uri destination) {
        if (archiveExportSession == null || destination == null) return;
        ArchiveExportSession session = new ArchiveExportSession(destination);
        for (ArchiveExportSession.PendingItem item : archiveExportSession.queue) session.queue.add(item);
        session.pathToZip.putAll(archiveExportSession.pathToZip);
        archiveExportSession = session;
        try {
            OutputStream output = getContentResolver().openOutputStream(destination);
            if (output == null) throw new IOException("Cannot create export archive");
            archiveExportSession.zip = new ZipOutputStream(output);
            continueArchiveExport();
        } catch (IOException error) {
            closeArchiveExport(false, "Export failed");
        }
    }

    private void continueArchiveExport() {
        ArchiveExportSession session = archiveExportSession;
        if (session == null) return;
        if (session.awaitingFilePath != null || session.awaitingListPath != null) return;
        if (session.queue.isEmpty()) {
            closeArchiveExport(true, "Archive exported");
            return;
        }
        ArchiveExportSession.PendingItem item = session.queue.poll();
        if (item == null) {
            closeArchiveExport(true, "Archive exported");
            return;
        }
        if (item.directory) {
            try {
                session.zip.putNextEntry(new ZipEntry(item.zipPath + "/"));
                session.zip.closeEntry();
            } catch (IOException error) {
                closeArchiveExport(false, "Export failed");
                return;
            }
            session.awaitingListPath = item.path;
            sendDeviceCommand(WeaveService.ACTION_LIST_FILES, "path", item.path);
        } else {
            session.awaitingFilePath = item.path;
            sendDeviceCommand(WeaveService.ACTION_READ_FILE, "path", item.path);
        }
    }

    private void onArchiveListing(String path, boolean ok, String[] names, boolean[] directories, long[] sizes) {
        ArchiveExportSession session = archiveExportSession;
        if (session == null || path == null || !path.equals(session.awaitingListPath)) return;
        session.awaitingListPath = null;
        if (!ok || names == null || directories == null) {
            closeArchiveExport(false, "Export failed");
            return;
        }
        String zipPrefix = session.pathToZip.get(path);
        if (zipPrefix == null) zipPrefix = path.substring(path.lastIndexOf('/') + 1);
        for (int i = names.length - 1; i >= 0; i--) {
            if (names[i] == null || !FilesBrowserState.validName(names[i])) continue;
            boolean isDir = i < directories.length && directories[i];
            String childPath = path + "/" + names[i];
            String childZip = zipPrefix.isEmpty() ? names[i] : zipPrefix + "/" + names[i];
            session.pathToZip.put(childPath, childZip);
            session.queue.addFirst(new ArchiveExportSession.PendingItem(childPath, childZip, isDir));
        }
        continueArchiveExport();
    }

    private void onArchiveFile(String path, byte[] bytes, String error) {
        ArchiveExportSession session = archiveExportSession;
        if (session == null || path == null || !path.equals(session.awaitingFilePath)) return;
        session.awaitingFilePath = null;
        if (bytes == null) {
            closeArchiveExport(false, error == null ? "Export failed" : error);
            return;
        }
        String zipPath = session.pathToZip.get(path);
        if (zipPath == null) zipPath = path.substring(path.lastIndexOf('/') + 1);
        try {
            session.zip.putNextEntry(new ZipEntry(zipPath));
            session.zip.write(bytes);
            session.zip.closeEntry();
        } catch (IOException ioError) {
            closeArchiveExport(false, "Export failed");
            return;
        }
        continueArchiveExport();
    }

    private void closeArchiveExport(boolean success, String message) {
        ArchiveExportSession session = archiveExportSession;
        archiveExportSession = null;
        if (session != null && session.zip != null) {
            try { session.zip.close(); } catch (IOException ignored) { }
        }
        toast(message);
    }

    private void beginPaste() {
        if (pasteRunning || !filesState.hasListing() || !filesState.hasClipboard()) return;
        List<String> sources = filesState.clipboardSources();
        boolean move = filesState.isMoveClipboard();
        pasteSources.clear();
        pasteTargets.clear();
        Set<String> reservedNames = new HashSet<>();
        for (FilesBrowserState.Entry destination : filesState.entries()) reservedNames.add(destination.name);
        for (String source : sources) {
            if (source == null || !source.startsWith(filesState.storage + "/")) {
                toast("Cannot paste between internal storage and SD card"); return;
            }
            String name = source.substring(source.lastIndexOf('/') + 1);
            if (name.isEmpty() || filesState.path.equals(source) || filesState.path.startsWith(source + "/")) {
                toast("Cannot paste a folder into itself"); return;
            }
            String sourceFolder = source.substring(0, source.lastIndexOf('/'));
            String finalName = name;
            if (move && sourceFolder.equals(filesState.path)) {
                toast("Open another folder to move these items");
                return;
            }
            if (!move && reservedNames.contains(finalName)) finalName = nextDuplicateName(finalName, reservedNames);
            reservedNames.add(finalName);
            pasteSources.add(source);
            pasteTargets.add(filesState.path + "/" + finalName);
        }
        if (!move && filesState.clipboardHasDirectories()) {
            toast("Folder copy is not supported yet; select individual files."); return;
        }
        pasteDestination = filesState.path;
        pasteIsMove = move;
        pasteRunning = true;
        runNextPaste();
    }

    private void runNextPaste() {
        if (!pasteRunning) return;
        if (pasteSources.isEmpty() || pasteTargets.isEmpty()) {
            pasteRunning = false;
            pasteReadSource = null;
            pasteWriteTarget = null;
            if (pasteIsMove) filesState.clearClipboard();
            toast("Flipper confirmed the paste operations");
            filesState.invalidateListing();
            requestCurrentFiles();
            return;
        }
        String source = pasteSources.peek();
        String target = pasteTargets.peek();
        if (pasteIsMove) {
            startService(new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_RENAME)
                    .putExtra("path", source).putExtra("newPath", target));
        } else {
            pasteReadSource = source;
            pasteWriteTarget = target;
            sendDeviceCommand(WeaveService.ACTION_READ_FILE, "path", source);
        }
    }

    private FrameLayout navigation() {
        // This FrameLayout contains one moving highlight and a fixed row of buttons.
        // The highlight is behind the buttons, so clicks and accessibility still work.
        navigationFrame = new FrameLayout(this);
        navigationFrame.setBackgroundColor(SURFACE);
        navigationPill = new View(this);
        navigationPill.setBackground(rounded(ORANGE_DARK, ORANGE_DARK, 14));
        navigationPill.setImportantForAccessibility(View.IMPORTANT_FOR_ACCESSIBILITY_NO);
        navigationFrame.addView(navigationPill, new FrameLayout.LayoutParams(0, 0));

        LinearLayout bar = new LinearLayout(this);
        navigationTabRow = bar;
        bar.setGravity(Gravity.CENTER);
        bar.setPadding(dp(6), dp(6), dp(6), dp(8));
        int[] icons = {R.drawable.ic_nav_home, R.drawable.ic_nav_apps,
                R.drawable.ic_nav_console, R.drawable.ic_nav_files, R.drawable.ic_nav_settings};
        String[] names = {"Home", "Apps", "Terminal", "Files", "Settings"};
        for (int i = 0; i < TABS; i++) {
            final int index = i;
            TextView tab = text(names[i], 10, MUTED);
            tab.setTypeface(Typeface.DEFAULT_BOLD);
            tab.setGravity(Gravity.CENTER);
            tab.setCompoundDrawablesWithIntrinsicBounds(0, icons[i], 0, 0);
            tab.setCompoundDrawablePadding(dp(4));
            tab.setPadding(dp(2), dp(5), dp(2), dp(4));
            tab.setOnClickListener(v -> selectPage(index));
            tabs[i] = tab;
            LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(0, -1, 1);
            lp.leftMargin = dp(2);
            lp.rightMargin = dp(2);
            bar.addView(tab, lp);
        }
        navigationFrame.addView(bar, new FrameLayout.LayoutParams(-1, -1));
        // Also realign without animation after rotation or other size changes.
        navigationFrame.addOnLayoutChangeListener((v, l, t, r, b, ol, ot, oright, ob) -> {
            if (r - l != oright - ol || b - t != ob - ot) positionNavigationPill(false);
        });
        return navigationFrame;
    }

    private void positionNavigationPill(boolean animate) {
        if (navigationPill == null || navigationTabRow == null || tabs[selectedPage] == null) return;
        TextView destination = tabs[selectedPage];
        if (destination.getWidth() <= 0 || destination.getHeight() <= 0) return;
        if (navigationPillAnimator != null) {
            ValueAnimator previousAnimator = navigationPillAnimator;
            navigationPillAnimator = null;
            previousAnimator.cancel(); // Keep the in-flight position for a rapid second tap.
        }
        FrameLayout.LayoutParams lp = (FrameLayout.LayoutParams)navigationPill.getLayoutParams();
        int targetWidth = destination.getWidth();
        int targetHeight = destination.getHeight();
        int top = navigationTabRow.getTop() + destination.getTop();
        if (lp.width != targetWidth || lp.height != targetHeight || lp.topMargin != top) {
            lp.width = targetWidth;
            lp.height = targetHeight;
            lp.topMargin = top;
            navigationPill.setLayoutParams(lp);
        }
        float targetX = navigationTabRow.getLeft() + destination.getLeft();
        if (!navigationPillPlaced || !animate) {
            navigationPill.setTranslationX(targetX);
            navigationPill.setScaleX(1f);
            navigationPill.setScaleY(1f);
            navigationPillPlaced = true;
            return;
        }
        float initialX = navigationPill.getTranslationX();
        if (Math.abs(targetX - initialX) < 0.5f) return;
        // Smooth ease-out, no bounce or overshoot outside the navigation bar.
        ValueAnimator animator = ValueAnimator.ofFloat(initialX, targetX);
        navigationPillAnimator = animator;
        float distanceInTabs = Math.abs(targetX - initialX) / Math.max(1f, targetWidth);
        animator.setDuration((long)Math.min(450f, 290f + 35f * distanceInTabs));
        animator.setInterpolator(new PathInterpolator(0.22f, 1f, 0.36f, 1f));
        animator.addUpdateListener(animation -> {
            navigationPill.setTranslationX((float) animation.getAnimatedValue());
            // A subtle stretch gives the moving highlight a soft, fluid feel.
            float stretch = (float)Math.sin(Math.PI * animation.getAnimatedFraction());
            navigationPill.setScaleX(1f + 0.06f * stretch);
            navigationPill.setScaleY(1f - 0.015f * stretch);
        });
        animator.addListener(new AnimatorListenerAdapter() {
            @Override public void onAnimationEnd(Animator animation) {
                if (navigationPillAnimator != animator) return;
                navigationPill.setTranslationX(targetX);
                navigationPill.setScaleX(1f);
                navigationPill.setScaleY(1f);
                navigationPillAnimator = null;
            }
        });
        animator.start();
    }

    private void selectPage(int selected) {
        int previousPage = selectedPage;
        if (appDetailEntry != null) closeAppDetail();
        if (settingsInfoOpen && selected != SETTINGS) showSettingsMain();
        selectedPage = selected < 0 || selected >= TABS ? HOME : selected;
        if (previousPage != selectedPage) {
            if (selectedPage == HOME) {
                // A cached frame is never evidence that the newly visible
                // Home shows the current physical Flipper display.
                screenAwaitingFreshFrame = true;
                screenRecoveryFailed = false;
                homeScreenRefreshRequestedAt = 0;
                if (flipperScreenStatus != null && flipperIsConnected()) {
                    flipperScreenStatus.setText("Synchronizing Flipper display…");
                    flipperScreenStatus.setVisibility(View.VISIBLE);
                }
                if (flipperScreenImage != null && lastScreenBitmap != null)
                    flipperScreenImage.setAlpha(0.28f);
            } else if (previousPage == HOME) {
                handler.removeCallbacks(homeScreenRefreshRetry);
                homeScreenRefreshRequestedAt = 0;
            }
        }
        if (previousPage == FILES && selectedPage != FILES) {
            handler.removeCallbacks(delayedFilesSearch);
            handler.removeCallbacks(nextFilesSearchStep);
            handler.removeCallbacks(filesSearchFolderTimeout);
            filesGlobalSearch = null; // Do not monopolize BLE in another tab.
            // Its in-flight directory reply may be ignored offscreen; allow a
            // fresh request if the same directory is reopened.
            requestedFilesPath = null;
        }
        if (previousPage == APPS && selectedPage != APPS) {
            handler.removeCallbacks(delayedCatalogSearch);
            handler.removeCallbacks(delayedAppsFilter);
            ++catalogSearchGeneration;
            if (appCatalogClient != null) appCatalogClient.cancelSearch();
            catalogSearchLoading = false;
            // Pause the animation, not the actual catalog/inventory operation.
            if (appsRefreshAnimator != null) {
                appsRefreshAnimator.cancel();
                appsRefreshAnimator = null;
            }
        }
        if (selectedPage == FILES && !filesSearchTerm.isEmpty() && filesGlobalSearch == null)
            handler.postDelayed(delayedFilesSearch, 200);
        if (selectedPage == SETTINGS && previousPage != SETTINGS && flipperIsConnected() &&
                updateCurrentVersion(UPDATE_FIRMWARE) == null)
            requestInstalledFirmwareVersion();
        if (gestureRefreshActive && selectedPage != gestureRefreshPage) finishGestureRefresh();
        String[] names = {"Home", "Apps", "Terminal", "Files", "Settings"};
        pageTitle.setText(names[selectedPage]);
        if (selectedPage == CONSOLE) {
            renderTerminalPanel(previousPage != CONSOLE);
            if (!cliBleConnected && !cliUsbConnected)
                handler.postDelayed(() -> {
                    if (selectedPage == CONSOLE && !cliBleConnected && !cliUsbConnected)
                        connectNativeCli();
                }, 120);
        }
        if (selectedPage == FILES) {
            updateFilesPanel();
            if (previousPage != FILES && flipperIsConnected())
                sendDeviceCommand(WeaveService.ACTION_STORAGE_INFO,
                        "path", FilesBrowserState.SD);
        }
        if (selectedPage == APPS) {
            if (appsMode != 0) ensureInstalledApps();
            if (appsRefreshInProgress && appsRefreshAnimator == null)
                startAppsRefreshIconOnly();
            resolveInstalledMetadata(); // Defer Apps-only lookups until visible.
            // The public catalog may have changed while another page was visible.
            renderAppsList();
            if (appsMode == 0 && catalogSearchTerm.length() >= 2 &&
                    catalogSearchResults.isEmpty()) {
                handler.removeCallbacks(delayedCatalogSearch);
                handler.postDelayed(delayedCatalogSearch, 450);
            }
        }
        for (int i = 0; i < TABS; i++) {
            pages[i].setVisibility(i == selectedPage ? View.VISIBLE : View.GONE);
            int color = i == selectedPage ? ORANGE : MUTED;
            tabs[i].setTextColor(color);
            tabs[i].setSelected(i == selectedPage);
            tabs[i].setCompoundDrawableTintList(ColorStateList.valueOf(color));
            // Selected background is provided by the shared animated navigationPill.
        }
        if (previousPage != selectedPage || !navigationPillPlaced)
            positionNavigationPill(previousPage != selectedPage);
        if (previousPage != selectedPage) {
            View incoming = pages[selectedPage];
            incoming.animate().cancel();
            incoming.setAlpha(0.72f);
            incoming.setTranslationX(dp(previousPage < selectedPage ? 17 : -17));
            incoming.setTranslationY(dp(3));
            incoming.animate().alpha(1f).translationX(0f).translationY(0f)
                    .setInterpolator(new DecelerateInterpolator())
                    .setDuration(210).start();
        }
        updateInstallRing();
        if (appForeground && previousPage != selectedPage) {
            updateScreenSubscription();
            // Rehydrate only the newly visible page from the shared state.
            // Home waits for a real fresh framebuffer instead of an old cached one.
            refresh();
            handler.removeCallbacks(refresher);
            handler.postDelayed(refresher, PageWorkPolicy.refreshDelayMillis(
                    selectedPage, appsRefreshInProgress || activeBadgeJob() != null));
        }
    }

    private void updateScreenSubscription() {
        boolean wanted = appForeground && selectedPage == HOME;
        boolean previous = prefs.getBoolean("screen_requested", false);
        if (wanted == previous) return;
        prefs.edit().putBoolean("screen_requested", wanted).apply();
        if (flipperIsConnected()) command(wanted ? WeaveService.ACTION_START_SCREEN : WeaveService.ACTION_STOP_SCREEN);
    }

    private LinearLayout pageRoot() {
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(dp(20), dp(12), dp(20), dp(30));
        return root;
    }
    private ScrollView scroll(LinearLayout root) {
        ScrollView view = new ScrollView(this);
        view.setFillViewport(true);
        view.setVerticalScrollBarEnabled(false);
        view.addView(root);
        return view;
    }
    private TextView section(String title) {
        TextView view = text(title, 13, MUTED);
        view.setTypeface(Typeface.DEFAULT_BOLD);
        view.setPadding(0, dp(24), 0, dp(9));
        return view;
    }
    private LinearLayout card(boolean raised) {
        LinearLayout view = new LinearLayout(this);
        view.setOrientation(LinearLayout.VERTICAL);
        view.setPadding(dp(17), dp(17), dp(17), dp(17));
        view.setBackground(rounded(raised ? 0xff151515 : CARD, BORDER, 18));
        return view;
    }
    private LinearLayout.LayoutParams cardParams() {
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, -2);
        params.topMargin = dp(7);
        return params;
    }
    private GradientDrawable rounded(int color, int stroke, int radius) {
        GradientDrawable drawable = new GradientDrawable();
        drawable.setColor(color);
        drawable.setCornerRadius(dp(radius));
        drawable.setStroke(dp(1), stroke);
        return drawable;
    }
    private TextView text(String text, float size, int color) {
        TextView view = new TextView(this);
        view.setText(text);
        view.setTextSize(size);
        view.setTextColor(color);
        return view;
    }
    private void divider(LinearLayout parent) {
        View line = new View(this);
        line.setBackgroundColor(BORDER);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(-1, dp(1));
        params.topMargin = dp(12);
        params.bottomMargin = dp(12);
        parent.addView(line, params);
    }
    private TextView valueRow(LinearLayout parent, String label, String initial) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        TextView key = text(label, 13, MUTED);
        row.addView(key, new LinearLayout.LayoutParams(0, -2, 1));
        TextView value = text(initial, 13, WHITE);
        value.setGravity(Gravity.END);
        row.addView(value);
        parent.addView(row);
        return value;
    }
    private TextView metric(LinearLayout parent, String label, boolean second) {
        LinearLayout cell = card(false);
        cell.setPadding(dp(12), dp(14), dp(12), dp(14));
        TextView number = text("0", 17, WHITE);
        number.setTypeface(Typeface.DEFAULT_BOLD);
        cell.addView(number);
        TextView caption = text(label, 11, MUTED);
        caption.setPadding(0, dp(5), 0, 0);
        cell.addView(caption);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(0, -2, 1);
        if (second) params.leftMargin = dp(8);
        parent.addView(cell, params);
        return number;
    }
    private Switch setting(LinearLayout parent, String title, String detail,
                           CompoundButton.OnCheckedChangeListener listener) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        LinearLayout labels = new LinearLayout(this);
        labels.setOrientation(LinearLayout.VERTICAL);
        TextView main = text(title, 14, WHITE);
        main.setTypeface(Typeface.DEFAULT_BOLD);
        labels.addView(main);
        TextView sub = text(detail, 12, MUTED);
        sub.setPadding(0, dp(5), 0, 0);
        labels.addView(sub);
        row.addView(labels, new LinearLayout.LayoutParams(0, -2, 1));
        Switch toggle = new Switch(this);
        toggle.setShowText(false);
        toggle.setSwitchMinWidth(dp(54));
        toggle.setMinimumWidth(dp(54));
        toggle.setTextColor(WHITE);
        toggle.setTextSize(11);
        toggle.setTypeface(Typeface.DEFAULT_BOLD);
        toggle.setPadding(dp(6), 0, 0, 0);
        toggle.setThumbTintList(new ColorStateList(new int[][]{
                new int[]{android.R.attr.state_checked}, new int[]{}}, new int[]{ORANGE, 0xff777777}));
        toggle.setTrackTintList(new ColorStateList(new int[][]{
                new int[]{android.R.attr.state_checked}, new int[]{}},
                new int[]{0xff99440c, 0xff303030}));
        toggle.setOnCheckedChangeListener(listener);
        row.addView(toggle);
        row.setOnClickListener(v -> toggle.setChecked(!toggle.isChecked()));
        parent.addView(row);
        return toggle;
    }
    private void action(LinearLayout parent, int iconRes, String title, String description,
                          View.OnClickListener listener) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER_VERTICAL);
        row.setPadding(0, dp(15), 0, dp(15));
        row.addView(statusIcon(iconRes, WHITE),
                new LinearLayout.LayoutParams(dp(27), dp(27)));
        LinearLayout labels = new LinearLayout(this);
        labels.setOrientation(LinearLayout.VERTICAL);
        TextView name = text(title, 14, WHITE);
        name.setTypeface(Typeface.DEFAULT_BOLD);
        labels.addView(name);
        TextView hint = text(description, 12, MUTED);
        hint.setPadding(0, dp(4), 0, 0);
        labels.addView(hint);
        LinearLayout.LayoutParams labelParams = new LinearLayout.LayoutParams(0, -2, 1);
        labelParams.leftMargin = dp(14);
        row.addView(labels, labelParams);
        row.addView(statusIcon(R.drawable.ic_files_chevron_right, MUTED),
                new LinearLayout.LayoutParams(dp(20), dp(20)));
        row.setOnClickListener(listener);
        row.setContentDescription(title);
        row.setClickable(true);
        row.setFocusable(true);
        parent.addView(row);
    }
    private int dp(int px) { return Math.round(px * getResources().getDisplayMetrics().density); }

    private void toggleService(boolean enabled) {
        if (syncing) return;
        prefs.edit().putBoolean(WeaveService.KEY_ENABLED, enabled).apply();
        if (enabled) {
            // On Android 12+, connectedDevice FGS needs granted Bluetooth permission.
            // Never start it prematurely then silently disable the user's service.
            if (!hasBlePermission()) {
                prefs.edit().putString(WeaveService.KEY_STATUS,
                        "Grant Nearby devices permission to start Bluetooth service").apply();
                requestBlePermission();
            } else {
                startEnabledService();
                finishServiceSetup();
            }
        } else {
            stopService(new Intent(this, WeaveService.class));
            prefs.edit().putBoolean(WeaveService.KEY_ACTIVE, false)
                    .putBoolean(WeaveService.KEY_CONNECTED, false)
                    .putBoolean(WeaveService.KEY_WV1, false).apply();
        }
        refresh();
    }

    private void startEnabledService() {
        if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false) || !hasBlePermission()) return;
        Intent intent = new Intent(this, WeaveService.class).setAction(WeaveService.ACTION_START);
        try {
            if (Build.VERSION.SDK_INT >= 26) startForegroundService(intent);
            else startService(intent);
        } catch (RuntimeException error) {
            prefs.edit().putString(WeaveService.KEY_STATUS,
                    "Android blocked background service: " + error.getClass().getSimpleName()).apply();
            toast("Android blocked Weave — check app permissions and battery settings");
        }
    }

    private void finishServiceSetup() {
        BluetoothManager manager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = manager == null ? null : manager.getAdapter();
        if (adapter != null && !adapter.isEnabled()) {
            ensureBluetoothEnabled();
            return;
        }
        if (Build.VERSION.SDK_INT >= 33 &&
                checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            askNotificationPermission();
            return;
        }
        showBatteryGuidance();
    }

    private void ensureBluetoothEnabled() {
        if (!hasBlePermission()) return;
        BluetoothManager manager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        BluetoothAdapter adapter = manager == null ? null : manager.getAdapter();
        if (adapter != null && !adapter.isEnabled()) {
            try { startActivityForResult(new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE), REQUEST_ENABLE_BLUETOOTH); }
            catch (RuntimeException error) { toast("Enable Bluetooth from Android settings"); }
        }
    }

    private void showBatteryGuidance() {
        if (Build.VERSION.SDK_INT < 23) return;
        PowerManager power = (PowerManager) getSystemService(POWER_SERVICE);
        if (power == null || power.isIgnoringBatteryOptimizations(getPackageName())) return;
        // Only show the explanation once, when the user opts in. Android must
        // make the battery policy decision; the app never bypasses the system.
        if (prefs.getBoolean("battery_guidance_shown", false)) return;
        prefs.edit().putBoolean("battery_guidance_shown", true).apply();
        new AlertDialog.Builder(this)
                .setTitle("Keep Weave connected")
                .setMessage("Weave runs a foreground service and reconnects to your Flipper over Bluetooth. "
                        + "Android battery restrictions may still interrupt it. Allow background operation "
                        + "for Weave in your phone's battery settings.")
                .setPositiveButton("Battery settings", (dialog, which) -> openBatterySettings())
                .setNegativeButton("Later", null).show();
    }

    private void openBatterySettings() {
        try { startActivity(new Intent(Settings.ACTION_IGNORE_BATTERY_OPTIMIZATION_SETTINGS)); }
        catch (RuntimeException unavailable) {
            try { startActivity(new Intent(Settings.ACTION_APPLICATION_DETAILS_SETTINGS,
                    Uri.parse("package:" + getPackageName()))); }
            catch (RuntimeException ignored) { toast("Open Weave > Battery in Android settings"); }
        }
    }

    private void command(String action) {
        if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false)) {
            toast("Enable the service first");
            return;
        }
        Intent intent = new Intent(this, WeaveService.class).setAction(action);
        try {
            if (Build.VERSION.SDK_INT >= 26) startForegroundService(intent);
            else startService(intent);
        } catch (RuntimeException error) {
            toast("Unable to start Weave service");
        }
    }

    private void connectUsb() {
        if (!prefs.getBoolean(WeaveService.KEY_ENABLED, false)) {
            toast("Enable the service first"); return;
        }
        UsbLink probe = new UsbLink(this, new UsbLink.Callbacks() {
            @Override public void status(String message) { }
            @Override public void ready() { }
            @Override public void bytes(byte[] bytes) { }
            @Override public void disconnected() { }
        });
        List<UsbDevice> devices = probe.discover();
        if (devices.isEmpty()) { toast("Flipper USB CDC not detected — check USB data/OTG mode"); return; }
        UsbDevice device = devices.get(0);
        UsbManager manager = (UsbManager) getSystemService(USB_SERVICE);
        if (manager.hasPermission(device)) {
            command(WeaveService.ACTION_USB);
        } else {
            pendingUsb = device;
            PendingIntent permission = PendingIntent.getBroadcast(this, 0,
                    new Intent(USB_PERMISSION).setPackage(getPackageName()),
                    PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
            manager.requestPermission(device, permission);
        }
    }

    private boolean hasBlePermission() {
        if (Build.VERSION.SDK_INT >= 31) {
            return checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED
                    && checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
        }
        return Build.VERSION.SDK_INT < 23
                || checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED;
    }
    private void requestBlePermission() {
        blePermissionPromptedThisSession = true;
        if (Build.VERSION.SDK_INT >= 31) requestPermissions(new String[]{
                Manifest.permission.BLUETOOTH_SCAN, Manifest.permission.BLUETOOTH_CONNECT}, REQUEST_BLE);
        else if (Build.VERSION.SDK_INT >= 23) requestPermissions(new String[]{
                Manifest.permission.ACCESS_FINE_LOCATION}, REQUEST_BLE);
    }
    private void scanBle() {
        if (!hasBlePermission()) { requestBlePermission(); return; }
        command(WeaveService.ACTION_SCAN);
    }
    private void askNotificationPermission() {
        if (Build.VERSION.SDK_INT >= 33 &&
                checkSelfPermission(Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED)
            requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, REQUEST_NOTIFICATIONS);
    }
    @Override public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grants) {
        super.onRequestPermissionsResult(requestCode, permissions, grants);
        if (requestCode == REQUEST_BLE) {
            if (hasBlePermission()) {
                if (prefs.getBoolean(WeaveService.KEY_ENABLED, false)) {
                    startEnabledService();
                    command(WeaveService.ACTION_SCAN);
                    finishServiceSetup();
                } else if (prefs.getBoolean(WeaveService.KEY_BOOT, false)) showBatteryGuidance();
            } else {
                prefs.edit().putString(WeaveService.KEY_STATUS,
                        "Nearby devices permission denied — BLE connection unavailable").apply();
                if (!"USB".equals(prefs.getString(WeaveService.KEY_MODE, "Auto")))
                    prefs.edit().putBoolean(WeaveService.KEY_ENABLED, false).apply();
                toast("Allow Nearby devices permission to use Flipper Bluetooth");
                refresh();
            }
        } else if(requestCode == REQUEST_PHONE_STATE) {
            boolean denied = false;
            for(int grant : grants) if(grant != PackageManager.PERMISSION_GRANTED) denied = true;
            if(denied) toast("Some alerts may be limited; notification access is also required");
            startActivity(new Intent(Settings.ACTION_NOTIFICATION_LISTENER_SETTINGS));
        } else if (requestCode == REQUEST_NOTIFICATIONS &&
                prefs.getBoolean(WeaveService.KEY_ENABLED, false)) {
            showBatteryGuidance();
        }
    }
    private void selectMode() {
        String[] modes = {"Auto", "USB", "Bluetooth"};
        String current = prefs.getString(WeaveService.KEY_MODE, "Auto");
        int checked = "USB".equals(current) ? 1 : ("Bluetooth".equals(current) ? 2 : 0);
        new AlertDialog.Builder(this).setTitle("Preferred connection")
                .setSingleChoiceItems(modes, checked, (dialog, selected) -> {
                    prefs.edit().putString(WeaveService.KEY_MODE, modes[selected]).apply();
                    dialog.dismiss();
                    if (prefs.getBoolean(WeaveService.KEY_ACTIVE, false)) command(WeaveService.ACTION_DISCONNECT);
                    if (selected == 2 && !hasBlePermission()) requestBlePermission();
                    refresh();
                }).setNegativeButton("Cancel", null).show();
    }

    private void refresh() {
        updateInstallRing();
        if (appsRefreshInProgress && !appsRefreshInventoryDone) {
            String status = prefs.getString(WeaveService.KEY_INVENTORY_STATUS, "");
            if ("ready".equals(status) || "unavailable".equals(status)) {
                appsRefreshInventoryDone = true;
                checkAppsRefreshCompletion();
            }
        }
        boolean enabled = prefs.getBoolean(WeaveService.KEY_ENABLED, false);
        boolean active = enabled && prefs.getBoolean(WeaveService.KEY_ACTIVE, false);
        boolean connected = active && prefs.getBoolean(WeaveService.KEY_CONNECTED, false);
        String transport = prefs.getString(WeaveService.KEY_TRANSPORT, "");
        syncing = true;
        if (selectedPage == HOME && homeService != null && homeService.isChecked() != enabled)
            homeService.setChecked(enabled);
        if (selectedPage == SETTINGS) {
            if (serviceSwitch != null && serviceSwitch.isChecked() != enabled)
                serviceSwitch.setChecked(enabled);
            boolean boot = prefs.getBoolean(WeaveService.KEY_BOOT, false);
            if (bootSwitch != null && bootSwitch.isChecked() != boot)
                bootSwitch.setChecked(boot);
        }
        syncing = false;
        if (!connected) {
            screenAwaitingFreshFrame = true;
            screenRecoveryFailed = false;
        }
        if (selectedPage == HOME && flipperScreenStatus != null) {
            if (!connected) {
                flipperScreenStatus.setText(lastScreenBitmap == null
                        ? "Flipper\ndisconnected" : "Flipper disconnected\nReconnecting…");
                flipperScreenStatus.setVisibility(View.VISIBLE);
                // Preserve the last REAL frame underneath the reconnecting message,
                // clearly dimmed so it is not mistaken for a live feed.
                if (flipperScreenImage != null && lastScreenBitmap != null)
                    flipperScreenImage.setAlpha(0.28f);
            } else {
                if (flipperScreenImage != null) flipperScreenImage.setAlpha(1f);
                if (lastScreenBitmap == null || screenAwaitingFreshFrame) {
                    flipperScreenStatus.setText(screenRecoveryFailed
                            ? "Flipper display not responding\nCheck connection and retry"
                            : homeScreenRefreshRequestedAt != 0
                                    ? "Resynchronizing Flipper display…"
                                    : "Connecting to Flipper screen…");
                    flipperScreenStatus.setVisibility(View.VISIBLE);
                    if (flipperScreenImage != null && lastScreenBitmap != null)
                        flipperScreenImage.setAlpha(0.28f);
                } else flipperScreenStatus.setVisibility(View.GONE);
            }
        }
        if (selectedPage == HOME) {
            updateRemoteDisplayTransferState();
            String battery = connected ? prefs.getString(WeaveService.KEY_BATTERY, "") : "";
            if (batteryDetailHome != null) batteryDetailHome.setText(battery.isEmpty() ? "—" : battery + "%");
            if (batteryIconHome != null) batteryIconHome.setImageTintList(ColorStateList.valueOf(GREEN));
            if (btValueHome != null) btValueHome.setText(connected && "Bluetooth".equalsIgnoreCase(transport) ? "Connected" : "Disconnected");
            if (usbValueHome != null) usbValueHome.setText(connected && "USB".equalsIgnoreCase(transport) ? "Connected" : "Disconnected");
        }
        String inventorySignature = selectedPage == APPS
                ? prefs.getString(WeaveService.KEY_INVENTORY_STATUS, "") + ":"
                        + prefs.getString(WeaveService.KEY_INVENTORY, "") : "";
        if (!connected) {
            shopCatalogSyncedThisConnection = false;
            handler.removeCallbacks(delayedShopIconSync);
        }
        else if (!shopCatalogSyncedThisConnection && !catalogApps.isEmpty()) {
            shopCatalogSyncedThisConnection = true;
            handler.postDelayed(this::syncShopCatalogToFlipper, 3500);
        }
        if (selectedPage == APPS && appsListHost != null &&
                (appsLastLinkState != connected ||
                !inventorySignature.equals(appsLastInventorySignature))) {
            appsLastLinkState = connected;
            appsLastInventorySignature = inventorySignature;
            renderAppsList();
        }
        if (selectedPage == APPS) reconcileInstallDetail();
        if (selectedPage == SETTINGS) {
            if (modeValue != null) modeValue.setText(
                    prefs.getString(WeaveService.KEY_MODE, "Auto") + "  ›");
            if (statusLine != null) statusLine.setText(
                    prefs.getString(WeaveService.KEY_STATUS, "Waiting for Weave link"));
            if (hint != null) hint.setText(connected ?
                    (prefs.getBoolean(WeaveService.KEY_WV1, false) ? "Weave WV1 gateway active." :
                        "Flipper BLE connected; remote commands require an RPC gateway.") :
                    "Waiting for a verified Flipper Bluetooth or WV1 connection.");
            if (usbButton != null) usbButton.setEnabled(enabled);
            if (bleScanButton != null) bleScanButton.setEnabled(enabled);
            if (bleConnectButton != null) bleConnectButton.setEnabled(enabled);
        }
        updateFlipperHeader(connected);
        if (filesPanelReady && filesLastConnected != connected) {
            filesLastConnected = connected;
            filesState.resetForConnectionChange();
            handler.removeCallbacks(nextFilesSearchStep);
            handler.removeCallbacks(filesSearchFolderTimeout);
            filesGlobalSearch = null;
            if (connected && selectedPage == FILES && !filesSearchTerm.isEmpty())
                handler.postDelayed(delayedFilesSearch, 250);
            requestedFilesPath = null;
            lastFilesError = null;
            if (selectedPage == FILES) {
                updateFilesPanel();
                if (connected)
                    sendDeviceCommand(WeaveService.ACTION_STORAGE_INFO,
                            "path", FilesBrowserState.SD);
            }
        }
        if (selectedPage == CONSOLE) renderTerminalPanel(false);
        if (selectedPage == SETTINGS && !settingsInfoOpen) {
            if (settingsLastConnected != connected || !settingsLastTransport.equals(transport)) {
                showSettingsMain();
                if (connected && updateCurrentVersion(UPDATE_FIRMWARE) == null &&
                        !firmwareVersionReadPending) requestInstalledFirmwareVersion();
            } else {
                String nowFirmware = prefs.getString(WeaveService.KEY_FIRMWARE, "");
                if (!settingsLastFirmware.equals(nowFirmware)) {
                    settingsLastFirmware = nowFirmware;
                    drawUpdateControl(UPDATE_FIRMWARE);
                }
                drawUpdateControl(UPDATE_APP);
            }
        }
    }
    private String currentTransportLabel() {
        String transport = prefs == null ? "" : prefs.getString(WeaveService.KEY_TRANSPORT, "");
        return transport == null || transport.trim().isEmpty() ? "Auto" : transport;
    }

    private Boolean lastHeaderConnected;
    private void updateFlipperHeader(boolean connected) {
        if (chip == null || (lastHeaderConnected != null && lastHeaderConnected == connected)) return;
        lastHeaderConnected = connected;
        String label = "Flipper O";
        SpannableString text = new SpannableString(label);
        text.setSpan(new ForegroundColorSpan(WHITE), 0, label.length() - 1, Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        text.setSpan(new ForegroundColorSpan(connected ? ORANGE : 0xffe55353), label.length() - 1, label.length(), Spanned.SPAN_EXCLUSIVE_EXCLUSIVE);
        chip.setText(text);
    }

    private void toast(String message) { Toast.makeText(this, message, Toast.LENGTH_SHORT).show(); }
    @Override public void onBackPressed() {
        if (selectedPage == APPS && appDetailEntry != null) closeAppDetail();
        else if (selectedPage == SETTINGS && settingsInfoOpen) showSettingsMain();
        else super.onBackPressed();
    }

    /** A single keyboard modal for every stock Flipper TextInput, on any page
     * within Weave. The original Flipper input remains the source of truth. */
    private void showFlipperKeyboardIfActive() {
        // The bridge is always driven by a verified BLE request. A simultaneous
        // USB link must not suppress its modal just because USB has UI priority.
        if(!appForeground || !flipperIsConnected()) return;
        String token = prefs.getString(WeaveService.KEY_TEXT_TOKEN, "");
        if(token.isEmpty() || token.equals(flipperKeyboardIgnoredToken)) return;
        if(flipperKeyboardDialog != null && flipperKeyboardDialog.isShowing()) {
            if(token.equals(flipperKeyboardToken)) return;
            flipperKeyboardDialog.dismiss();
        }
        String header = prefs.getString(WeaveService.KEY_TEXT_HEADER, "Flipper input");
        int max = Math.max(1, Math.min(256, prefs.getInt(WeaveService.KEY_TEXT_LIMIT, 32)));
        EditText input = new EditText(this);
        input.setSingleLine(true);
        input.setHint("Type on your phone (ASCII)");
        input.setTextColor(WHITE);
        input.setHintTextColor(MUTED);
        input.setFilters(new android.text.InputFilter[]{
                new android.text.InputFilter.LengthFilter(max)});
        LinearLayout content = new LinearLayout(this);
        content.setPadding(dp(20), dp(5), dp(20), dp(5));
        content.addView(input, new LinearLayout.LayoutParams(-1, -2));
        flipperKeyboardToken = token;
        // 550ms debounce: responsive editing without one BLE/SD write per key.
        input.addTextChangedListener(new android.text.TextWatcher() {
            @Override public void beforeTextChanged(CharSequence s, int start, int count, int after) { }
            @Override public void onTextChanged(CharSequence s, int start, int before, int count) {
                if(flipperKeyboardPreviewTask != null)
                    handler.removeCallbacks(flipperKeyboardPreviewTask);
                final String value = s.toString();
                for(int i = 0; i < value.length(); i++) {
                    char c = value.charAt(i);
                    if(c < 32 || c > 126) return;
                }
                flipperKeyboardPreviewTask = () -> {
                    if(!token.equals(flipperKeyboardToken) ||
                            flipperKeyboardDialog == null ||
                            !flipperKeyboardDialog.isShowing()) return;
                    startService(new Intent(MainActivity.this, WeaveService.class)
                            .setAction(WeaveService.ACTION_TEXT_INPUT_PREVIEW)
                            .putExtra("token", token).putExtra("text", value));
                };
                handler.postDelayed(flipperKeyboardPreviewTask, 550);
            }
            @Override public void afterTextChanged(android.text.Editable s) { }
        });
        flipperKeyboardDialog = new AlertDialog.Builder(this)
                .setTitle(header.isEmpty() ? "Flipper keyboard" : header)
                .setMessage("Text appears on the Flipper as you type. Confirm with OK on the Flipper.")
                .setView(content)
                .setNegativeButton("Cancel", (dialog, which) -> {
                    flipperKeyboardIgnoredToken = token;
                })
                .setPositiveButton("Send", null)
                .create();
        AlertDialog dialog = flipperKeyboardDialog;
        dialog.setOnShowListener(ignored -> dialog.getButton(AlertDialog.BUTTON_POSITIVE)
                .setOnClickListener(v -> {
                    String typed = input.getText().toString();
                    for(int i = 0; i < typed.length(); i++) {
                        char c = typed.charAt(i);
                        if(c < 32 || c > 126) {
                            toast("Flipper keyboard accepts ASCII text only");
                            return;
                        }
                    }
                    flipperKeyboardIgnoredToken = token;
                    if(flipperKeyboardPreviewTask != null)
                        handler.removeCallbacks(flipperKeyboardPreviewTask);
                    flipperKeyboardPreviewTask = null;
                    startService(new Intent(this, WeaveService.class)
                            .setAction(WeaveService.ACTION_TEXT_INPUT_SEND)
                            .putExtra("token", token).putExtra("text", typed));
                    dialog.dismiss();
                }));
        dialog.setOnDismissListener(ignored -> {
            if(flipperKeyboardPreviewTask != null)
                handler.removeCallbacks(flipperKeyboardPreviewTask);
            flipperKeyboardPreviewTask = null;
            flipperKeyboardIgnoredToken = token;
            if(flipperKeyboardDialog == dialog) {
                flipperKeyboardDialog = null;
                flipperKeyboardToken = null;
            }
        });
        dialog.show();
        dialog.getWindow().setSoftInputMode(android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
        input.requestFocus();
    }

    private void showFindDeviceDialogIfRinging() {
        if(!prefs.getBoolean(WeaveService.KEY_FIND_RINGING, false)) return;
        if(findDeviceDialog != null && findDeviceDialog.isShowing()) return;
        findDeviceDialog = new AlertDialog.Builder(this)
                .setTitle("Find my Device")
                .setMessage("Your device is ringing and vibrating.")
                // Any dismissal (button, BACK, or outside) stops the alert exactly once.
                .setPositiveButton("Stop ringing", null)
                .create();
        findDeviceDialog.setOnDismissListener(dialog -> {
            findDeviceDialog = null;
            if(prefs.getBoolean(WeaveService.KEY_FIND_RINGING, false)) stopFindDeviceRing();
        });
        findDeviceDialog.setCanceledOnTouchOutside(true);
        findDeviceDialog.show();
    }
    private void stopFindDeviceRing() {
        startService(new Intent(this, WeaveService.class)
                .setAction(WeaveService.ACTION_STOP_FIND_DEVICE));
    }
    private void consumeFindDeviceIntent() {
        Intent intent = getIntent();
        if(intent != null && intent.getBooleanExtra("show_find_device", false)) {
            intent.removeExtra("show_find_device");
            handler.post(this::showFindDeviceDialogIfRinging);
        }
    }
    @Override protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        consumeFindDeviceIntent();
    }
    @Override protected void onResume() {
        super.onResume();
        appForeground = true;
        if (apkReadyToInstall) handler.post(this::openApkInstaller);
        consumeFindDeviceIntent();
        // If the alert began while Weave was behind another app, opening Weave
        // (normally from either notification action) must show the same modal.
        // This also covers any missed broadcasts and Activity recreation.
        showFindDeviceDialogIfRinging();
        showFlipperKeyboardIfActive();
        // Existing installs may have Weave enabled without the new Nearby devices
        // grant; ask once per visible Activity instance, never in a loop.
        if (prefs.getBoolean(WeaveService.KEY_ENABLED, false) && !hasBlePermission()
                && !blePermissionPromptedThisSession) requestBlePermission();
        // Foreground services can be stopped by the OS/vendor. Re-establish on
        // foreground return only when the user explicitly enabled Weave.
        if (prefs.getBoolean(WeaveService.KEY_ENABLED, false) && hasBlePermission()
                && !prefs.getBoolean(WeaveService.KEY_ACTIVE, false)
                && System.currentTimeMillis() - lastServiceRestart > 10000) {
            lastServiceRestart = System.currentTimeMillis();
            startEnabledService();
        }
        updateScreenSubscription();
        if (selectedPage == APPS) {
            if (appsRefreshInProgress && appsRefreshAnimator == null)
                startAppsRefreshIconOnly();
            if (appsMode == 0 && catalogSearchTerm.length() >= 2 &&
                    catalogSearchResults.isEmpty()) {
                handler.removeCallbacks(delayedCatalogSearch);
                handler.postDelayed(delayedCatalogSearch, 450);
            }
        } else if (selectedPage == FILES && !filesSearchTerm.isEmpty() && filesGlobalSearch == null) {
            handler.removeCallbacks(delayedFilesSearch);
            handler.postDelayed(delayedFilesSearch, 200);
        }
        // Every visible tab receives one immediate state reconciliation on return.
        refresh();
        handler.removeCallbacks(refresher);
        handler.postDelayed(refresher, PageWorkPolicy.refreshDelayMillis(
                selectedPage, appsRefreshInProgress || activeBadgeJob() != null));
    }
    @Override protected void onPause() {
        appForeground = false;
        screenAwaitingFreshFrame = true;
        homeScreenRefreshRequestedAt = 0;
        for (Runnable release : releaseGamepadOnPause) release.run();
        updateScreenSubscription(); // Stop only if Home was actually streaming.
        handler.removeCallbacks(refresher);
        // Background activity must not keep animated views or ad-hoc searches
        // active. Never cancel the foreground service's ongoing transfers.
        handler.removeCallbacks(delayedAppsFilter);
        handler.removeCallbacks(delayedCatalogSearch);
        ++catalogSearchGeneration;
        if (appCatalogClient != null) appCatalogClient.cancelSearch();
        if (appsRefreshAnimator != null) {
            appsRefreshAnimator.cancel();
            appsRefreshAnimator = null;
        }
        if (selectedPage == FILES) {
            handler.removeCallbacks(delayedFilesSearch);
            handler.removeCallbacks(nextFilesSearchStep);
            handler.removeCallbacks(filesSearchFolderTimeout);
            filesGlobalSearch = null;
            requestedFilesPath = null;
        }
        finishGestureRefresh();
        super.onPause();
    }
    @Override protected void onSaveInstanceState(Bundle state) {
        state.putInt("page", selectedPage);
        if (pendingUpdateApk != null && pendingApkVersion != null) {
            state.putString("apk_update_version", pendingApkVersion);
            state.putInt("apk_update_request", apkUpdateRequest);
            state.putBoolean("apk_update_ready", apkReadyToInstall);
        }
        super.onSaveInstanceState(state);
    }
    @Override protected void onDestroy() {
        if (navigationPillAnimator != null) {
            navigationPillAnimator.cancel();
            navigationPillAnimator = null;
        }
        handler.removeCallbacks(refresher);
        handler.removeCallbacks(delayedShopIconSync);
        handler.removeCallbacks(delayedAppsFilter);
        handler.removeCallbacks(delayedCatalogSearch);
        handler.removeCallbacks(delayedFilesSearch);
        handler.removeCallbacks(nextFilesSearchStep);
        handler.removeCallbacks(filesSearchFolderTimeout);
        stopAppsRefreshAnimation();
        finishGestureRefresh();
        releaseGamepadOnPause.clear();
        if (receiverRegistered) unregisterReceiver(usbPermissions);
        if (flipperEventsRegistered) unregisterReceiver(deviceEvents);
        if (appCatalogClient != null) appCatalogClient.close();
        super.onDestroy();
    }
}
