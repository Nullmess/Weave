package io.github.nullmess.weave;

import android.Manifest;
import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothAdapter;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.media.AudioManager;
import android.media.ToneGenerator;
import android.os.Vibrator;
import android.os.VibrationEffect;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.ArrayDeque;
import java.util.List;
import org.json.JSONArray;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/** User-controlled foreground connection service. Never treats a transport open as a WV1 handshake. */
public final class WeaveService extends Service {
    static final String PREFS = "weave_companion";
    static final String KEY_ENABLED = "enabled";
    static final String KEY_BOOT = "boot";
    static final String KEY_GATEWAY = "gateway";
    static final String KEY_MODE = "mode";
    static final String KEY_ACTIVE = "active";
    static final String KEY_CONNECTED = "connected";
    static final String KEY_WV1 = "wv1";
    static final String KEY_BATTERY = "battery";
    static final String KEY_FIRMWARE = "firmware";
    static final String KEY_DEVICE_NAME = "device_name";
    static final String KEY_STATUS = "status";
    static final String KEY_TRANSPORT = "transport";
    static final String KEY_LOG = "log";
    static final String KEY_REQUESTS = "requests";
    static final String KEY_ERRORS = "errors";
    static final String ACTION_START = "io.github.nullmess.weave.START";
    static final String ACTION_USB = "io.github.nullmess.weave.USB";
    private static final String ACTION_USB_PERMISSION = "io.github.nullmess.weave.USB_PERMISSION_SERVICE";
    static final String ACTION_SCAN = "io.github.nullmess.weave.SCAN";
    static final String ACTION_CONNECT_BLE = "io.github.nullmess.weave.CONNECT_BLE";
    static final String ACTION_DISCONNECT = "io.github.nullmess.weave.DISCONNECT";
    static final String ACTION_REFRESH_APPS = "io.github.nullmess.weave.REFRESH_APPS";
    static final String ACTION_LAUNCH_APP = "io.github.nullmess.weave.LAUNCH_APP";
    static final String ACTION_START_SCREEN = "io.github.nullmess.weave.START_SCREEN";
    static final String ACTION_RECOVER_SCREEN = "io.github.nullmess.weave.RECOVER_SCREEN";
    static final String ACTION_STOP_SCREEN = "io.github.nullmess.weave.STOP_SCREEN";
    static final String ACTION_PRESS_KEY = "io.github.nullmess.weave.PRESS_KEY";
    static final String ACTION_KEY_EVENT = "io.github.nullmess.weave.KEY_EVENT";
    static final String ACTION_ENTER_DFU = "io.github.nullmess.weave.ENTER_DFU";
    static final String ACTION_FIND_FLIPPER = "io.github.nullmess.weave.FIND_FLIPPER";
    static final String ACTION_ALERT_EVENT = "io.github.nullmess.weave.ALERT_EVENT";
    static final String ACTION_TEXT_INPUT_SEND = "io.github.nullmess.weave.TEXT_INPUT_SEND";
    static final String ACTION_TEXT_INPUT_PREVIEW = "io.github.nullmess.weave.TEXT_INPUT_PREVIEW";
    static final String EVENT_TEXT_INPUT = "io.github.nullmess.weave.TEXT_INPUT_STATE";
    static final String KEY_TEXT_TOKEN = "text_input_active_token";
    static final String KEY_TEXT_HEADER = "text_input_active_header";
    static final String KEY_TEXT_LIMIT = "text_input_active_limit";
    static final String ACTION_STOP_FIND_DEVICE = "io.github.nullmess.weave.STOP_FIND_DEVICE";
    static final String KEY_FIND_RINGING = "find_device_ringing";
    static final String ACTION_LONG_BACK = "io.github.nullmess.weave.LONG_BACK";
    static final String ACTION_QUEUE_CATALOG_APP = "io.github.nullmess.weave.QUEUE_CATALOG_APP";
    static final String ACTION_CANCEL_QUEUED_APP = "io.github.nullmess.weave.CANCEL_QUEUED_APP";
    static final String EVENT_CATALOG_QUEUE = "io.github.nullmess.weave.CATALOG_QUEUE";
    static final String KEY_CATALOG_QUEUE = "catalog_install_queue_v1";
    private static final String KEY_CATALOG_CLEANUP = "catalog_install_cleanup_v1";
    static final String KEY_CATALOG_FAILURES = "catalog_install_failures_v1";
    static final String KEY_CATALOG_INSTALLED_VERSIONS = "catalog_verified_installed_versions_v1";
    static final String KEY_CATALOG_ACTIVE_DEVICE = "catalog_current_ble_device_v1";
    static final String ACTION_LIST_FILES = "io.github.nullmess.weave.LIST_FILES";
    static final String ACTION_STORAGE_INFO = "io.github.nullmess.weave.STORAGE_QUERY";
    static final String ACTION_MKDIR = "io.github.nullmess.weave.MKDIR";
    static final String ACTION_RENAME = "io.github.nullmess.weave.RENAME";
    static final String ACTION_DELETE_FILE = "io.github.nullmess.weave.DELETE_FILE";
    static final String ACTION_DELETE_BATCH = "io.github.nullmess.weave.DELETE_BATCH";
    static final String ACTION_UPLOAD_FAP = "io.github.nullmess.weave.UPLOAD_FAP";
    static final String ACTION_SYNC_SHOP = "io.github.nullmess.weave.SYNC_SHOP";
    static final String ACTION_READ_FILE = "io.github.nullmess.weave.READ_FILE";
    static final String ACTION_UPLOAD_FILE = "io.github.nullmess.weave.UPLOAD_FILE";
    static final String EVENT_FILE_DATA = "io.github.nullmess.weave.FILE_CONTENT";
    static final String ACTION_CLI_START = "io.github.nullmess.weave.CLI_START";
    static final String ACTION_CLI_SEND = "io.github.nullmess.weave.CLI_SEND";
    static final String ACTION_CLI_STOP = "io.github.nullmess.weave.CLI_STOP";
    static final String EVENT_CLI = "io.github.nullmess.weave.CLI_DATA";
    static final String EVENT_SCREEN = "io.github.nullmess.weave.SCREEN_FRAME";
    static final String EVENT_FILES = "io.github.nullmess.weave.FILES_REPLY";
    static final String EVENT_STORAGE_INFO = "io.github.nullmess.weave.STORAGE_INFO";
    static final String KEY_STORAGE_INT_TOTAL = "storage_int_total";
    static final String KEY_STORAGE_INT_FREE = "storage_int_free";
    static final String KEY_STORAGE_EXT_TOTAL = "storage_ext_total";
    static final String KEY_STORAGE_EXT_FREE = "storage_ext_free";
    static final String KEY_STORAGE_INT_ERROR = "storage_int_error";
    static final String KEY_STORAGE_EXT_ERROR = "storage_ext_error";
    static final String EVENT_OPERATION = "io.github.nullmess.weave.OPERATION_REPLY";
    static final String EVENT_PROGRESS = "io.github.nullmess.weave.OPERATION_PROGRESS";
    static final String KEY_INVENTORY = "apps_inventory";
    static final String KEY_INVENTORY_PATHS = "apps_inventory_paths";
    static final String KEY_FIRMWARE_API = "firmware_api";
    private static final String KEY_LAST_API = "last_verified_firmware_api";
    private static final String KEY_LAST_API_DEVICE = "last_verified_firmware_api_device";
    static final String KEY_INVENTORY_STATUS = "apps_inventory_status";
    static final String KEY_FAP_METADATA = "fap_metadata_by_path_v1";
    static final String KEY_FAP_METADATA_DEVICE = "fap_metadata_device_v1";
    static final String EVENT_FAP_METADATA = "io.github.nullmess.weave.FAP_METADATA";
    private static final String FAP_INDEX_REQ = "/ext/.weave/fap-index.req";
    private static final String FAP_INDEX_RESULT = "/ext/.weave/fap-index.txt";
    private static final String CHANNEL_ID = "weave_link";
    private static final int NOTIFICATION_ID = 418;
    private static final int FIND_NOTIFICATION_ID = 419;
    private static final String FIND_CHANNEL_ID = "weave_finder";
    static final String EVENT_SHOP_PROGRESS = "io.github.nullmess.weave.SHOP_PROGRESS";
    static final String KEY_SHOP_JOB = "native_shop_job";
    static final String KEY_BLE_DROPS = "ble_link_drops";
    private static final String SHOP_CHANNEL_ID = "weave_app_install";
    private static final int SHOP_NOTIFICATION_ID = 420;

    private SharedPreferences prefs;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private long lastScreenBroadcastAt;
    private byte[] pendingScreenFrame;
    private boolean screenDeliveryScheduled;
    private final Runnable deliverScreenFrame = () -> {
        screenDeliveryScheduled = false;
        byte[] framebuffer = pendingScreenFrame;
        pendingScreenFrame = null;
        if (framebuffer == null || WeaveService.this.stopping ||
                prefs == null || !prefs.getBoolean("screen_requested", false)) return;
        lastScreenBroadcastAt = android.os.SystemClock.uptimeMillis();
        Intent event = new Intent(EVENT_SCREEN).setPackage(getPackageName());
        event.putExtra("pixels", framebuffer);
        sendBroadcast(event);
    };
    private String pendingFindFlipperToken;
    private long findFlipperDeadline;
    private final ExecutorService network = Executors.newSingleThreadExecutor();
    private UsbLink usb;
    private FlipperUsbCli cli;
    private BleLink ble;
    private BluetoothDevice foundBle;
    private volatile boolean usbConfirmed;
    private byte[] pendingInstallBytes;
    private String pendingInstallPath;
    private final ArrayDeque<String> queuedDeletes = new ArrayDeque<>();
    private boolean deletingBatch;
    private volatile boolean bleConfirmed;
    private static final String SHOP_INDEX = "/ext/.weave/shop.catalog";
    private static final String SHOP_REQUEST = "/ext/.weave/shop.request";
    private static final String SHOP_RESULT = "/ext/.weave/shop.result";
    private static final String REMOTE_INSTALL_REQUEST = "/ext/.weave/shop.remote.request";
    private static final String REMOTE_INSTALL_READY = "/ext/.weave/shop.remote.ready";
    private static final String REMOTE_INSTALL_HOME = "/ext/.weave/shop.remote.home";
    private static final String SHOP_LAST_REQUEST = "last_native_shop_request";
    /* One-shot acknowledgments must not become permanent files on the SD.
     * Drain only when the file is no longer needed and no RPC transfer is active.
     * Pending files are overwritten by the next operation if the link drops. */
    private final ArrayDeque<String> shopAckCleanup = new ArrayDeque<>();
    private boolean shopAckCleanupRunning;
    private String shopAckCleanupDevice;
    private void queueShopAckCleanup(String path) {
        if (!REMOTE_INSTALL_READY.equals(path) && !REMOTE_INSTALL_HOME.equals(path)) return;
        String device = ble == null ? null : ble.connectedDeviceKey();
        if (device == null) return;
        if (!device.equals(shopAckCleanupDevice)) {
            shopAckCleanup.clear();
            shopAckCleanupDevice = device;
        }
        if (!shopAckCleanup.contains(path)) shopAckCleanup.addLast(path);
        handler.postDelayed(drainShopAckCleanup, 350);
    }
    private final Runnable drainShopAckCleanup = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (stopping || shopAckCleanup.isEmpty() || shopAckCleanupRunning) return;
            if (ble == null || !shopAckCleanupDevice.equals(ble.connectedDeviceKey())) {
                shopAckCleanup.clear();
                shopAckCleanupDevice = null;
                return;
            }
            if (activeCatalogJob != null || nativeShopHomeToken != null ||
                    shopTransferActive || shopVerifying || shopNetworkBusy ||
                    deletingBatch || catalogCleanupRunning ||
                    !bleConfirmed || ble == null || !ble.isIdleForShop()) {
                handler.postDelayed(this, 700);
                return;
            }
            shopAckCleanupRunning = true;
            ble.delete(shopAckCleanup.peekFirst(), false);
        }
    };
    private AppCatalogClient shopClient;
    private String pendingShopCatalog;
    private String shopCatalogVersion;
    private byte[] pendingShopFap;
    private String pendingShopPath;
    private String shopRequestToken;
    private String shopRequestDevice;
    private String pendingShopStatus;
    private boolean shopTransferActive, shopNetworkBusy;
    private boolean shopVerifying;
    private boolean shopVerificationRequested;
    private String shopStatusInFlight;
    private final Runnable shopVerify = new Runnable() {
        @Override public void run() {
            if(!shopVerifying || stopping || shopVerificationRequested) return;
            if(bleConfirmed && ble != null && ble.isIdleForShop()) {
                shopVerificationRequested = true;
                ble.requestInventory();
            } else handler.postDelayed(this, 650);
        }
    };
    private String shopTitle = "Application";
    private int shopPercent = -1;
    private long shopEta = -1;
    private long shopLastReportTime;
    private long shopPhaseStartTime;
    private String shopPhase = "";
    /* Pendant un transfert, aucune commande GUI ne doit modifier un ecran non diffuse. */
    private boolean remoteInputLocked() {
        return shopNetworkBusy || shopTransferActive || shopVerifying || pendingShopFap != null ||
                !catalogQueue.isEmpty() || activeCatalogJob != null ||
                (shopRequestToken != null && ("downloading".equals(shopPhase) ||
                        "installing".equals(shopPhase)));
    }
    private String shopVerifiedPath;
    // Catalog downloads belong to this foreground service, never an Activity or screen.
    // Persist pending jobs to survive activity recreation and a service process restart.
    private static final int MAX_CATALOG_QUEUE = 32;
    private static final class CatalogJob {
        final String id, category, title, version, archiveId;
        String phase = "queued";
        int percent = -1;
        long etaSeconds = -1;
        boolean cancelRequested;
        CatalogJob(String id, String category, String title, String version, String archiveId) {
            this.id = id; this.category = category; this.title = title;
            this.version = version; this.archiveId = archiveId;
        }
        org.json.JSONObject json() throws org.json.JSONException {
            org.json.JSONObject item = new org.json.JSONObject();
            item.put("id", id); item.put("category", category); item.put("title", title);
            item.put("version", version); item.put("archiveId", archiveId);
            item.put("phase", phase); item.put("percent", percent);
            item.put("eta", etaSeconds);
            return item;
        }
    }
    private final ArrayDeque<CatalogJob> catalogQueue = new ArrayDeque<>();
    private CatalogJob activeCatalogJob;
    private AppCatalogClient.DownloadTicket activeCatalogTicket;
    private byte[] activeCatalogBytes;
    private String activeCatalogPath;
    private String activeCatalogStagePath;
    private String activeCatalogBackupPath;
    private boolean activeCatalogHadExisting;
    // A single on-device updater screen for phone-origin and Flipper-origin jobs.
    private String activeCatalogScreenToken;
    private boolean activeCatalogScreenStarted;
    private boolean activeCatalogScreenRequestSent;
    private boolean activeCatalogDownloadStarted;
    private long activeCatalogScreenDeadline;

    // A missing/busy native viewer must never cancel a valid FAP download.
    private boolean activeCatalogScreenUnavailable;
    private int lastRemoteDownloadPercent = -10;
    private String pendingRemoteInstallFinal;
    private long remoteInstallFinalQueuedAt;
    private long remoteInstallViewerReadyAt;
    private long remoteInstallHomeDeadline;
    private boolean remoteInstallHomePollInFlight;
    private long remoteInstallHomeLastReadAt;
    private String nativeShopHomeToken;
    private String nativeShopHomeOutcome;
    private long nativeShopHomeDeadline;

    private final ArrayDeque<String> catalogPartialCleanup = new ArrayDeque<>();
    private boolean catalogCleanupRunning;
    private String deferredCatalogFilesPath;
    private final Runnable showRemoteDownloadScreen = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (activeCatalogJob == null ||
                    !("opening_screen".equals(activeCatalogJob.phase) ||
                      "downloading".equals(activeCatalogJob.phase)) ||
                    activeCatalogScreenUnavailable || stopping) return;
            if (android.os.SystemClock.elapsedRealtime() > activeCatalogScreenDeadline &&
                    !activeCatalogScreenStarted) {
                finishCatalogJob(false, "Flipper install screen did not respond. " +
                        "Close the running app on Flipper and retry.");
                return;
            }
            if (!bleConfirmed || ble == null) {
                handler.postDelayed(this, 500);
                return;
            }
            if (!activeCatalogScreenRequestSent) {
                if (ble.isIdleForShop() && !shopTransferActive && !shopNetworkBusy &&
                        !bleCliActive) {
                    activeCatalogScreenRequestSent = true;
                    String title = activeCatalogJob.title.replaceAll("[^A-Za-z0-9 _.-]", " ").trim();
                    if (title.isEmpty()) title = activeCatalogJob.id;
                    if (title.length() > 30) title = title.substring(0, 30);
                    String version = activeCatalogJob.version == null ? "0" :
                            activeCatalogJob.version.replaceAll("[^A-Za-z0-9_.-]", "");
                    if (version.isEmpty()) version = "0";
                    if (version.length() > 30) version = version.substring(0, 30);
                    String init = activeCatalogScreenToken + "|" + activeCatalogJob.id +
                            "|" + title + "|" + version + "|0|-";
                    ble.uploadFile(REMOTE_INSTALL_REQUEST, init.getBytes(
                            java.nio.charset.StandardCharsets.US_ASCII), "Remote Install Prepare");
                } else handler.postDelayed(this, 550);
            } else if (activeCatalogScreenStarted &&
                    "downloading".equals(activeCatalogJob.phase) && ble.isIdleForShop() &&
                    activeCatalogJob.percent >= 0 &&
                    (activeCatalogJob.percent - lastRemoteDownloadPercent >= 10 ||
                     lastRemoteDownloadPercent < 0)) {
                lastRemoteDownloadPercent = activeCatalogJob.percent;
                String status = activeCatalogScreenToken + "|downloading|" +
                        activeCatalogJob.percent + "|" +
                        Math.max(0, activeCatalogJob.etaSeconds) + "|-|-";
                ble.uploadFile(SHOP_RESULT, status.getBytes(
                        java.nio.charset.StandardCharsets.US_ASCII),
                        "Remote Install Download Progress");
            } else handler.postDelayed(this, 900);
        }
    };
    /** Poll the Flipper's native-view acknowledgment, not the App.Start RPC result. */
    private final Runnable checkRemoteInstallReady = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (activeCatalogJob == null || stopping || activeCatalogScreenStarted ||
                    !activeCatalogScreenRequestSent) return;
            if (android.os.SystemClock.elapsedRealtime() > activeCatalogScreenDeadline) {
                finishCatalogJob(false, "Flipper did not open the native install screen. " +
                        "Close the running app on Flipper and install matching Weave firmware.");
                return;
            }
            if (bleConfirmed && ble != null && ble.isIdleForShop()) {
                ble.readFile(REMOTE_INSTALL_READY);
            } else handler.postDelayed(this, 450);
        }
    };
    private void onRemoteInstallReady(byte[] bytes) {
        if (activeCatalogJob == null || !activeCatalogScreenRequestSent ||
                activeCatalogScreenStarted || activeCatalogScreenToken == null) return;
        String ack = bytes == null ? "" :
                new String(bytes, java.nio.charset.StandardCharsets.US_ASCII).trim();
        if (ack.equals(activeCatalogScreenToken)) {
            activeCatalogScreenStarted = true;
            activeCatalogScreenUnavailable = false;
            queueShopAckCleanup(REMOTE_INSTALL_READY);
            beginActiveCatalogDownload();
        } else handler.postDelayed(checkRemoteInstallReady, 450);
    }

    /* The upload and inventory can already be complete while the Flipper is
     * still rendering its 0..100% native animation. Keep Android's horizontal
     * bar indeterminate until a TOKEN-MATCHED close marker is received. */
    private final Runnable checkRemoteInstallHome = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (stopping || activeCatalogJob == null ||
                    !"awaiting_home".equals(activeCatalogJob.phase) ||
                    pendingRemoteInstallFinal != null) return;
            long now = android.os.SystemClock.elapsedRealtime();
            if (remoteInstallHomePollInFlight) {
                if (now - remoteInstallHomeLastReadAt < 2400L) {
                    handler.postDelayed(this, 350);
                    return;
                }
                // Recover from a lost or disconnected read callback.
                remoteInstallHomePollInFlight = false;
            }
            if (now > remoteInstallHomeDeadline) {
                // The FAP was verified, but we cannot claim the native viewer closed.
                activeCatalogScreenStarted = false;
                activeCatalogScreenToken = null;
                finishCatalogJob(false, "FAP verified but Flipper home was not confirmed");
                return;
            }
            if (bleConfirmed && ble != null && ble.isIdleForShop()) {
                remoteInstallHomePollInFlight = true;
                remoteInstallHomeLastReadAt = now;
                ble.readFile(REMOTE_INSTALL_HOME);
                handler.postDelayed(this, 2400);
            } else handler.postDelayed(this, 350);
        }
    };
    private void onRemoteInstallHome(byte[] bytes) {
        remoteInstallHomePollInFlight = false;
        if (activeCatalogJob == null ||
                !"awaiting_home".equals(activeCatalogJob.phase) ||
                activeCatalogScreenToken == null) return;
        String token = bytes == null ? "" :
                new String(bytes, java.nio.charset.StandardCharsets.US_ASCII).trim();
        if (token.equals(activeCatalogScreenToken)) {
            // Token was emitted AFTER the GUI event loop finished, not when
            // the upload RPC or inventory completed. The desktop is resuming.
            pendingRemoteInstallFinal = null;
            activeCatalogScreenStarted = false;
            activeCatalogScreenToken = null;
            remoteInstallViewerReadyAt = android.os.SystemClock.elapsedRealtime() + 400L;
            queueShopAckCleanup(REMOTE_INSTALL_HOME);
            finishCatalogJob(true, null);
        } else handler.postDelayed(checkRemoteInstallHome, 450);
    }

    /* Flipper-originated Shop installs use the same 100%/home handshake.
     * Do not mark Android done just because RPC verified the saved file. */
    private final Runnable checkNativeShopHome = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (stopping || nativeShopHomeToken == null) return;
            if (android.os.SystemClock.elapsedRealtime() > nativeShopHomeDeadline) {
                setStatus("Native Shop: Flipper did not confirm leaving the install screen");
                final boolean failed = "failed".equals(nativeShopHomeOutcome);
                nativeShopHomeToken = null;
                nativeShopHomeOutcome = null;
                shopPhase = failed ? "failed" : "unconfirmed";
                shopPercent = -1;
                try {
                    org.json.JSONObject job = new org.json.JSONObject(
                            prefs.getString(KEY_SHOP_JOB, "{}"));
                    job.put("phase", failed ? "failed" : "unconfirmed");
                    job.put("percent", -1);
                    job.put("updatedAt", System.currentTimeMillis());
                    prefs.edit().putString(KEY_SHOP_JOB, job.toString()).apply();
                } catch (org.json.JSONException ignored) { }
                sendBroadcast(new Intent(EVENT_SHOP_PROGRESS).setPackage(getPackageName())
                        .putExtra("phase", failed ? "failed" : "unconfirmed")
                        .putExtra("percent", -1));
                // The installation's result is not proof that the native UI
                // closed; do not silently launch another GUI over a stuck view.
                return;
            }
            if (bleConfirmed && ble != null && ble.isIdleForShop()) {
                ble.readFile(REMOTE_INSTALL_HOME);
                handler.postDelayed(this, 2200);
            } else handler.postDelayed(this, 650);
        }
    };
    private void onNativeShopHome(byte[] bytes) {
        if (nativeShopHomeToken == null) return;
        String token = bytes == null ? "" :
                new String(bytes, java.nio.charset.StandardCharsets.US_ASCII).trim();
        if (nativeShopHomeToken.equals(token)) {
            final String completedToken = nativeShopHomeToken;
            final boolean failed = "failed".equals(nativeShopHomeOutcome);
            nativeShopHomeToken = null;
            nativeShopHomeOutcome = null;
            handler.removeCallbacks(checkNativeShopHome);
            shopPhase = failed ? "failed" : "done";
            shopPercent = failed ? -1 : 100;
            shopEta = 0;
            final String id = completedToken.substring(0, completedToken.indexOf('|'));
            if (failed) {
                // Do not retain a terminal failure as a permanent banner on
                // the app details. The actual cause remains in the diagnostics log.
                prefs.edit().remove(KEY_SHOP_JOB).apply();
            } else try {
                org.json.JSONObject job = new org.json.JSONObject(
                        prefs.getString(KEY_SHOP_JOB, "{}"));
                job.put("phase", "done");
                job.put("percent", 100);
                job.put("eta", 0);
                job.put("updatedAt", System.currentTimeMillis());
                prefs.edit().putString(KEY_SHOP_JOB, job.toString()).apply();
            } catch (org.json.JSONException ignored) { }
            sendBroadcast(new Intent(EVENT_SHOP_PROGRESS).setPackage(getPackageName())
                    .putExtra("id", id).putExtra("phase", shopPhase)
                    .putExtra("percent", shopPercent).putExtra("eta", 0));
            showShopNotification(shopPhase, shopPercent, 0);
            shopRequestToken = null;
            shopRequestDevice = null;
            queueShopAckCleanup(REMOTE_INSTALL_HOME);
            handler.post(drainCatalogQueue);
        } else handler.postDelayed(checkNativeShopHome, 650);
    }

    private final Runnable drainCatalogQueue = this::drainCatalogQueueNow;
    private final Runnable catalogVerifyTimeout = () -> {
        if (activeCatalogJob != null && "verifying".equals(activeCatalogJob.phase))
            finishCatalogJob(false, "Installed file was not verified by Flipper inventory");
    };

    private static String catalogKey(String alias) {
        return alias == null ? "" : alias.toLowerCase(Locale.ROOT).replaceAll("[^a-z0-9]", "");
    }
    private void restoreCatalogQueue() {
        try {
            JSONArray saved = new JSONArray(prefs.getString(KEY_CATALOG_QUEUE, "[]"));
            for (int i = 0; i < saved.length() && catalogQueue.size() < MAX_CATALOG_QUEUE; i++) {
                org.json.JSONObject item = saved.optJSONObject(i);
                if (item == null) continue;
                String id = item.optString("id");
                String category = item.optString("category", "Tools");
                if (!id.matches("[A-Za-z0-9_-]{2,100}") || !category.matches("[A-Za-z0-9_-]{2,30}")) continue;
                CatalogJob job = new CatalogJob(id, category, item.optString("title", id),
                        item.optString("version", ""), item.optString("archiveId", ""));
                // A dead process cannot resume a BLE stream: restart the job from the
                // original verified catalog binary, never assume the upload succeeded.
                catalogQueue.addLast(job);
            }
        } catch (org.json.JSONException ignored) { }
        persistCatalogQueue("restored", null);
    }
    private void persistCatalogQueue(String change, String id) {
        JSONArray snapshot = new JSONArray();
        for (CatalogJob job : catalogQueue) {
            try { snapshot.put(job.json()); } catch (org.json.JSONException ignored) { }
        }
        // Persist structural transitions synchronously so a fresh Activity sees them.
        // Progress updates can be asynchronous: frequent disk commits cause UI stutter.
        SharedPreferences.Editor editor = prefs.edit().putString(KEY_CATALOG_QUEUE, snapshot.toString());
        if ("progress".equals(change)) editor.apply();
        else editor.commit();
        Intent update = new Intent(EVENT_CATALOG_QUEUE).setPackage(getPackageName())
                .putExtra("change", change);
        if (id != null) update.putExtra("id", id);
        sendBroadcast(update);
    }
    private boolean queuedCatalogId(String id) {
        String key = catalogKey(id);
        for (CatalogJob job : catalogQueue) if (catalogKey(job.id).equals(key)) return true;
        return false;
    }
    private void enqueueCatalogApp(Intent intent) {
        if (intent == null) return;
        String id = intent.getStringExtra("id");
        String category = intent.getStringExtra("category");
        String title = intent.getStringExtra("title");
        String version = intent.getStringExtra("version");
        String archiveId = intent.getStringExtra("archiveId");
        if (id == null || !id.matches("[A-Za-z0-9_-]{2,100}")) return;
        category = shopCanonicalCategory(category);
        if (title == null || title.trim().isEmpty() || title.length() > 150) title = id;
        if (version == null || version.length() > 50) version = "";
        if (archiveId == null || !archiveId.matches("[a-fA-F0-9]{24}")) archiveId = "";
        if (queuedCatalogId(id)) { persistCatalogQueue("duplicate", id); return; }
        if (catalogQueue.size() >= MAX_CATALOG_QUEUE) {
            catalogFailure(id, "Installation queue is full"); return;
        }
        catalogClearFailure(id);
        catalogQueue.addLast(new CatalogJob(id, category, title, version, archiveId));
        persistCatalogQueue("queued", id);
        handler.post(drainCatalogQueue);
    }
    // L'upload utilise un fichier staging, sans ecraser la FAP installée.
    private void cancelQueuedCatalogApp(String id) {
        if (id == null) return;
        if (activeCatalogJob != null && catalogKey(activeCatalogJob.id).equals(catalogKey(id))) {
            CatalogJob job = activeCatalogJob;
            job.cancelRequested = true;
            if (activeCatalogTicket != null) activeCatalogTicket.cancel();
            if ("uploading".equals(job.phase) && ble != null && ble.cancelCatalogUpload()) {
                job.phase = "cancelling";
                persistCatalogQueue("cancelling", job.id);
                return;
            }
            if ("committing".equals(job.phase) || "restoring".equals(job.phase) ||
                    "verifying".equals(job.phase) || "awaiting_home".equals(job.phase)) {
                job.cancelRequested = false;
                setStatus("Installation already finalizing on Flipper");
                return;
            }
            if ("backing_up".equals(job.phase)) {
                // Attendre le Rename en cours, puis restaurer la FAP d'origine.
                persistCatalogQueue("cancelling", job.id);
                return;
            }
            finishCatalogJob(false, "Cancelled");
            return;
        }
        for (java.util.Iterator<CatalogJob> iterator = catalogQueue.iterator(); iterator.hasNext();) {
            CatalogJob job = iterator.next();
            if (catalogKey(job.id).equals(catalogKey(id))) {
                iterator.remove();
                persistCatalogQueue("cancelled", job.id);
                handler.post(drainCatalogQueue);
                return;
            }
        }
    }

    private void persistCatalogCleanup() {
        JSONArray pending = new JSONArray();
        for (String path : catalogPartialCleanup) pending.put(path);
        prefs.edit().putString(KEY_CATALOG_CLEANUP, pending.toString()).apply();
    }
    private void restoreCatalogCleanup() {
        try {
            JSONArray pending = new JSONArray(prefs.getString(KEY_CATALOG_CLEANUP, "[]"));
            for (int i = 0; i < pending.length() && i < 64; i++) {
                String path = pending.optString(i, null);
                if (validCatalogTemporaryPath(path) && !catalogPartialCleanup.contains(path))
                    catalogPartialCleanup.addLast(path);
            }
        } catch (org.json.JSONException ignored) { }
    }
    private static boolean validCatalogTemporaryPath(String path) {
        return path != null && path.matches(
                "/ext/apps/(?:[A-Za-z0-9_. -]+/)*[A-Za-z0-9_. -]+\\.fap\\.(?:weave-part|weave-old)")
                && !path.contains("..");
    }
    private void scheduleCatalogPartialCleanup(String path) {
        if (!validCatalogTemporaryPath(path)) return;
        if (!catalogPartialCleanup.contains(path)) catalogPartialCleanup.addLast(path);
        persistCatalogCleanup();
        handler.postDelayed(drainCatalogQueue, 300);
    }
    private void catalogClearFailure(String id) {
        try {
            org.json.JSONObject failures = new org.json.JSONObject(prefs.getString(KEY_CATALOG_FAILURES, "{}"));
            failures.remove(catalogKey(id));
            prefs.edit().putString(KEY_CATALOG_FAILURES, failures.toString()).apply();
        } catch (org.json.JSONException ignored) { }
    }
    private void catalogFailure(String id, String message) {
        try {
            org.json.JSONObject failures = new org.json.JSONObject(prefs.getString(KEY_CATALOG_FAILURES, "{}"));
            failures.put(catalogKey(id), message == null ? "Download failed" : message);
            prefs.edit().putString(KEY_CATALOG_FAILURES, failures.toString()).commit();
        } catch (org.json.JSONException ignored) { }
        persistCatalogQueue("failed", id);
    }
    private void drainCatalogQueueNow() {
        handler.removeCallbacks(drainCatalogQueue);
        if (stopping) return;
        // A phone-queued app must never steal the local native Shop view while
        // that Shop is downloading, uploading, verifying, or animating home.
        if (nativeShopHomeToken != null || shopNetworkBusy || shopTransferActive ||
                shopVerifying || (shopRequestToken != null &&
                ("downloading".equals(shopPhase) || "installing".equals(shopPhase))) ) {
            if (!catalogQueue.isEmpty()) handler.postDelayed(drainCatalogQueue, 650);
            return;
        }
        if (activeCatalogJob != null &&
                !"awaiting_home".equals(activeCatalogJob.phase)) return;
        // Several callbacks may try to drain the queue. A hard cooldown (not
        // just one delayed post) protects the on-device 100% animation.
        long nativeAnimationRemaining = remoteInstallViewerReadyAt -
                android.os.SystemClock.elapsedRealtime();
        if (nativeAnimationRemaining > 0) {
            handler.postDelayed(drainCatalogQueue, nativeAnimationRemaining);
            return;
        }
        // Finish the native screen before allowing a second app to take it over.
        if (pendingRemoteInstallFinal != null) {
            if (bleConfirmed && ble != null && ble.isIdleForShop()) {
                ble.uploadFile(SHOP_RESULT, pendingRemoteInstallFinal.getBytes(
                        java.nio.charset.StandardCharsets.US_ASCII), "Remote Install Final");
                return;
            }
            if (android.os.SystemClock.elapsedRealtime() - remoteInstallFinalQueuedAt > 30000L) {
                pendingRemoteInstallFinal = null;
                if (activeCatalogJob != null &&
                        "awaiting_home".equals(activeCatalogJob.phase)) {
                    activeCatalogScreenStarted = false;
                    activeCatalogScreenToken = null;
                    finishCatalogJob(false, "Unable to notify native Flipper install screen");
                    return;
                }
            } else {
                handler.postDelayed(drainCatalogQueue, 450);
                return;
            }
        }
        if (activeCatalogJob != null) {
            handler.post(checkRemoteInstallHome);
            return;
        }
        // Remove the previous session's one-shot acknowledgments BEFORE
        // starting another install, so cleanup cannot erase a new ACK.
        if (shopAckCleanupRunning || !shopAckCleanup.isEmpty()) {
            handler.post(drainShopAckCleanup);
            if (!catalogQueue.isEmpty()) handler.postDelayed(drainCatalogQueue, 500);
            return;
        }
        if (!catalogPartialCleanup.isEmpty()) {
            if (!catalogCleanupRunning && bleConfirmed && ble != null && ble.isIdleForShop()) {
                catalogCleanupRunning = true;
                ble.delete(catalogPartialCleanup.peekFirst(), false);
            } else if (!catalogCleanupRunning && catalogQueue.isEmpty())
                handler.postDelayed(drainCatalogQueue, 1000);
            // Un cleanup SD en attente ne doit jamais bloquer un nouveau download HTTP.
        }
        if (catalogQueue.isEmpty()) return;
        // Do not start an 18s native-screen deadline while the device is offline.
        if (!bleConfirmed || ble == null) {
            handler.postDelayed(drainCatalogQueue, 900);
            return;
        }
        // Start the phone HTTP download only after a real native-view ACK.
        activeCatalogJob = catalogQueue.peekFirst();
        activeCatalogScreenToken = "phone-" + System.currentTimeMillis() + "-" +
                Long.toHexString(android.os.SystemClock.elapsedRealtime());
        activeCatalogScreenStarted = false;
        activeCatalogScreenRequestSent = false;
        activeCatalogScreenUnavailable = false;
        activeCatalogDownloadStarted = false;
        activeCatalogScreenDeadline = android.os.SystemClock.elapsedRealtime() + 18000L;
        lastRemoteDownloadPercent = -10;
        activeCatalogJob.phase = "opening_screen";
        activeCatalogJob.percent = -1;
        persistCatalogQueue("opening_screen", activeCatalogJob.id);
        handler.post(showRemoteDownloadScreen);
    }
    private void beginActiveCatalogDownload() {
        if (activeCatalogJob == null || stopping || activeCatalogDownloadStarted ||
                !activeCatalogScreenStarted) return;
        activeCatalogDownloadStarted = true;
        activeCatalogJob.phase = "downloading";
        persistCatalogQueue("started", activeCatalogJob.id);
        handler.post(showRemoteDownloadScreen);
        if (shopClient == null) shopClient = new AppCatalogClient(this);
        final CatalogJob expected = activeCatalogJob;
        AppCatalogClient.Entry app = new AppCatalogClient.Entry(expected.id, expected.title,
                expected.category, expected.version, null, null, null, null, null,
                null, java.util.Collections.emptyList(), 0, 0,
                expected.archiveId.isEmpty() ? null : expected.archiveId);
        activeCatalogTicket = new AppCatalogClient.DownloadTicket();
        final AppCatalogClient.DownloadTicket ticket = activeCatalogTicket;
        final long startedAt = android.os.SystemClock.elapsedRealtime();
        shopClient.downloadAvailableOfficialFap(app, prefs.getString(KEY_FIRMWARE_API, null),
                (bytesRead, totalBytes) -> handler.post(() -> {
                    if (expected != activeCatalogJob || ticket.cancelled || totalBytes <= 0) return;
                    int pct = (int) Math.min(99, bytesRead * 100 / totalBytes);
                    if (pct > expected.percent) {
                        expected.percent = pct;
                        long elapsed = android.os.SystemClock.elapsedRealtime() - startedAt;
                        expected.etaSeconds = pct > 1 && elapsed > 0 ?
                                Math.max(0, (100L - pct) * elapsed / (pct * 1000L)) : -1;
                        persistCatalogQueue("progress", expected.id);
                        handler.post(showRemoteDownloadScreen);
                    }
                }), ticket, (data, publishedApi, compatible, error) -> {
            if (expected != activeCatalogJob || stopping) return;
            if (data == null || data.length < 33 || data.length > 2_000_000 ||
                    (data[0] & 255) != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
                finishCatalogJob(false, error == null ? "No valid official FAP build available" : error);
                return;
            }
            // The download phase reaches 100% on Android while the real Flipper
            // keeps showing its infinite spinner. The installer phase begins next.
            expected.percent = 100;
            persistCatalogQueue("progress", expected.id);
            handler.postDelayed(() -> {
                if (expected != activeCatalogJob || stopping || expected.cancelRequested) return;
                activeCatalogBytes = data;
                activeCatalogTicket = null;
                String verified = null;
                try {
                    org.json.JSONObject paths = new org.json.JSONObject(prefs.getString(KEY_INVENTORY_PATHS, "{}"));
                    verified = paths.optString(catalogKey(expected.id), null);
                    if (verified == null || verified.isEmpty())
                        verified = paths.optString(catalogKey(expected.title), null);
                } catch (org.json.JSONException ignored) { }
                // Une mise a jour conserve le chemin existant, avec un staging distinct.
                activeCatalogHadExisting = verified != null &&
                        verified.matches("/ext/apps/(?:[A-Za-z0-9_. -]+/)*[A-Za-z0-9_. -]+\\.fap") &&
                        !verified.contains("..");
                if (activeCatalogHadExisting) activeCatalogPath = verified;
                else activeCatalogPath = "/ext/apps/" + shopCanonicalCategory(expected.category) +
                        "/" + expected.id + ".fap";
                activeCatalogStagePath = activeCatalogPath + ".weave-part";
                activeCatalogBackupPath = activeCatalogPath + ".weave-old";
                expected.phase = "waiting";
                // Preserve the completed HTTP 100% in the job history;
                // Android renders an indeterminate bar until native home ACK.
                expected.percent = 100;
                persistCatalogQueue("waiting", expected.id);
                handler.post(attemptCatalogUpload);
            }, 240);
        });
    }
    private final Runnable attemptCatalogUpload = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (activeCatalogJob == null || activeCatalogBytes == null || stopping) return;
            if (!catalogPartialCleanup.isEmpty() || catalogCleanupRunning) {
                if (!catalogCleanupRunning && bleConfirmed && ble != null && ble.isIdleForShop()) {
                    catalogCleanupRunning = true;
                    ble.delete(catalogPartialCleanup.peekFirst(), false);
                }
                handler.postDelayed(this, 850);
                return;
            }
            if (!bleConfirmed || ble == null || !ble.isIdleForShop() ||
                    !"ready".equals(prefs.getString(KEY_INVENTORY_STATUS, "")) ||
                    pendingInstallPath != null || shopTransferActive || shopNetworkBusy ||
                    bleCliActive || deletingBatch) {
                if (bleConfirmed && ble != null && !"loading".equals(
                        prefs.getString(KEY_INVENTORY_STATUS, ""))) ble.requestInventory();
                handler.postDelayed(this, 850);
                return;
            }
            // L'inventaire peut arriver pendant le HTTP download : resoudre la cible ici.
            try {
                org.json.JSONObject paths = new org.json.JSONObject(prefs.getString(KEY_INVENTORY_PATHS, "{}"));
                String found = paths.optString(catalogKey(activeCatalogJob.id), null);
                if (found == null || found.isEmpty())
                    found = paths.optString(catalogKey(activeCatalogJob.title), null);
                if (found != null && found.matches(
                        "/ext/apps/(?:[A-Za-z0-9_. -]+/)*[A-Za-z0-9_. -]+\\.fap") &&
                        !found.contains("..")) {
                    activeCatalogHadExisting = true;
                    activeCatalogPath = found;
                    activeCatalogStagePath = found + ".weave-part";
                    activeCatalogBackupPath = found + ".weave-old";
                }
            } catch (org.json.JSONException ignored) { }
            if (activeCatalogStagePath == null || activeCatalogStagePath.length() >= 128) {
                finishCatalogJob(false, "Flipper installation path is too long");
                return;
            }
            activeCatalogJob.phase = "preparing";
            persistCatalogQueue("preparing", activeCatalogJob.id);
            // Keep the native infinite ring during the entire FAP upload.
            // The verified `done` status alone starts the on-device bar.
            if (activeCatalogScreenStarted) {
                String initial = activeCatalogScreenToken + "|installing|0|-|" +
                        activeCatalogBytes.length + "|" + activeCatalogStagePath;
                ble.uploadFile(SHOP_RESULT, initial.getBytes(
                        java.nio.charset.StandardCharsets.US_ASCII), "Remote Install Initial");
            } else {
                // Never transfer a FAP while the Flipper cannot display phase 1.
                finishCatalogJob(false, "Flipper native installation screen was not confirmed");
            }
        }
    };
    private void finishCatalogJob(boolean success, String message) {
        if (activeCatalogJob == null) return;
        if (success && activeCatalogScreenStarted && activeCatalogScreenToken != null &&
                !"awaiting_home".equals(activeCatalogJob.phase)) {
            // Inventory verified the final FAP; send DONE to start the native
            // cosmetic bar, but keep the Android job active until native close.
            pendingRemoteInstallFinal = activeCatalogScreenToken + "|done|-|-|-|" + activeCatalogPath;
            remoteInstallFinalQueuedAt = android.os.SystemClock.elapsedRealtime();
            remoteInstallHomeDeadline = remoteInstallFinalQueuedAt + 45000L;
            activeCatalogJob.phase = "awaiting_home";
            activeCatalogJob.percent = 100;
            persistCatalogQueue("awaiting_home", activeCatalogJob.id);
            handler.post(drainCatalogQueue);
            return;
        }
        handler.removeCallbacks(checkRemoteInstallHome);
        remoteInstallHomePollInFlight = false;
        CatalogJob finished = activeCatalogJob;
        // The companion reports success ONLY after the inventory verified the final FAP.
        if (activeCatalogScreenStarted && activeCatalogScreenToken != null) {
            pendingRemoteInstallFinal = activeCatalogScreenToken + "|" +
                    (success ? "done" : "failed") + "|-|-|-|" +
                    (success && activeCatalogPath != null ? activeCatalogPath : "-");
            remoteInstallFinalQueuedAt = android.os.SystemClock.elapsedRealtime();
        }
        activeCatalogScreenStarted = false;
        activeCatalogScreenToken = null;
        String verifiedPath = activeCatalogPath;
        String staging = activeCatalogStagePath;
        String backup = activeCatalogHadExisting ? activeCatalogBackupPath : null;
        boolean hadStartedUpload = "uploading".equals(finished.phase) ||
                "cancelling".equals(finished.phase) || "backing_up".equals(finished.phase) ||
                "committing".equals(finished.phase) || "restoring".equals(finished.phase) ||
                "verifying".equals(finished.phase);
        handler.removeCallbacks(catalogVerifyTimeout);
        handler.removeCallbacks(attemptCatalogUpload);
        handler.removeCallbacks(showRemoteDownloadScreen);
        handler.removeCallbacks(checkRemoteInstallReady);
        handler.removeCallbacks(checkRemoteInstallHome);
        catalogQueue.remove(finished);
        activeCatalogJob = null;
        if (activeCatalogTicket != null) activeCatalogTicket.cancel();
        activeCatalogTicket = null;
        activeCatalogBytes = null;
        activeCatalogPath = null;
        activeCatalogStagePath = null;
        activeCatalogBackupPath = null;
        activeCatalogHadExisting = false;
        if (!success && hadStartedUpload) scheduleCatalogPartialCleanup(staging);
        if (success && backup != null && bleConfirmed) {
            // Nettoyer l'ancien binaire apres confirmation du nouvel inventaire.
            scheduleCatalogPartialCleanup(backup);
        }
        if (success) {
            // Record the published version ONLY after a successful RPC inventory
            // verification, and only for the verified path that was actually written.
            if (verifiedPath != null && verifiedPath.startsWith("/ext/apps/") &&
                    finished.version != null && !finished.version.isEmpty()) {
                try {
                    org.json.JSONObject versions = new org.json.JSONObject(prefs.getString(
                            KEY_CATALOG_INSTALLED_VERSIONS, "{}"));
                    org.json.JSONObject record = new org.json.JSONObject();
                    String deviceKey = ble == null ? null : ble.connectedDeviceKey();
                    if (deviceKey != null && !deviceKey.isEmpty()) {
                        record.put("version", finished.version);
                        record.put("path", verifiedPath);
                        record.put("device", deviceKey);
                        versions.put(catalogKey(finished.id), record);
                        prefs.edit().putString(KEY_CATALOG_INSTALLED_VERSIONS, versions.toString()).commit();
                    }
                } catch (org.json.JSONException ignored) { }
            }
            catalogClearFailure(finished.id);
            persistCatalogQueue("completed", finished.id);
        } else if (finished.cancelRequested || "Cancelled".equals(message)) {
            catalogClearFailure(finished.id);
            persistCatalogQueue("cancelled", finished.id);
        } else catalogFailure(finished.id, message);
        if (catalogQueue.isEmpty() && deferredCatalogFilesPath != null) {
            final String browse = deferredCatalogFilesPath;
            deferredCatalogFilesPath = null;
            handler.postDelayed(() -> {
                if (bleConfirmed && ble.isIdleForShop() && catalogQueue.isEmpty())
                    ble.listFiles(browse);
            }, 1000);
        }
        handler.postDelayed(drainCatalogQueue, 400);
    }

    private static final String RING_SEARCH_REQ = "/ext/.weave/ringtone-search.req";
    private static final String RING_SEARCH_RESULT = "/ext/.weave/ringtone-search.result";
    private static final String RING_FETCH_REQ = "/ext/.weave/ringtone-fetch.req";
    private static final String RING_FETCH_RESULT = "/ext/.weave/ringtone-fetch.result";
    private static final String RING_PREVIEW_REQ = "/ext/.weave/ringtone-preview.req";
    private static final String ALERT_REQ = "/ext/.weave/alert.req";
    private String lastRingSearch, lastRingFetch;
    private boolean ringSearchBusy, ringFetchBusy;
    private static final class RingTransfer {
        final String operation, path, previewName;
        final byte[] data;
        RingTransfer(String operation, String path, byte[] data, String previewName) {
            this.operation = operation;
            this.path = path;
            this.data = data;
            this.previewName = previewName;
        }
    }
    private final ArrayDeque<RingTransfer> ringTransfers = new ArrayDeque<>();
    private RingTransfer ringTransferActive;
    private final Runnable ringDrain = new Runnable() {
        @Override public void run() {
            if (ringTransferActive != null || ringTransfers.isEmpty() || stopping) return;
            if (!bleConfirmed || ble == null || !ble.isIdleForShop() ||
                    shopTransferActive || bleCliActive || pendingShopFap != null) {
                handler.postDelayed(this, 350);
                return;
            }
            ringTransferActive = ringTransfers.removeFirst();
            ble.uploadFile(ringTransferActive.path, ringTransferActive.data,
                    ringTransferActive.operation);
        }
    };
    private long networkSequence = (System.currentTimeMillis() & 0xffffffffL);
    private static final long NETWORK_HEARTBEAT_INTERVAL_MS = 14000L;
    private static final long NETWORK_HEARTBEAT_INPUT_LIMIT_MS = 21000L;
    private long lastNetworkHeartbeatQueuedAt;
    private long lastRemoteInputAt;
    private String lastNetworkRequest;
    private boolean networkRequestBusy;
    private final Runnable networkHeartbeat = new Runnable() {
        @Override public void run() {
            if(stopping) return;
            // Un heartbeat ne doit pas bloquer un input ni saturer le RPC GUI.
            long now = android.os.SystemClock.uptimeMillis();
            boolean heartbeatDue = lastNetworkHeartbeatQueuedAt == 0 ||
                    now - lastNetworkHeartbeatQueuedAt >= NETWORK_HEARTBEAT_INTERVAL_MS;
            boolean inputIdle = now - lastRemoteInputAt >= 1200L;
            boolean heartbeatUrgent = lastNetworkHeartbeatQueuedAt != 0 &&
                    now - lastNetworkHeartbeatQueuedAt >= NETWORK_HEARTBEAT_INPUT_LIMIT_MS;
            if(heartbeatDue && (inputIdle || heartbeatUrgent) &&
                    prefs.getBoolean(KEY_GATEWAY, false) && bleConfirmed && ble != null &&
                    ble.isIdleForShop() && ringTransferActive == null && ringTransfers.isEmpty() &&
                    !shopTransferActive && !bleCliActive && catalogQueue.isEmpty()) {
                final String type = ble.isUsbActive() ? "USB" : "BLE";
                queueRingtoneTransfer("Net Link", NetworkMailbox.LINK,
                        NetworkMailbox.heartbeat(type, networkSequence++ & 0xffffffffL), null);
                lastNetworkHeartbeatQueuedAt = now;
            }
            // No periodic heartbeat transmission when the gateway is disabled.
            // Check less often until an active BLE gateway needs the scheduler.
            handler.postDelayed(this,
                    prefs.getBoolean(KEY_GATEWAY, false) && bleConfirmed ? 1400 : 3200);
        }
    };
    private void handleNetworkRequest(byte[] bytes) {
        if(!prefs.getBoolean(KEY_GATEWAY, false) || !bleConfirmed || networkRequestBusy) return;
        NetworkMailbox.Request request = NetworkMailbox.parse(bytes);
        if(request == null) return;
        String identity = request.id + "|" + request.verb + "|" + request.target;
        if(identity.equals(lastNetworkRequest)) return;
        lastNetworkRequest = identity;
        networkRequestBusy = true;
        network.execute(() -> {
            NetworkGateway.Result result;
            try { result = NetworkGateway.execute(request, true); }
            catch(Exception error) { result = new NetworkGateway.Result(6, 0, new byte[0]); }
            final byte[] reply = NetworkMailbox.result(request.id, result);
            handler.post(() -> {
                networkRequestBusy = false;
                if(!stopping && bleConfirmed && prefs.getBoolean(KEY_GATEWAY, false)) {
                    queueRingtoneTransfer("Net Reply", NetworkMailbox.RESPONSE, reply, null);
                    // This marker is queued after, and can only be uploaded once
                    // the complete response has been acknowledged by Storage RPC.
                    queueRingtoneTransfer("Net Ready", NetworkMailbox.READY,
                            NetworkMailbox.ready(request.id), null);
                }
            });
        });
    }
    private void queueRingtoneTransfer(String operation, String path, byte[] data,
                                        String previewName) {
        if(data == null) return;
        // Latest-value wins for live text. Never queue one SD write per keystroke.
        if("Text Preview".equals(operation) || "Text Input".equals(operation)) {
            ringTransfers.removeIf(job -> "Text Preview".equals(job.operation));
        }
        if(ringTransfers.size() >= 16) return;
        if("Ring Call End".equals(operation) || "Text Input".equals(operation) ||
                "Text Preview".equals(operation))
            ringTransfers.addFirst(new RingTransfer(operation, path, data, previewName));
        else ringTransfers.addLast(new RingTransfer(operation, path, data, previewName));
        handler.post(ringDrain);
    }
    private void handleRingtoneSearch(byte[] bytes) {
        if (bytes == null || bytes.length < 3 || bytes.length > 80 || ringSearchBusy) return;
        String token = new String(bytes, java.nio.charset.StandardCharsets.UTF_8).trim();
        if (token.equals(lastRingSearch)) return;
        int sep = token.lastIndexOf('|');
        String query = sep > 0 ? token.substring(0, sep) : token;
        if (!query.toLowerCase(Locale.ROOT).matches("[a-z0-9 _-]{1,30}")) return;
        lastRingSearch = token;
        ringSearchBusy = true;
        network.execute(() -> {
            StringBuilder result = new StringBuilder();
            try {
                for (RingtoneCatalog.Result item : RingtoneCatalog.search(query))
                    result.append(item.source).append('|').append(item.path).append('\n');
            } catch (Exception error) { setStatus("Ringtone search: " + error.getClass().getSimpleName()); }
            byte[] output = (result.length() == 0 ? "-\n" : result.toString())
                    .getBytes(java.nio.charset.StandardCharsets.UTF_8);
            handler.post(() -> {
                ringSearchBusy = false;
                queueRingtoneTransfer("Ring Search", RING_SEARCH_RESULT, output, null);
                queueRingtoneTransfer("Ring Cleanup", RING_SEARCH_REQ,
                        "-".getBytes(java.nio.charset.StandardCharsets.US_ASCII), null);
            });
        });
    }
    private void handleRingtoneFetch(byte[] bytes) {
        if (bytes == null || bytes.length < 5 || bytes.length > 180 || ringFetchBusy) return;
        String token = new String(bytes, java.nio.charset.StandardCharsets.UTF_8).trim();
        if (token.equals(lastRingFetch)) return;
        String[] parts = token.split("\\|", 4);
        if (parts.length < 3 || !("P".equals(parts[0]) || "D".equals(parts[0])) ||
                !("0".equals(parts[1]) || "1".equals(parts[1])) ||
                !RingtoneCatalog.validPath(parts[2])) return;
        final boolean preview = "P".equals(parts[0]);
        final int source = Integer.parseInt(parts[1]);
        final String path = parts[2];
        final String name = RingtoneCatalog.safeLocalName(source, path, preview);
        lastRingFetch = token;
        ringFetchBusy = true;
        network.execute(() -> {
            byte[] data = null;
            try { data = RingtoneCatalog.download(source, path); }
            catch (Exception error) {setStatus("Ringtone download: " + error.getClass().getSimpleName());}
            final byte[] payload = data;
            handler.post(() -> {
                ringFetchBusy = false;
                if (payload != null)
                    queueRingtoneTransfer("Ring Fetch", "/ext/.weave/ringtones/" + name,
                            payload, preview ? name : null);
                else queueRingtoneTransfer("Ring Result", RING_FETCH_RESULT,
                        "error|download unavailable".getBytes(java.nio.charset.StandardCharsets.US_ASCII),null);
                queueRingtoneTransfer("Ring Cleanup", RING_FETCH_REQ,
                        "-".getBytes(java.nio.charset.StandardCharsets.US_ASCII),null);
            });
        });
    }
    private static final String FIND_FLIPPER_FILE = "/ext/.weave/find-flipper.req";
    private static final String FIND_DEVICE_FILE = "/ext/.weave/find-device.req";
    private static final String TEXT_INPUT_REQUEST = "/ext/.weave/text-input.req";
    private static final String TEXT_INPUT_REPLY = "/ext/.weave/text-input.reply";
    private String lastTextInputToken;
    private String completedRemoteTextToken;
    // The request file is the *last event*, not a live state flag. Its contents
    // survive Android service restarts and may be hours or days old.
    private static final String KEY_FIND_DEVICE_LAST_REQUEST_PREFIX = "find_device_last_request_v2_";
    private boolean findDeviceInitialReadComplete;
    private String findDeviceConnectedKey;
    private static final String FW_CHECK_REQ = "/ext/.weave/fw-check.req";
    private static final String FW_CHECK_RESULT = "/ext/.weave/fw-check.result";
    private String lastFirmwareCheckToken;
    private int findPollPhase;
    private ToneGenerator findTone;
    private int previousAlarmVolume = -1;
    private final Runnable findToneRepeat = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if(findTone == null || !prefs.getBoolean(KEY_FIND_RINGING, false)) return;
            try { findTone.startTone(ToneGenerator.TONE_CDMA_ALERT_CALL_GUARD, 1900); }
            catch(RuntimeException ignored) { }
            handler.postDelayed(this, 1950);
        }
    };
    private void stopDeviceRingNow() {
        handler.removeCallbacks(findToneRepeat);
        if(findTone != null) {
            try { findTone.stopTone(); findTone.release(); } catch(RuntimeException ignored) { }
            findTone = null;
        }
        try {
            Vibrator vibrator = (Vibrator)getSystemService(VIBRATOR_SERVICE);
            if(vibrator != null) vibrator.cancel();
        } catch(RuntimeException ignored) { }
        if(previousAlarmVolume >= 0) {
            try {
                AudioManager audio = (AudioManager)getSystemService(AUDIO_SERVICE);
                if(audio != null) audio.setStreamVolume(AudioManager.STREAM_ALARM, previousAlarmVolume, 0);
            } catch(RuntimeException ignored) { }
            previousAlarmVolume = -1;
        }
        prefs.edit().putBoolean(KEY_FIND_RINGING, false).apply();
        NotificationManager manager = getSystemService(NotificationManager.class);
        if(manager != null) manager.cancel(FIND_NOTIFICATION_ID);
        sendBroadcast(new Intent(EVENT_FIND_DEVICE).setPackage(getPackageName())
                .putExtra("ringing", false));
    }
    static final String EVENT_FIND_DEVICE = "io.github.nullmess.weave.FIND_DEVICE_STATE";
    private void soundFindDevice() {
        // Do not briefly cancel an active alarm: that would dismiss the UI
        // and dispatch a delayed Stop just as a second request arrives.
        if(prefs.getBoolean(KEY_FIND_RINGING, false)) return;
        stopDeviceRingNow();
        try {
            AudioManager audio = (AudioManager)getSystemService(AUDIO_SERVICE);
            if(audio != null) {
                previousAlarmVolume = audio.getStreamVolume(AudioManager.STREAM_ALARM);
                audio.setStreamVolume(AudioManager.STREAM_ALARM,
                        audio.getStreamMaxVolume(AudioManager.STREAM_ALARM), 0);
            }
        } catch(RuntimeException ignored) { previousAlarmVolume = -1; }
        try {
            findTone = new ToneGenerator(AudioManager.STREAM_ALARM, 100);
            // Each pulse ends; the worker renews it until the user explicitly stops.
            Vibrator vibrator = (Vibrator)getSystemService(VIBRATOR_SERVICE);
            if(vibrator != null && vibrator.hasVibrator()) {
                if(Build.VERSION.SDK_INT >= 26)
                    vibrator.vibrate(VibrationEffect.createWaveform(new long[]{0, 400, 150, 400}, 0));
                else vibrator.vibrate(new long[]{0, 400, 150, 400}, 0);
            }
        } catch(RuntimeException e) { setStatus("Find device: audio unavailable"); }
        prefs.edit().putBoolean(KEY_FIND_RINGING, true).apply();
        handler.post(findToneRepeat);
        createChannel();
        NotificationManager manager = getSystemService(NotificationManager.class);
        if(manager != null) manager.notify(FIND_NOTIFICATION_ID, findDeviceNotification());
        sendBroadcast(new Intent(EVENT_FIND_DEVICE).setPackage(getPackageName())
                .putExtra("ringing", true));
    }
    private Notification findDeviceNotification() {
        Intent openIntent = new Intent(this, MainActivity.class)
                .addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP)
                .putExtra("show_find_device", true);
        PendingIntent open = PendingIntent.getActivity(this, 419, openIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        /* Stop action opens the same single-button modal as tapping the
         * notification itself. Dismissing that modal also stops the ring. */
        PendingIntent stop = PendingIntent.getActivity(this, 420, openIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        return new Notification.Builder(this, FIND_CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_weave_notification)
                .setContentTitle("Find my Device")
                .setContentText("Ringing · Open Weave to stop")
                .setCategory(Notification.CATEGORY_ALARM)
                .setContentIntent(open)
                .addAction(android.R.drawable.ic_media_pause, "Stop", stop)
                .setOngoing(true)
                .setOnlyAlertOnce(true)
                .build();
    }
    private void writeFirmwareCheckResult(byte[] result,int attempt) {
        if(stopping || !bleConfirmed || attempt>12)return;
        if(ble.isIdleForShop()&&catalogQueue.isEmpty()&&!shopTransferActive)
            ble.uploadFile(FW_CHECK_RESULT,result,"Firmware Check");
        else handler.postDelayed(() -> writeFirmwareCheckResult(result,attempt+1),1800);
    }
    private void onFindDeviceRequestRead(byte[] bytes, String error) {
        if (!bleConfirmed || ble == null) return;
        String deviceKey = ble.connectedDeviceKey();
        if (deviceKey == null || deviceKey.isEmpty()) return;
        // Do not associate requests from a different Flipper with this session.
        if (!deviceKey.equals(findDeviceConnectedKey)) {
            findDeviceConnectedKey = deviceKey;
            findDeviceInitialReadComplete = false;
        }
        // A failed RPC or a disconnect is not a usable initial snapshot.
        // A legitimate "file does not exist" reply *is* a usable empty snapshot.
        boolean readable = bytes != null;
        boolean missing = error != null && error.startsWith("Flipper file read error ");
        if (!readable && !missing) return;
        String token = readable ? new String(bytes, java.nio.charset.StandardCharsets.US_ASCII).trim() : "";
        boolean valid = token.matches("[0-9]{1,12}-[0-9]{1,12}");
        String key = KEY_FIND_DEVICE_LAST_REQUEST_PREFIX + deviceKey;
        String previouslyProcessed = prefs.getString(key, "");
        if (!findDeviceInitialReadComplete) {
            // First snapshot after enabling/reconnecting is a baseline, NEVER an
            // instruction to ring. A stale .req must not trigger Find my Device
            // just because Service Weave was enabled.
            findDeviceInitialReadComplete = true;
            if (valid && !token.equals(previouslyProcessed))
                prefs.edit().putString(key, token).apply();
            return;
        }
        // Only a NEW Flipper button press changes the token. Remember it before
        // making noise, so even process death/reconnect cannot replay it.
        if (!valid || token.equals(previouslyProcessed)) return;
        prefs.edit().putString(key, token).apply();
        soundFindDevice();
        setStatus("Flipper requested Find my Device");
        // Deliberately leave the token on SD. Deleting inside fileData() used to
        // fail: BleLink still holds readId until after this callback returns.
        // Keeping the last token avoids a delete/write race with a new request.
    }
    private void onRemoteTextInputRead(byte[] bytes, String error) {
        if(bytes == null) {
            if(error == null || !error.startsWith("Flipper file read error ")) return;
            if(lastTextInputToken != null) {
                lastTextInputToken = null;
                prefs.edit().remove(KEY_TEXT_TOKEN).remove(KEY_TEXT_HEADER)
                        .remove(KEY_TEXT_LIMIT).apply();
                sendBroadcast(new Intent(EVENT_TEXT_INPUT).setPackage(getPackageName())
                        .putExtra("visible", false));
            }
            return;
        }
        if(bytes.length < 5 || bytes.length > 110) return;
        String request = new String(bytes, java.nio.charset.StandardCharsets.US_ASCII);
        String[] parts = request.split("\\|", 3);
        if(parts.length != 3 || !parts[0].matches("[0-9]{1,12}-[0-9]{1,12}")) return;
        int limit;
        try { limit = Integer.parseInt(parts[1]); } catch(NumberFormatException e) { return; }
        if(limit < 1 || limit > 256 || parts[2].length() > 35) return;
        if(parts[0].equals(lastTextInputToken)) return;
        lastTextInputToken = parts[0];
        completedRemoteTextToken = null;
        prefs.edit().putString(KEY_TEXT_TOKEN, parts[0])
                .putString(KEY_TEXT_HEADER, parts[2]).putInt(KEY_TEXT_LIMIT, limit).apply();
        sendBroadcast(new Intent(EVENT_TEXT_INPUT).setPackage(getPackageName())
                .putExtra("visible", true));
    }
    /* One shared, fair RPC scheduler: individual 850ms/1250ms polling loops
     * previously raced on BleLink.readId and could starve the keyboard forever.
     * Alternate TextInput and FindDevice; keep firmware release checks alive. */
    private int bridgeProbeIndex;
    private long lastBridgeProbeAt;
    private final Runnable bridgePoll = new Runnable() {
        @Override public void run() {
            if(stopping) return;
            // Les RPC de fond s'effacent pendant les inputs GUI.
            long sinceInput = android.os.SystemClock.uptimeMillis() - lastRemoteInputAt;
            long now = android.os.SystemClock.uptimeMillis();
            // Keep the BLE RPC path for physical button presses and live GUI
            // frames. Slow noninteractive SD mailbox probes when mirroring, and
            // hold them briefly after a gamepad press to prevent read traffic
            // competing with the next input/frame pair.
            boolean mirroring = prefs.getBoolean("screen_requested", false);
            // Once the native keyboard is active, notice its closure promptly.
            // Keep the normal backoff otherwise so GUI frames do not starve.
            long minimumProbeGap = lastTextInputToken != null ?
                    (mirroring ? 1300L : 750L) : (mirroring ? 2300L : 750L);
            long userQuietTime = mirroring ? 2200L : 1200L;
            if(sinceInput >= userQuietTime && now - lastBridgeProbeAt >= minimumProbeGap &&
                    pendingFindFlipperToken == null &&
                    bleConfirmed && ble != null && ble.isIdleForShop() &&
                    ringTransferActive == null && !shopTransferActive && !shopNetworkBusy &&
                    pendingShopStatus == null && pendingShopFap == null &&
                    catalogQueue.isEmpty() && !shopVerifying && !bleCliActive) {
                final int slot = bridgeProbeIndex++ % 8;
                final String path = slot == 0 || slot == 2 || slot == 6
                        ? TEXT_INPUT_REQUEST : slot == 3 ? SHOP_REQUEST
                        : slot == 5 ? FW_CHECK_REQ
                        : slot == 7 && prefs.getBoolean(KEY_GATEWAY, false)
                        ? NetworkMailbox.REQUEST : FIND_DEVICE_FILE;
                lastBridgeProbeAt = now;
                ble.readFile(path);
            }
            // The global keyboard/finder bridge stays alive in every tab,
            // but an offline Flipper does not need a 650ms UI-thread wakeup.
            handler.postDelayed(this, bleConfirmed && ble != null ? 650 : 2200);
        }
    };
    private void sendRemoteTextInput(Intent intent) {
        if(!bleConfirmed || intent == null || !prefs.getBoolean(KEY_ENABLED, false)) return;
        String token = intent.getStringExtra("token");
        String value = intent.getStringExtra("text");
        if(token == null || !token.equals(lastTextInputToken) || value == null ||
                token.equals(completedRemoteTextToken)) return;
        boolean preview = ACTION_TEXT_INPUT_PREVIEW.equals(intent.getAction());
        int limit = prefs.getInt(KEY_TEXT_LIMIT, 0);
        if(value.length() > limit || value.length() > 256 ||
                value.indexOf('\n') >= 0 || value.indexOf('\r') >= 0) return;
        for(int i = 0; i < value.length(); ++i) {
            char c = value.charAt(i);
            if(c < 32 || c > 126) return;
        }
        byte[] reply = (token + (preview ? "|P|" : "|F|") + value + "\n")
                .getBytes(java.nio.charset.StandardCharsets.US_ASCII);
        queueRingtoneTransfer(preview ? "Text Preview" : "Text Input",
                TEXT_INPUT_REPLY, reply, null);
        if(preview) return;
        completedRemoteTextToken = token;
        prefs.edit().remove(KEY_TEXT_TOKEN).remove(KEY_TEXT_HEADER)
                .remove(KEY_TEXT_LIMIT).apply();
        sendBroadcast(new Intent(EVENT_TEXT_INPUT).setPackage(getPackageName())
                .putExtra("visible", false));
        // Keep the last token so the same active input never reopens the modal.
    }
    private final Runnable shopProcess = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (stopping) return;
            // A queued phone install must not starve an active Flipper-origin
            // result (especially FAILED). The phone queue is already held by
            // drainCatalogQueueNow until this native job has closed.
            if (activeCatalogJob != null || pendingRemoteInstallFinal != null) {
                handler.postDelayed(this, 650);
                return;
            }
            if (!bleConfirmed || !ble.isIdleForShop()) {
                if (pendingShopStatus != null || pendingShopFap != null ||
                        pendingShopCatalog != null)
                    handler.postDelayed(this, 750);
                return;
            }
            if (shopRequestDevice != null && !shopRequestDevice.equals(ble.connectedDeviceKey())) {
                pendingShopFap = null;
                pendingShopPath = null;
                pendingShopStatus = null;
                shopRequestToken = null;
                shopRequestDevice = null;
                nativeShopHomeToken = null;
                nativeShopHomeOutcome = null;
                handler.removeCallbacks(checkNativeShopHome);
                shopTransferActive = false;
                shopVerifying = false;
                shopVerificationRequested = false;
                handler.removeCallbacks(shopVerify);
                clearShopNotification();
            }
            if (pendingShopStatus != null) {
                String status = pendingShopStatus;
                pendingShopStatus = null;
                shopStatusInFlight = status;
                ble.uploadFile(SHOP_RESULT, status.getBytes(java.nio.charset.StandardCharsets.UTF_8), "Shop Result");
            } else if (pendingShopFap != null && pendingInstallPath == null &&
                    !shopTransferActive && !shopVerifying) {
                pendingInstallBytes = pendingShopFap;
                pendingInstallPath = pendingShopPath;
                pendingShopFap = null;
                pendingShopPath = null;
                shopTransferActive = true;
                ble.mkdir(pendingInstallPath.substring(0, pendingInstallPath.lastIndexOf('/')));
            } else if (pendingShopCatalog != null) {
                String index = pendingShopCatalog;
                pendingShopCatalog = null;
                ble.uploadFile(SHOP_INDEX, index.getBytes(java.nio.charset.StandardCharsets.UTF_8), "Shop Sync");
            }
            if (pendingShopStatus != null || pendingShopFap != null ||
                    pendingShopCatalog != null)
                handler.postDelayed(this, 800);
        }
    };
    private static final String CLI_IN = "/ext/.weave/cli.in";
    private static final String CLI_OUT = "/ext/.weave/cli.out";
    private final ArrayDeque<String> bleCliCommands = new ArrayDeque<>();
    private final Runnable sendPendingBleCli = this::drainBleCliCommands;
    private void drainBleCliCommands() {
        handler.removeCallbacks(sendPendingBleCli);
        if (!bleCliActive || !bleConfirmed || !bleCliReady || bleCliCommands.isEmpty()) return;
        if (!ble.isIdleForShop()) {
            handler.postDelayed(sendPendingBleCli, 300);
            return;
        }
        String command = bleCliCommands.pollFirst();
        ble.uploadFile(CLI_IN, (command + "\r\n").getBytes(
                java.nio.charset.StandardCharsets.UTF_8), "BLE CLI");
    }
    private boolean bleCliActive, bleCliReady;
    private int bleCliStartAttempts;
    private final Runnable bleCliBootstrap = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (stopping || !bleCliActive || bleCliReady) return;
            if (!bleConfirmed || !ble.isRpcReady()) return;
            if (ble.isIdleForShop()) {
                // Start the real shell with a harmless built-in command. The firmware
                // creates /ext/.weave/cli.out only after accepting this request.
                ble.uploadFile(CLI_IN, "help\r\n".getBytes(
                        java.nio.charset.StandardCharsets.UTF_8), "BLE CLI");
            } else if (++bleCliStartAttempts <= 30) {
                handler.postDelayed(this, 550);
            } else emitCli("\n[BLE CLI busy; try again]\n");
        }
    };
    private byte[] previousBleCliOutput = new byte[0];
    private final Runnable bleCliPoll = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (bleCliActive && !stopping) {
                if (bleConfirmed && ble.isIdleForShop()) ble.readFile(CLI_OUT);
                handler.postDelayed(this, 750);
                if (!bleCliCommands.isEmpty()) handler.post(sendPendingBleCli);
            }
        }
    };
    private void emitCli(String text) {
        sendBroadcast(new Intent(EVENT_CLI).setPackage(getPackageName()).putExtra("text", text));
    }
    private void onBleCliOutput(byte[] bytes) {
        if (!bleCliActive || bytes == null) return;
        int skip = 0;
        if (bytes.length >= previousBleCliOutput.length) {
            boolean prefix = true;
            for (int i = 0; i < previousBleCliOutput.length; i++) {
                if (bytes[i] != previousBleCliOutput[i]) { prefix = false; break; }
            }
            if (prefix) skip = previousBleCliOutput.length;
        }
        previousBleCliOutput = bytes;
        if (!bleCliReady) {
            bleCliReady = true;
            emitCli("\n[BLE CLI ready]\n");
        }
        if (skip < bytes.length)
            emitCli(new String(bytes, skip, bytes.length - skip, java.nio.charset.StandardCharsets.UTF_8));
        if (!bleCliCommands.isEmpty()) handler.post(sendPendingBleCli);
    }
    private boolean fapIndexPending;
    private String fapIndexToken;
    private long lastFapIndexAt;
    private int fapIndexReadAttempts;
    private final Runnable requestFapIndex = new Runnable() {
        @Override public void run() {
            if (stopping || !bleConfirmed || fapIndexPending || ble == null) return;
            if (!ble.isIdleForShop() || !catalogQueue.isEmpty() || shopTransferActive ||
                    bleCliActive || deletingBatch || activeCatalogJob != null) {
                handler.postDelayed(this, 2500);
                return;
            }
            fapIndexPending = true;
            fapIndexReadAttempts = 0;
            lastFapIndexAt = android.os.SystemClock.elapsedRealtime();
            fapIndexToken = Long.toString(android.os.SystemClock.elapsedRealtime());
            ble.uploadFile(FAP_INDEX_REQ, (fapIndexToken + "\n").getBytes(
                    java.nio.charset.StandardCharsets.US_ASCII), "FAP Index Request");
        }
    };
    private final Runnable readFapIndex = new Runnable() {
        @Override public void run() {
            if (!fapIndexPending || stopping || !bleConfirmed) return;
            if (fapIndexReadAttempts >= 8) { fapIndexPending = false; return; }
            if (!ble.isIdleForShop()) { handler.postDelayed(this, 1800); return; }
            ++fapIndexReadAttempts;
            ble.readFile(FAP_INDEX_RESULT);
        }
    };
    private void onFapIndex(byte[] bytes) {
        if (!fapIndexPending) return;
        if (bytes == null || bytes.length < 5 || bytes.length > 110000) {
            handler.postDelayed(readFapIndex, 1500);
            return;
        }
        try {
            String lines = new String(bytes, java.nio.charset.StandardCharsets.UTF_8);
            if (fapIndexToken == null || !lines.startsWith("#" + fapIndexToken + "\n")) {
                handler.postDelayed(readFapIndex, 1400);
                return;
            }
            org.json.JSONObject metadata = new org.json.JSONObject();
            int valid = 0;
            for (String line : lines.split("\n")) {
                String[] fields = line.split("\\|", 5);
                if (fields.length != 5 || !fields[0].matches(
                        "/ext/apps/[A-Za-z0-9 _.-]{1,40}/[A-Za-z0-9 _.-]{1,100}\\.fap")) continue;
                long size = Long.parseLong(fields[2]);
                long version = Long.parseLong(fields[3]);
                if (size <= 0 || size > 16_777_216 || version < 0 || version > 0xffffffffL) continue;
                org.json.JSONObject fap = new org.json.JSONObject();
                fap.put("title", fields[1]);
                fap.put("size", size);
                fap.put("version", version);
                if (fields[4].matches("[0-9a-fA-F]{64}")) fap.put("icon", fields[4]);
                metadata.put(fields[0], fap);
                if (++valid >= 300) break;
            }
            String device = ble.connectedDeviceKey();
            if (device == null) return;
            prefs.edit().putString(KEY_FAP_METADATA, metadata.toString())
                    .putString(KEY_FAP_METADATA_DEVICE, device).apply();
            fapIndexPending = false;
            fapIndexToken = null;
            sendBroadcast(new Intent(EVENT_FAP_METADATA).setPackage(getPackageName()));
        } catch (Exception ignored) { handler.postDelayed(readFapIndex, 1500); }
    }
    private boolean stopping;
    private boolean priorBluetoothLink;
    private final Runnable sendPendingFindFlipper = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (pendingFindFlipperToken == null || stopping) return;
            if (android.os.SystemClock.uptimeMillis() > findFlipperDeadline) {
                pendingFindFlipperToken = null;
                setStatus("Find my Flipper timed out: RPC connection unavailable");
                return;
            }
            if (bleConfirmed && ble != null && ble.isIdleForShop() &&
                    catalogQueue.isEmpty() && !shopTransferActive && !bleCliActive) {
                final String token = pendingFindFlipperToken;
                pendingFindFlipperToken = null;
                ble.uploadFile(FIND_FLIPPER_FILE,
                        token.getBytes(java.nio.charset.StandardCharsets.US_ASCII), "Find Flipper");
                return;
            }
            handler.postDelayed(this, 450);
        }
    };

    private String lastStatus = "";
    private long lastBleScan;
    private long lastUsbAttempt;
    private long lastUsbStatusPoll;
    private long lastBleInfoRefresh;
    private boolean bluetoothReceiverRegistered;
    private boolean usbReceiverRegistered;
    private String lastUsbPermissionDevice;
    private long lastUsbPermissionTime;
    private final BroadcastReceiver bluetoothStateReceiver = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            if (!BluetoothAdapter.ACTION_STATE_CHANGED.equals(intent.getAction())) return;
            int state = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.ERROR);
            if (state == BluetoothAdapter.STATE_ON && !stopping) {
                lastBleScan = 0;
                handler.post(reconnect);
            } else if (state == BluetoothAdapter.STATE_OFF) {
                if (!usbConfirmed) {
                    ble.close();
                    bleConfirmed = false;
                    updateConnection();
                    setStatus("Bluetooth disabled — enable it to reconnect");
                }
            }
        }
    };

    private final BroadcastReceiver usbEvents = new BroadcastReceiver() {
        @Override public void onReceive(Context context, Intent intent) {
            if (intent == null || intent.getAction() == null) return;
            String action = intent.getAction();
            if (ACTION_USB_PERMISSION.equals(action)) {
                if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                    lastUsbAttempt = 0;
                    handler.post(() -> attemptUsb());
                } else setStatus("Flipper USB permission denied — Bluetooth remains available");
            } else if (UsbManager.ACTION_USB_DEVICE_ATTACHED.equals(action)) {
                lastUsbAttempt = 0;
                handler.post(() -> attemptUsb());
            } else if (UsbManager.ACTION_USB_DEVICE_DETACHED.equals(action)) {
                UsbDevice device = null;
                if (Build.VERSION.SDK_INT >= 33) device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE, UsbDevice.class);
                else device = intent.getParcelableExtra(UsbManager.EXTRA_DEVICE);
                if (UsbLink.isFlipper(device)) {
                    usb.close();
                    usbConfirmed = false;
                    ble.onUsbDisconnected();
                    updateConnection();
                    handler.post(reconnect);
                }
            }
        }
    };

    private final Runnable activateReadyUsb = new Runnable() {
        @Override public void run() {
            if (stopping || !prefs.getBoolean(KEY_ENABLED, false) || !usb.isReady()) return;
            if (bleConfirmed && !ble.isIdleForShop()) {
                // Never interrupt a queued FAP upload, CLI session, or pending
                // operation merely because a USB cable was plugged in.
                handler.postDelayed(this, 400);
                return;
            }
            usbConfirmed = true;
            cli.close();
            ble.activateUsb(usb);
            updateConnection();
        }
    };

    private long recoveryReconnectPauseUntil;
    private final ConnectionBackoff bleBackoff = new ConnectionBackoff();

    private final Runnable reconnect = new Runnable() {
        @Override public void run() {
            handler.removeCallbacks(this);
            if (stopping || !prefs.getBoolean(KEY_ENABLED, false)) return;
            if (android.os.SystemClock.uptimeMillis() < recoveryReconnectPauseUntil) {
                handler.postDelayed(this, 3000);
                return;
            }
            long now = System.currentTimeMillis();
            // USB detach/attach est signale par BroadcastReceiver: poll de secours lent.
            if (usbConfirmed && now - lastUsbStatusPoll >= 30000L) {
                lastUsbStatusPoll = now;
                if (usb.discover().stream().noneMatch(usb::isSameDevice)) {
                    usb.close();
                    usbConfirmed = false;
                    ble.onUsbDisconnected();
                }
            }
            String mode = prefs.getString(KEY_MODE, "Auto");
            // Pas d'enumeration USB toutes les 3 s sur le thread UI pendant le BLE.
            long usbProbeInterval = bleConfirmed ? 30000L : 6000L;
            if (!usbConfirmed && !usb.isOpen() && !"Bluetooth".equals(mode) &&
                    now - lastUsbAttempt >= usbProbeInterval) {
                lastUsbAttempt = now;
                attemptUsb();
            }
            if (!usbConfirmed) {
                if (!hasBlePermission()) {
                    setStatus("Bluetooth permissions required — open Settings in Weave");
                } else if (!ble.isAvailable()) {
                    setStatus("Enable Bluetooth to connect to Flipper");
                } else if (ble.isConnected()) {
                    if (now - lastBleInfoRefresh >= 45000) {
                        lastBleInfoRefresh = now;
                        ble.refreshInformation();
                    }
                } else if (!ble.isConnecting() && bleBackoff.permits(
                        android.os.SystemClock.uptimeMillis())) {
                    // Reconnect directly to a bonded Flipper even if a .fap is open.
                    // Unpaired discovery stays less frequent to limit battery use.
                    BluetoothDevice paired = ble.pairedFlipper();
                    long retryInterval = paired != null ? 6500 : 20000;
                    if (now - lastBleScan >= retryInterval) {
                        lastBleScan = now;
                        if (paired != null) {
                            foundBle = paired;
                            bleBackoff.attempted(android.os.SystemClock.uptimeMillis());
                            ble.connect(paired);
                        } else {
                            bleBackoff.attempted(android.os.SystemClock.uptimeMillis());
                            ble.scan();
                        }
                    }
                }
            }
            handler.postDelayed(this, 3000);
        }
    };

    @Override public void onCreate() {
        super.onCreate();
        prefs = getSharedPreferences(PREFS, MODE_PRIVATE);
        // A previous Android process cannot leave a valid audible alarm behind.
        prefs.edit().putBoolean(KEY_FIND_RINGING, false).apply();
        restoreCatalogCleanup();
        restoreCatalogQueue();
        cli = new FlipperUsbCli(this, new FlipperUsbCli.Callbacks() {
            @Override public void status(String message) {
                sendBroadcast(new Intent(EVENT_CLI).setPackage(getPackageName())
                        .putExtra("text", "\n[" + message + "]\n"));
            }
            @Override public void data(String text) {
                sendBroadcast(new Intent(EVENT_CLI).setPackage(getPackageName())
                        .putExtra("text", text));
            }
        });
        usb = new UsbLink(this, new UsbLink.Callbacks() {
            @Override public void status(String message) { handler.post(() -> setStatus(message)); }
            @Override public void ready() {
                handler.post(() -> {
                    if (stopping || !prefs.getBoolean(KEY_ENABLED, false)) return;
                    handler.removeCallbacks(activateReadyUsb);
                    handler.post(activateReadyUsb);
                });
            }
            @Override public void bytes(byte[] data) { ble.onUsbRpcBytes(data); }
            @Override public void disconnected() {
                NetworkGateway.reset();
                lastNetworkRequest = null;
                handler.post(() -> {
                    if (stopping || usb.isReady()) return;
                    usbConfirmed = false;
                    ble.onUsbDisconnected();
                    updateConnection();
                    handler.post(reconnect);
                });
            }
        });
        ble = new BleLink(this, new BleLink.Callbacks() {
            @Override public void status(String message) { setStatus(message); }
            @Override public void found(BluetoothDevice device) {
                foundBle = device;
                setStatus("Flipper BLE discovered");
                String mode = prefs.getString(KEY_MODE, "Auto");
                if (!usbConfirmed) connectBle();
            }
            @Override public void connected(boolean connected) {
                // Diagnostic: compter les pertes radio, pas les transferts USB.
                if (!connected && priorBluetoothLink && !ble.isUsbActive())
                    prefs.edit().putInt(KEY_BLE_DROPS,
                            prefs.getInt(KEY_BLE_DROPS, 0) + 1).apply();
                priorBluetoothLink = connected && !ble.isUsbActive();
                if (!connected) {
                    lastNetworkHeartbeatQueuedAt = 0;
                    fapIndexPending = false;
                    fapIndexToken = null;
                    handler.removeCallbacks(requestFapIndex);
                    handler.removeCallbacks(readFapIndex);
                }
                bleConfirmed = connected;
                if (connected) bleBackoff.ready();
                usbConfirmed = connected && ble.isUsbActive();
                // Each BLE session must first observe the existing request file
                // without executing it. Only subsequent, changed tokens ring.
                findDeviceInitialReadComplete = false;
                findDeviceConnectedKey = connected ? ble.connectedDeviceKey() : null;
                if (!connected) {
                    lastTextInputToken = null;
                    prefs.edit().remove(KEY_TEXT_TOKEN).remove(KEY_TEXT_HEADER)
                            .remove(KEY_TEXT_LIMIT).apply();
                    sendBroadcast(new Intent(EVENT_TEXT_INPUT).setPackage(getPackageName())
                            .putExtra("visible", false));
                    if (activeCatalogJob != null && activeCatalogJob.cancelRequested) {
                        finishCatalogJob(false, "Cancelled");
                    } else if (activeCatalogJob != null) {
                        // Roll back an interrupted transfer, do not claim completion.
                        handler.removeCallbacks(catalogVerifyTimeout);
                        handler.removeCallbacks(attemptCatalogUpload);
                        activeCatalogJob.phase = "queued";
                        activeCatalogJob.percent = -1;
                        activeCatalogJob = null;
                        if (activeCatalogTicket != null) activeCatalogTicket.cancel();
                        activeCatalogTicket = null;
                        activeCatalogBytes = null;
                        activeCatalogPath = null;
                        activeCatalogStagePath = null;
                        activeCatalogBackupPath = null;
                        activeCatalogHadExisting = false;
                        persistCatalogQueue("reconnecting", null);
                    }
                    if (bleCliActive) emitCli("\n[BLE CLI disconnected]\n");
                    bleCliActive = false;
                    bleCliReady = false;
                    handler.removeCallbacks(bleCliPoll);
                    handler.removeCallbacks(bleCliBootstrap);
                    handler.removeCallbacks(sendPendingBleCli);
                    bleCliCommands.clear();
                    prefs.edit().remove(KEY_STORAGE_INT_TOTAL).remove(KEY_STORAGE_INT_FREE)
                            .remove(KEY_STORAGE_EXT_TOTAL).remove(KEY_STORAGE_EXT_FREE)
                            .remove(KEY_STORAGE_INT_ERROR).remove(KEY_STORAGE_EXT_ERROR).apply();
                    prefs.edit().remove(KEY_BATTERY).remove(KEY_FIRMWARE).remove(KEY_DEVICE_NAME)
                            .remove(KEY_INVENTORY).remove(KEY_INVENTORY_PATHS).remove(KEY_FIRMWARE_API)
                            .remove(KEY_CATALOG_ACTIVE_DEVICE)
                .remove(KEY_STORAGE_INT_TOTAL).remove(KEY_STORAGE_INT_FREE)
                .remove(KEY_STORAGE_EXT_TOTAL).remove(KEY_STORAGE_EXT_FREE)
                .remove(KEY_STORAGE_INT_ERROR).remove(KEY_STORAGE_EXT_ERROR)
                .putString(KEY_INVENTORY_STATUS, "unavailable").apply();
                } else {
                    prefs.edit().remove(KEY_INVENTORY).putString(KEY_INVENTORY_STATUS, "loading").apply();
                    // Restore only a firmware API that was previously verified on this exact Flipper.
                    String deviceKey = ble.connectedDeviceKey();
                    if (deviceKey != null) prefs.edit().putString(KEY_CATALOG_ACTIVE_DEVICE, deviceKey).apply();
                    String cachedKey = prefs.getString(KEY_LAST_API_DEVICE, null);
                    String cachedApi = prefs.getString(KEY_LAST_API, null);
                    if (deviceKey != null && deviceKey.equals(cachedKey) && cachedApi != null
                            && cachedApi.matches("[0-9]+\\.[0-9]+"))
                        prefs.edit().putString(KEY_FIRMWARE_API, cachedApi).apply();
                }
                updateConnection();
                if (connected) handler.postDelayed(drainCatalogQueue, 1200);
            }
            @Override public void inventory(List<String> installed, java.util.Map<String, String> paths,
                    boolean complete, String message) {
                if (!bleConfirmed) return;
                if (complete) {
                    JSONArray list = new JSONArray();
                    for (String app : installed) list.put(app);
                    org.json.JSONObject verifiedPaths = new org.json.JSONObject();
                    for (java.util.Map.Entry<String, String> item : paths.entrySet()) {
                        try { verifiedPaths.put(item.getKey().toLowerCase(java.util.Locale.ROOT)
                                .replaceAll("[^a-z0-9]", ""), item.getValue()); }
                        catch (org.json.JSONException ignored) { }
                    }
                    if (!verifiedPaths.toString().equals(prefs.getString(KEY_INVENTORY_PATHS, "{}")))
                        lastFapIndexAt = 0;
                    prefs.edit().putString(KEY_INVENTORY, list.toString())
                            .putString(KEY_INVENTORY_PATHS, verifiedPaths.toString())
                            .putString(KEY_INVENTORY_STATUS, "ready").apply();
                    if (!fapIndexPending && android.os.SystemClock.elapsedRealtime() - lastFapIndexAt > 60000)
                        handler.postDelayed(requestFapIndex, 2100);
                    setStatus("Flipper apps inventory: " + installed.size() + " .fap files");
                    if (shopVerifying && shopRequestToken != null) {
                        boolean found = shopVerifiedPath != null && paths.containsValue(shopVerifiedPath);
                        if (!found && shopVerifiedPath != null) {
                            for (String stored : paths.values())
                                if (shopVerifiedPath.equals(stored)) { found = true; break; }
                        }
                        shopVerifying = false;
                        shopVerificationRequested = false;
                        handler.removeCallbacks(shopVerify);
                        shopReport(found ? "done" : "failed", found ? "" : "FAP not found on Flipper");
                    }
                    if (activeCatalogJob != null && "verifying".equals(activeCatalogJob.phase)) {
                        boolean found = paths.containsKey(catalogKey(activeCatalogJob.id));
                        // Updates to a renamed/aliased FAP reuse the original verified
                        // path: verifying the exact destination is authoritative.
                        if (!found && activeCatalogPath != null)
                            found = paths.containsValue(activeCatalogPath);
                        // Paths may contain unnormalized aliases from custom firmware builds.
                        if (!found) for (String alias : paths.keySet())
                            if (catalogKey(alias).equals(catalogKey(activeCatalogJob.id))) { found = true; break; }
                        finishCatalogJob(found, found ? null : "Flipper did not list the installed app");
                    }
                } else {
                    // Failure to list apps does not invalidate the firmware's separately verified API.
                    prefs.edit().remove(KEY_INVENTORY).remove(KEY_INVENTORY_PATHS)
                            .putString(KEY_INVENTORY_STATUS, "unavailable").apply();
                    if (message != null) setStatus(message);
                    if (shopVerifying) {
                        shopVerifying = false;
                        shopVerificationRequested = false;
                        handler.removeCallbacks(shopVerify);
                        shopReport("failed", "Could not verify FAP on Flipper");
                    }
                    if (activeCatalogJob != null && "verifying".equals(activeCatalogJob.phase))
                        finishCatalogJob(false, "Could not verify the Flipper app inventory");
                }
            }
            @Override public void storageInfo(String path, long total, long free, String error) {
                if (!bleConfirmed || (!"/int".equals(path) && !"/ext".equals(path))) return;
                String totalKey = "/int".equals(path) ? KEY_STORAGE_INT_TOTAL : KEY_STORAGE_EXT_TOTAL;
                String freeKey = "/int".equals(path) ? KEY_STORAGE_INT_FREE : KEY_STORAGE_EXT_FREE;
                String errorKey = "/int".equals(path) ? KEY_STORAGE_INT_ERROR : KEY_STORAGE_EXT_ERROR;
                if (error == null && total > 0 && free >= 0 && free <= total)
                    prefs.edit().putLong(totalKey, total).putLong(freeKey, free).remove(errorKey).apply();
                else prefs.edit().remove(totalKey).remove(freeKey)
                        .putString(errorKey, error == null ? "Storage unavailable" : error).apply();
                Intent event = new Intent(EVENT_STORAGE_INFO).setPackage(getPackageName())
                        .putExtra("path", path);
                if (error != null) event.putExtra("error", error);
                sendBroadcast(event);
            }
            @Override public void files(String path, List<FlipperRpc.Item> items, boolean complete, String message) {
                Intent event = new Intent(EVENT_FILES).setPackage(getPackageName());
                event.putExtra("path", path);
                event.putExtra("ok", complete);
                if (message != null) event.putExtra("message", message);
                String[] names = new String[items.size()];
                boolean[] directories = new boolean[items.size()];
                long[] sizes = new long[items.size()];
                for (int i = 0; i < items.size(); i++) {
                    names[i] = items.get(i).name;
                    directories[i] = items.get(i).directory;
                    sizes[i] = items.get(i).size;
                }
                event.putExtra("names", names).putExtra("directories", directories).putExtra("sizes", sizes);
                sendBroadcast(event);
            }
            @Override public void fileData(String path, byte[] bytes, String error) {
                if (REMOTE_INSTALL_READY.equals(path)) {
                    onRemoteInstallReady(bytes);
                    return;
                }
                if (REMOTE_INSTALL_HOME.equals(path)) {
                    if (activeCatalogJob != null &&
                            "awaiting_home".equals(activeCatalogJob.phase))
                        onRemoteInstallHome(bytes);
                    else onNativeShopHome(bytes);
                    return;
                }
                if (FAP_INDEX_RESULT.equals(path)) { onFapIndex(bytes); return; }
                if (NetworkMailbox.REQUEST.equals(path)) {handleNetworkRequest(bytes); return;}
                if (RING_SEARCH_REQ.equals(path)) {handleRingtoneSearch(bytes); return;}
                if (RING_FETCH_REQ.equals(path)) {handleRingtoneFetch(bytes); return;}
                if (CLI_OUT.equals(path)) {
                    if (bytes != null) onBleCliOutput(bytes);
                    return;
                }
                if (FW_CHECK_REQ.equals(path)) {
                    if(bytes != null && bytes.length > 0 && bytes.length < 48) {
                        String token=new String(bytes,java.nio.charset.StandardCharsets.US_ASCII).trim();
                        if(token.matches("[0-9]{1,12}-[0-9]{1,12}") &&
                                !token.equals(lastFirmwareCheckToken)) {
                            lastFirmwareCheckToken=token;
                            final String request=token;
                            network.execute(() -> {
                                String result;
                                try {
                                    WeaveUpdates.Release release=WeaveUpdates.latest(true);
                                    result=request+"|ok|"+release.version;
                                } catch(Exception e) {result=request+"|error|release unavailable";}
                                final byte[] data=result.getBytes(java.nio.charset.StandardCharsets.US_ASCII);
                                handler.post(() -> writeFirmwareCheckResult(data,0));
                            });
                        }
                    }
                    return;
                }
                if (TEXT_INPUT_REQUEST.equals(path)) {
                    onRemoteTextInputRead(bytes, error);
                    return;
                }
                if (FIND_DEVICE_FILE.equals(path)) {
                    if (bytes == null || bytes.length <= 48)
                        onFindDeviceRequestRead(bytes, error);
                    return;
                }
                if (SHOP_REQUEST.equals(path)) {
                    if (bytes != null && bytes.length > 0 && bytes.length < 130)
                        handleShopRequest(new String(bytes, java.nio.charset.StandardCharsets.UTF_8).trim());
                    return;
                }
                Intent reply = new Intent(EVENT_FILE_DATA).setPackage(getPackageName());
                reply.putExtra("path", path);
                if (bytes != null) reply.putExtra("bytes", bytes);
                if (error != null) reply.putExtra("error", error);
                sendBroadcast(reply);
            }
            @Override public void screen(byte[] framebuffer) {
                // Coalescing: ne pas perdre la derniere frame apres un input.
                if (framebuffer == null || framebuffer.length != 1024 ||
                        prefs == null || !prefs.getBoolean("screen_requested", false)) return;
                pendingScreenFrame = framebuffer;
                if (screenDeliveryScheduled) return;
                long elapsed = android.os.SystemClock.uptimeMillis() - lastScreenBroadcastAt;
                screenDeliveryScheduled = true;
                handler.postDelayed(deliverScreenFrame, Math.max(0, 65 - elapsed));
            }
            @Override public void operation(String operation, boolean success, String message) {
                if (shopAckCleanupRunning && "Delete".equals(operation)) {
                    shopAckCleanupRunning = false;
                    // Removal is best-effort. On an RPC error the next session
                    // overwrites this one-shot token; do not block the queue.
                    shopAckCleanup.pollFirst();
                    handler.postDelayed(drainShopAckCleanup, 250);
                    handler.postDelayed(drainCatalogQueue, 250);
                    return;
                }
                if ("FAP Index Request".equals(operation)) {
                    if (success) handler.postDelayed(readFapIndex, 2200);
                    else fapIndexPending = false;
                    return;
                }
                if (catalogCleanupRunning && "Delete".equals(operation)) {
                    catalogCleanupRunning = false;
                    if (success || message != null && message.endsWith(" 6")) {
                        catalogPartialCleanup.pollFirst();
                        persistCatalogCleanup();
                    }
                    handler.postDelayed(drainCatalogQueue, success ? 200 : 1800);
                    return;
                }
                if (deletingBatch && "Delete".equals(operation)) {
                    if (!success || queuedDeletes.isEmpty()) {
                        deletingBatch = false;
                        queuedDeletes.clear();
                        operation = "Delete";
                    } else {
                        handler.post(() -> continueBatchDelete());
                        return;
                    }
                }
                if ("Remote Install Prepare".equals(operation)) {
                    if (!success || activeCatalogJob == null) {
                        if (activeCatalogJob != null) finishCatalogJob(false,
                                message == null ? "Cannot prepare Flipper install screen" : message);
                    } else ble.launchWeaveInstallScreen();
                    return;
                }
                if ("Remote Install Screen".equals(operation)) {
                    if (activeCatalogJob == null) return;
                    // If Weave Apps was already open, its native watcher can
                    // still accept the marker although Loader reports busy.
                    if (!success) setStatus("Flipper app launch: " +
                            (message == null ? "unavailable" : message) +
                            "; checking whether the native screen opened");
                    handler.post(checkRemoteInstallReady);
                    return;
                }
                if ("Remote Install Download Progress".equals(operation)) {
                    if (activeCatalogJob != null &&
                            "downloading".equals(activeCatalogJob.phase))
                        handler.postDelayed(showRemoteDownloadScreen, 700);
                    return;
                }
                if ("Remote Install Initial".equals(operation)) {
                    if (activeCatalogJob != null && "preparing".equals(activeCatalogJob.phase)) {
                        if (success) ble.mkdir(activeCatalogPath.substring(0,
                                activeCatalogPath.lastIndexOf('/')));
                        else finishCatalogJob(false, message);
                    }
                    return;
                }
                if ("Remote Install Final".equals(operation)) {
                    // Retry boundedly on a transient failure, unless this is no longer
                    // the same device/session. Do not overwrite a newer job's status.
                    if (success || android.os.SystemClock.elapsedRealtime() -
                            remoteInstallFinalQueuedAt > 30000L)
                        pendingRemoteInstallFinal = null;
                    // No fixed 3.8-second guess: wait until the native UI has
                    // completed its bar and acknowledged that it has closed.
                    if (success && activeCatalogJob != null &&
                            "awaiting_home".equals(activeCatalogJob.phase))
                        handler.postDelayed(checkRemoteInstallHome, 450);
                    else handler.postDelayed(drainCatalogQueue, 1100);
                    return;
                }
                if ("New folder".equals(operation) && activeCatalogJob != null &&
                        "preparing".equals(activeCatalogJob.phase)) {
                    if (activeCatalogJob.cancelRequested) finishCatalogJob(false, "Cancelled");
                    else if (success || (message != null && message.endsWith(" 6"))) {
                        activeCatalogJob.phase = "uploading";
                        activeCatalogJob.percent = 0;
                        activeCatalogJob.etaSeconds = -1;
                        persistCatalogQueue("uploading", activeCatalogJob.id);
                        ble.upload(activeCatalogStagePath, activeCatalogBytes);
                    } else finishCatalogJob(false, message);
                    return;
                }
                if ("Install".equals(operation) && activeCatalogJob != null &&
                        ("uploading".equals(activeCatalogJob.phase) ||
                         "cancelling".equals(activeCatalogJob.phase))) {
                    if (!success || activeCatalogJob.cancelRequested) {
                        finishCatalogJob(false, activeCatalogJob.cancelRequested ? "Cancelled" : message);
                    } else if (activeCatalogHadExisting) {
                        activeCatalogJob.phase = "backing_up";
                        persistCatalogQueue("backing_up", activeCatalogJob.id);
                        ble.rename(activeCatalogPath, activeCatalogBackupPath);
                    } else {
                        activeCatalogJob.phase = "committing";
                        persistCatalogQueue("committing", activeCatalogJob.id);
                        ble.rename(activeCatalogStagePath, activeCatalogPath);
                    }
                    return;
                }
                if ("Rename".equals(operation) && activeCatalogJob != null) {
                    if ("backing_up".equals(activeCatalogJob.phase)) {
                        if (!success) finishCatalogJob(false, message);
                        else if (activeCatalogJob.cancelRequested) {
                            activeCatalogJob.phase = "restoring";
                            ble.rename(activeCatalogBackupPath, activeCatalogPath);
                        } else {
                            activeCatalogJob.phase = "committing";
                            persistCatalogQueue("committing", activeCatalogJob.id);
                            ble.rename(activeCatalogStagePath, activeCatalogPath);
                        }
                        return;
                    }
                    if ("committing".equals(activeCatalogJob.phase)) {
                        if (!success && activeCatalogHadExisting) {
                            activeCatalogJob.phase = "restoring";
                            ble.rename(activeCatalogBackupPath, activeCatalogPath);
                        } else if (!success) finishCatalogJob(false, message);
                        else {
                            activeCatalogJob.phase = "verifying";
                            activeCatalogJob.percent = 100;
                            persistCatalogQueue("verifying", activeCatalogJob.id);
                            handler.postDelayed(() -> { if (bleConfirmed) ble.requestInventory(); }, 450);
                            handler.postDelayed(catalogVerifyTimeout, 20000);
                        }
                        return;
                    }
                    if ("restoring".equals(activeCatalogJob.phase)) {
                        finishCatalogJob(false, activeCatalogJob.cancelRequested ? "Cancelled" :
                                success ? "Install commit failed; prior app restored" :
                                        "Install commit failed; check the .weave-old backup");
                        return;
                    }
                }
                if ("New folder".equals(operation) && pendingInstallPath != null) {
                    String path = pendingInstallPath;
                    byte[] data = pendingInstallBytes;
                    pendingInstallPath = null;
                    pendingInstallBytes = null;
                    if (success || (message != null && message.endsWith(" 6"))) {
                        if (shopTransferActive) ble.uploadFile(path, data, "Shop Install");
                        else ble.upload(path, data);
                        return;
                    }
                    operation = shopTransferActive ? "Shop Install" : "Install";
                }
                if (ringTransferActive != null && operation.equals(ringTransferActive.operation)) {
                    RingTransfer completed = ringTransferActive;
                    ringTransferActive = null;
                    if ("Ring Fetch".equals(operation)) {
                        if (success && completed.previewName != null)
                            queueRingtoneTransfer("Ring Preview", RING_PREVIEW_REQ,
                                    completed.previewName.getBytes(java.nio.charset.StandardCharsets.US_ASCII),null);
                        queueRingtoneTransfer("Ring Result", RING_FETCH_RESULT,
                                (success ? "ok|" : "error|").concat(
                                    success ? completed.path.substring(completed.path.lastIndexOf('/')+1) :
                                    "transfer failed").getBytes(java.nio.charset.StandardCharsets.US_ASCII),null);
                    }
                    if (!success) setStatus("Ringtone RPC failed: " + operation);
                    handler.postDelayed(ringDrain, 200);
                    return;
                }
                if ("BLE CLI".equals(operation)) {
                    if (!success) emitCli("\n[BLE CLI command failed: " +
                            (message == null ? "RPC rejected request" : message) + "]\n");
                    if (!bleCliCommands.isEmpty()) handler.postDelayed(sendPendingBleCli, 100);
                    return;
                }
                if ("Shop Install".equals(operation)) {
                    shopTransferActive = false;
                    if (shopRequestToken != null) {
                        if (success) {
                            shopVerifying = true;
                            shopVerificationRequested = false;
                            shopProgress("installing", 99, -1, 0, shopVerifiedPath);
                            handler.postDelayed(shopVerify, 600);
                            handler.postDelayed(() -> {
                                if (shopVerifying) {
                                    shopVerifying = false;
                                    shopVerificationRequested = false;
                                    handler.removeCallbacks(shopVerify);
                                    shopReport("failed", "Flipper installation not verified");
                                }
                            }, 45000);
                        } else shopReport("failed", message);
                    }
                }
                if ("Shop Result".equals(operation)) {
                    if (!success) {
                        setStatus("Native Shop: unable to update install status");
                        if(shopStatusInFlight != null && pendingShopStatus == null)
                            pendingShopStatus = shopStatusInFlight;
                    } else {
                        if(shopStatusInFlight != null && shopRequestToken != null &&
                                shopStatusInFlight.startsWith(shopRequestToken + "|")) {
                            final String tail = shopStatusInFlight.substring(shopRequestToken.length()+1);
                            if(tail.startsWith("done|") || tail.startsWith("failed|")) {
                                prefs.edit().putString(SHOP_LAST_REQUEST, shopRequestToken).apply();
                                // A failed native install also closes its view
                                // and sends HOME. Poll both outcomes.
                                if (nativeShopHomeToken != null)
                                    handler.postDelayed(checkNativeShopHome, 500);
                            }
                        }
                        shopStatusInFlight = null;
                    }
                    handler.postDelayed(shopProcess, success ? 350 : 1800);
                }
                if ("Shop Sync".equals(operation)) {
                    if (!success) setStatus("Native Shop catalog sync failed; retry from Apps > Refresh");
                    handler.postDelayed(shopProcess, 600);
                }
                Intent event = new Intent(EVENT_OPERATION).setPackage(getPackageName());
                event.putExtra("operation", operation).putExtra("ok", success);
                if (message != null) event.putExtra("message", message);
                sendBroadcast(event);
                if (success && ("Install".equals(operation) || "Delete".equals(operation))) {
                    // The inventory is the source of truth, including externally removed FAPs.
                    handler.postDelayed(() -> ble.requestInventory(), 250);
                }
                if (success && ("Install".equals(operation) || "Delete".equals(operation)
                        || "Import".equals(operation) || "Shop Install".equals(operation)
                        || "New folder".equals(operation) || "Rename".equals(operation)))
                    handler.postDelayed(() -> ble.requestStorageInfo("/ext"), 650);
            }
            @Override public void progress(String operation, int percent) {
                if ("Shop Install".equals(operation) && shopTransferActive && shopRequestToken != null) {
                    // BLE RPC reports bytes acknowledged. Preserve 100% for inventory verification.
                    int bounded = Math.min(99, Math.max(0, percent));
                    long elapsed = android.os.SystemClock.elapsedRealtime() - shopPhaseStartTime;
                    long eta = bounded > 1 && elapsed > 0 ?
                            ((long)(100 - bounded) * elapsed) / (bounded * 1000L) : -1;
                    shopProgress("installing", bounded, eta, 0, shopVerifiedPath);
                }
                if ("Install".equals(operation) && activeCatalogJob != null &&
                        "uploading".equals(activeCatalogJob.phase)) {
                    activeCatalogJob.percent = Math.max(0, Math.min(99, percent));
                    persistCatalogQueue("progress", activeCatalogJob.id);
                }
                sendBroadcast(new Intent(EVENT_PROGRESS).setPackage(getPackageName())
                        .putExtra("operation", operation).putExtra("percent", percent));
            }
            @Override public void information(String field, String value) {
                if (!bleConfirmed) return;
                if ("battery".equals(field)) prefs.edit().putString(KEY_BATTERY, value).apply();
                else if ("firmware".equals(field)) prefs.edit().putString(KEY_FIRMWARE, value).apply();
                else if ("name".equals(field)) prefs.edit().putString(KEY_DEVICE_NAME, value).apply();
                else if ("api_version".equals(field) && value != null && value.matches("[0-9]+\\.[0-9]+")) {
                    String deviceKey = ble.connectedDeviceKey();
                    if (deviceKey != null)
                        prefs.edit().putString(KEY_FIRMWARE_API, value).putString(KEY_LAST_API, value)
                                .putString(KEY_LAST_API_DEVICE, deviceKey).apply();
                }
            }
        });
        try {
            if (Build.VERSION.SDK_INT >= 33)
                registerReceiver(bluetoothStateReceiver, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED), Context.RECEIVER_NOT_EXPORTED);
            else registerReceiver(bluetoothStateReceiver, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));
            bluetoothReceiverRegistered = true;
        } catch (RuntimeException ignored) { }
        try {
            IntentFilter usbFilter = new IntentFilter();
            usbFilter.addAction(ACTION_USB_PERMISSION);
            usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_ATTACHED);
            usbFilter.addAction(UsbManager.ACTION_USB_DEVICE_DETACHED);
            if (Build.VERSION.SDK_INT >= 33)
                registerReceiver(usbEvents, usbFilter, Context.RECEIVER_EXPORTED);
            else registerReceiver(usbEvents, usbFilter);
            usbReceiverRegistered = true;
        } catch (RuntimeException error) {
            setStatus("USB event receiver unavailable");
        }
        handler.postDelayed(bridgePoll, 900);
        handler.postDelayed(networkHeartbeat, 1400);
        // Offline ringtone library: do not poll legacy GitHub search/fetch requests.
        prefs.edit().putBoolean(KEY_ACTIVE, false).putBoolean(KEY_CONNECTED, false)
                .putBoolean(KEY_WV1, false).remove(KEY_TRANSPORT)
                .remove(KEY_BATTERY).remove(KEY_FIRMWARE).remove(KEY_DEVICE_NAME)
                .remove(KEY_INVENTORY).remove(KEY_INVENTORY_PATHS).remove(KEY_FIRMWARE_API)
                .remove(KEY_STORAGE_INT_TOTAL).remove(KEY_STORAGE_INT_FREE)
                .remove(KEY_STORAGE_EXT_TOTAL).remove(KEY_STORAGE_EXT_FREE)
                .remove(KEY_STORAGE_INT_ERROR).remove(KEY_STORAGE_EXT_ERROR)
                .putString(KEY_INVENTORY_STATUS, "unavailable").apply();
    }

    @Override public int onStartCommand(Intent intent, int flags, int startId) {
        if (!prefs.getBoolean(KEY_ENABLED, false)) {
            stopSelf();
            return START_NOT_STICKY;
        }
        if (!prefs.getBoolean(KEY_ACTIVE, false)) {
            createChannel();
            try {
                startForeground(NOTIFICATION_ID, notification("Searching for Flipper"));
            } catch (RuntimeException error) {
                setStatus("Foreground service unavailable: " + error.getClass().getSimpleName());
                prefs.edit().putBoolean(KEY_ENABLED, false).apply();
                stopSelf();
                return START_NOT_STICKY;
            }
            prefs.edit().putBoolean(KEY_ACTIVE, true).apply();
            setStatus("Service started — searching for Flipper");
            handler.removeCallbacks(reconnect);
            handler.post(reconnect);
            handler.postDelayed(drainCatalogQueue, 1800);
        }
        String action = intent == null ? ACTION_START : intent.getAction();
        if(ACTION_TEXT_INPUT_SEND.equals(action) || ACTION_TEXT_INPUT_PREVIEW.equals(action)) {
            sendRemoteTextInput(intent);
            return START_STICKY;
        }
        if(ACTION_ALERT_EVENT.equals(action)) {
            if (intent != null &&
                    (prefs.getBoolean("alerts_bridge_enabled", false) ||
                     "call_end".equals(intent.getStringExtra("type")))) {
                String alert = intent.getStringExtra("type");
                if ("call".equals(alert) || "sms".equals(alert) || "call_end".equals(alert)) {
                    final String token;
                    final String code;
                    if("call".equals(alert)) {
                        if(!bleConfirmed) return START_STICKY;
                        // One logical call => one start token, used again for stop.
                        String existing = prefs.getString("active_call_alert_token", "");
                        if(!existing.isEmpty()) {
                            try {
                                if(System.currentTimeMillis()-Long.parseLong(existing) < 180000L)
                                    return START_STICKY;
                            } catch(NumberFormatException ignored) { }
                            // A dead service must not block future calls forever.
                            prefs.edit().remove("active_call_alert_token").apply();
                        }
                        token = Long.toString(System.currentTimeMillis());
                        prefs.edit().putString("active_call_alert_token", token).apply();
                        code = "C|";
                    } else if("call_end".equals(alert)) {
                        token = prefs.getString("active_call_alert_token", "");
                        if(token.isEmpty()) return START_STICKY;
                        prefs.edit().remove("active_call_alert_token").apply();
                        code = "E|";
                    } else {
                        if(!bleConfirmed) return START_STICKY;
                        token = Long.toString(System.currentTimeMillis());
                        code = "S|";
                    }
                    if("E|".equals(code)) {
                        // If its start has not been transmitted yet, cancel it;
                        // otherwise transfer this matching stop event in order.
                        java.util.Iterator<RingTransfer> pending = ringTransfers.iterator();
                        boolean cancelledStart = false;
                        while(pending.hasNext()) {
                            RingTransfer item = pending.next();
                            if(item.data != null && ALERT_REQ.equals(item.path) &&
                                    ("C|" + token).equals(new String(item.data,
                                    java.nio.charset.StandardCharsets.US_ASCII))) {
                                pending.remove();
                                cancelledStart = true;
                            }
                        }
                        if(cancelledStart && ringTransferActive == null) return START_STICKY;
                    }
                    queueRingtoneTransfer("E|".equals(code) ? "Ring Call End" : "Ring Alert",
                            ALERT_REQ, (code + token).getBytes(
                            java.nio.charset.StandardCharsets.US_ASCII), null);
                }
            }
            return START_STICKY;
        }
        if(ACTION_STOP_FIND_DEVICE.equals(action)) {
            stopDeviceRingNow();
            return START_STICKY;
        }
        if (ACTION_USB.equals(action)) {
            lastUsbPermissionTime = 0;
            lastUsbAttempt = System.currentTimeMillis();
            attemptUsb();
        } else if (ACTION_SCAN.equals(action)) {
            if (hasBlePermission()) {
                lastBleScan = System.currentTimeMillis();
                if (!ble.isConnected() && !ble.isConnecting()) ble.scan();
                else if (ble.isConnected()) ble.refreshInformation();
            } else setStatus("Bluetooth permission required");
        } else if (ACTION_LAUNCH_APP.equals(action)) {
            if (remoteInputLocked()) {
                setStatus("Wait for the Flipper installation to finish before launching apps");
                return START_STICKY;
            }
            if (intent != null && bleConfirmed) {
                String path = intent.getStringExtra("path");
                if (path != null && path.startsWith("/ext/apps/") && path.endsWith(".fap")
                        && !path.contains("..")) ble.launch(path);
                else setStatus("App launch rejected: unverified FAP path");
            } else setStatus("Launch needs Flipper BLE RPC");
        } else if (ACTION_REFRESH_APPS.equals(action)) {
            if (intent != null && intent.getBooleanExtra("forceMetadata", false)) lastFapIndexAt = 0;
            if (!bleConfirmed) setStatus("App inventory needs Flipper BLE RPC connection");
            else if (!remoteInputLocked()) {
                prefs.edit().putString(KEY_INVENTORY_STATUS, "loading").apply();
                ble.requestInventory();
            } // The next completed transfer automatically refreshes verified inventory.

        } else if (ACTION_CONNECT_BLE.equals(action)) {
            connectBle();
        } else if (ACTION_START_SCREEN.equals(action)) {
            // The Activity may have moved away before this Intent was handled.
            if (prefs.getBoolean("screen_requested", false) && !remoteInputLocked())
                ble.startScreen();
        } else if (ACTION_RECOVER_SCREEN.equals(action)) {
            // The user explicitly requested a fresh Home framebuffer. Restart
            // only GUI streaming, never the firmware or an active FAP transfer.
            if (prefs.getBoolean("screen_requested", false) &&
                    !remoteInputLocked() && ble != null && ble.isConnected()) {
                handler.removeCallbacks(deliverScreenFrame);
                pendingScreenFrame = null;
                screenDeliveryScheduled = false;
                ble.recoverScreen();
            }
        } else if (ACTION_STOP_SCREEN.equals(action)) {
            // Drop a coalesced frame queued before the page was hidden.
            handler.removeCallbacks(deliverScreenFrame);
            pendingScreenFrame = null;
            screenDeliveryScheduled = false;
            ble.stopScreen();
            // Fast Home -> other tab -> Home switches may already have been
            // processed before this stale STOP action. Reconcile the latest
            // desired state without waiting for another navigation event.
            if (prefs.getBoolean("screen_requested", false) && !remoteInputLocked())
                ble.startScreen();
        } else if (ACTION_PRESS_KEY.equals(action)) {
            if (remoteInputLocked()) return START_STICKY;
            if (intent != null) {
                int key = intent.getIntExtra("key", -1);
                if (key >= 0 && key <= 5) {
                    lastRemoteInputAt = android.os.SystemClock.uptimeMillis();
                    ble.pressKey(key);
                }
            }
        } else if (ACTION_KEY_EVENT.equals(action)) {
            if (intent != null) {
                int key = intent.getIntExtra("key", -1);
                int type = intent.getIntExtra("type", -1);
                /* RELEASE peut terminer une touche appuyee juste avant le lock. */
                if (key >= 0 && key <= 5 && type >= 0 && type <= 4 &&
                        (!remoteInputLocked() || type == 1)) {
                    lastRemoteInputAt = android.os.SystemClock.uptimeMillis();
                    ble.inputEvent(key, type);
                }
            }
        } else if (ACTION_ENTER_DFU.equals(action)) {
            if (remoteInputLocked() || ble == null || !ble.enterFirmwareRecovery()) {
                setStatus("DFU unavailable: connect Flipper RPC and finish ongoing transfers");
            } else {
                recoveryReconnectPauseUntil = android.os.SystemClock.uptimeMillis() + 120000L;
                setStatus("Flipper entering DFU. Connect USB to qFlipper for recovery.");
            }
        } else if (ACTION_FIND_FLIPPER.equals(action)) {
            // Les commandes Finder ne sont plus perdues quand le RPC est occupe.
            if (pendingFindFlipperToken == null) {
                pendingFindFlipperToken = Long.toString(System.currentTimeMillis());
                findFlipperDeadline = android.os.SystemClock.uptimeMillis() + 30000;
            }
            setStatus("Find my Flipper queued until RPC is available");
            handler.post(sendPendingFindFlipper);
        } else if (ACTION_LONG_BACK.equals(action)) {
            if (remoteInputLocked()) return START_STICKY;
            lastRemoteInputAt = android.os.SystemClock.uptimeMillis();
            ble.longBack();
        } else if (ACTION_QUEUE_CATALOG_APP.equals(action)) {
            enqueueCatalogApp(intent);
        } else if (ACTION_CANCEL_QUEUED_APP.equals(action)) {
            cancelQueuedCatalogApp(intent == null ? null : intent.getStringExtra("id"));
        } else if (ACTION_LIST_FILES.equals(action)) {
            if (intent != null) {
                String browse = intent.getStringExtra("path");
                if (remoteInputLocked()) {
                    if (browse != null && (browse.startsWith("/ext") || browse.startsWith("/int"))
                            && !browse.contains("..")) deferredCatalogFilesPath = browse;
                } else ble.listFiles(browse);
            }
        } else if (ACTION_STORAGE_INFO.equals(action)) {
            if (intent != null && !remoteInputLocked()) ble.requestStorageInfo(intent.getStringExtra("path"));
        } else if (ACTION_MKDIR.equals(action)) {
            if (remoteInputLocked()) setStatus("Flipper is busy with an app installation");
            else if (intent != null) ble.mkdir(intent.getStringExtra("path"));
        } else if (ACTION_RENAME.equals(action)) {
            if (remoteInputLocked()) setStatus("Flipper is busy with an app installation");
            else if (intent != null) ble.rename(intent.getStringExtra("path"), intent.getStringExtra("newPath"));
        } else if (ACTION_DELETE_FILE.equals(action)) {
            if (remoteInputLocked()) {
                sendBroadcast(new Intent(EVENT_OPERATION).setPackage(getPackageName())
                        .putExtra("operation", "Delete").putExtra("ok", false)
                        .putExtra("message", "Wait for the app installation queue to finish"));
            } else if (intent != null && !deletingBatch)
                ble.delete(intent.getStringExtra("path"), intent.getBooleanExtra("recursive", false));
        } else if (ACTION_DELETE_BATCH.equals(action)) {
            if (remoteInputLocked()) {
                setStatus("Flipper is busy with an app installation");
                return START_STICKY;
            }
            if (intent != null && !deletingBatch) {
                String[] paths = intent.getStringArrayExtra("paths");
                if (paths != null && paths.length > 0 && paths.length <= 100) {
                    for (String path : paths) {
                        if (path == null || !(path.startsWith("/ext/") || path.startsWith("/int/"))
                                || path.contains("/../") || path.endsWith("/..")) {
                            queuedDeletes.clear();
                            return START_STICKY;
                        }
                        queuedDeletes.add(path);
                    }
                    deletingBatch = true;
                    continueBatchDelete();
                }
            }
        } else if (ACTION_CLI_START.equals(action)) {
            if (bleConfirmed && remoteInputLocked()) {
                emitCli("\n[Flipper installation in progress; retry CLI after completion]\n");
                return START_STICKY;
            }
            if (bleConfirmed && ble.isRpcReady()) {
                if (bleCliActive) return START_STICKY;
                bleCliActive = true;
                bleCliReady = false;
                previousBleCliOutput = new byte[0];
                bleCliCommands.clear();
                bleCliStartAttempts = 0;
                emitCli("\n[Connecting Flipper CLI over Bluetooth…]\n");
                handler.removeCallbacks(bleCliPoll);
                handler.removeCallbacks(bleCliBootstrap);
                handler.post(bleCliBootstrap);
                handler.postDelayed(bleCliPoll, 1800);
            } else {
                UsbDevice device = FlipperUsbCli.detect(this);
                if (device == null) setStatus("Connect Flipper over Bluetooth or USB for CLI");
                else cli.connect(device);
            }
        } else if (ACTION_CLI_SEND.equals(action)) {
            if (remoteInputLocked()) {
                emitCli("\n[CLI blocked while installing on Flipper]\n");
                return START_STICKY;
            }
            String line = intent == null ? null : intent.getStringExtra("command");
            if (line != null && !line.trim().isEmpty() && line.length() <= 180 &&
                    line.indexOf('\r') < 0 && line.indexOf('\n') < 0) {
                if (bleCliActive && bleConfirmed) {
                    if (bleCliCommands.size() >= 12) emitCli("\n[CLI queue full; retry command]\n");
                    else {
                        bleCliCommands.addLast(line.trim());
                        handler.post(sendPendingBleCli);
                    }
                } else if (!cli.send(line)) setStatus("Flipper CLI busy or disconnected");
            }
        } else if (ACTION_CLI_STOP.equals(action)) {
            bleCliActive = false;
            bleCliReady = false;
            handler.removeCallbacks(bleCliPoll);
            handler.removeCallbacks(bleCliBootstrap);
            handler.removeCallbacks(sendPendingBleCli);
            bleCliCommands.clear();
            emitCli("\n[BLE CLI disconnected]\n");
            cli.close();
        } else if (ACTION_READ_FILE.equals(action)) {
            if (remoteInputLocked()) setStatus("File read deferred until Flipper installation completes");
            else if (intent != null) ble.readFile(intent.getStringExtra("path"));
        } else if (ACTION_UPLOAD_FILE.equals(action)) {
            if (remoteInputLocked()) {
                setStatus("File upload unavailable during Flipper app installation");
                return START_STICKY;
            }
            if (intent != null) {
                String filename = intent.getStringExtra("filename");
                String path = intent.getStringExtra("path");
                if (filename != null && filename.matches("upload_[A-Za-z0-9_-]{1,100}\\.bin")
                        && path != null && (path.startsWith("/ext/") || path.startsWith("/int/"))) {
                    try {
                        java.io.File cache = new java.io.File(getCacheDir(), filename);
                        if (cache.isFile() && cache.length() > 0 && cache.length() <= 2_000_000)
                            ble.uploadFile(path, java.nio.file.Files.readAllBytes(cache.toPath()), "Import");
                        else setStatus("File missing or exceeds 2 MiB");
                    } catch (java.io.IOException error) { setStatus("Unable to read selected file"); }
                }
            }
        } else if (ACTION_UPLOAD_FAP.equals(action)) {
            if (!catalogQueue.isEmpty()) {
                setStatus("An app installation is queued; finish it before manually importing a FAP");
                return START_STICKY;
            }
            if (intent != null) {
                final String cacheName = intent.getStringExtra("filename");
                final String target = intent.getStringExtra("path");
                if (cacheName != null && cacheName.matches("fap_[a-zA-Z0-9_-]{1,100}\\.bin")
                        && target != null && target.startsWith("/ext/apps/") && target.endsWith(".fap")) {
                    try {
                        java.io.File file = new java.io.File(getCacheDir(), cacheName);
                        if (file.isFile() && file.length() > 0 && file.length() <= 2_000_000) {
                            byte[] bytes = java.nio.file.Files.readAllBytes(file.toPath());
                            pendingInstallBytes = bytes;
                            pendingInstallPath = target;
                            ble.mkdir(target.substring(0, target.lastIndexOf('/')));
                        } else setStatus("FAP file is missing or too large");
                    } catch (java.io.IOException error) { setStatus("Could not read downloaded FAP"); }
                }
            }
        } else if (ACTION_SYNC_SHOP.equals(action)) {
            String data = intent == null ? null : intent.getStringExtra("catalog");
            if (data != null && !data.isEmpty() && data.length() <= 50000 && !data.contains("\0")) {
                pendingShopCatalog = data;
                handler.postDelayed(shopProcess, 3000);
            }
        } else if (ACTION_DISCONNECT.equals(action)) {
            usb.close(); ble.close(); cli.close(); usbConfirmed = false; bleConfirmed = false;
            updateConnection();
            setStatus("Disconnected — reconnecting while service is enabled");
        }
        return START_STICKY;
    }

    /** Persist and broadcast the same job state for Activity and notification. */
    private void shopProgress(String phase, int percent, long eta, long total, String target) {
        if(shopRequestToken == null || stopping) return;
        percent = percent < 0 ? -1 : Math.min(
                "installing".equals(phase) ? 99 : 100, Math.max(0, percent));
        final boolean waitingNativeHome = "done".equals(phase) && nativeShopHomeToken != null;
        String visiblePhase = waitingNativeHome ? "installing" : phase;
        int visiblePercent = waitingNativeHome ? -1 : percent;
        shopPhase = visiblePhase;
        shopPercent = visiblePercent;
        shopEta = eta;
        try {
            org.json.JSONObject job = new org.json.JSONObject();
            job.put("id", shopRequestToken.substring(0, shopRequestToken.indexOf('|')));
            job.put("title", shopTitle);
            job.put("phase", visiblePhase); job.put("percent", visiblePercent);
            job.put("eta", eta); job.put("token", shopRequestToken);
            job.put("updatedAt", System.currentTimeMillis());
            prefs.edit().putString(KEY_SHOP_JOB, job.toString()).apply();
        } catch(org.json.JSONException ignored) { }
        sendBroadcast(new Intent(EVENT_SHOP_PROGRESS).setPackage(getPackageName())
                .putExtra("id", shopRequestToken.substring(0, shopRequestToken.indexOf('|')))
                .putExtra("phase", visiblePhase).putExtra("percent", visiblePercent)
                .putExtra("eta", eta));
        showShopNotification(visiblePhase, visiblePercent, eta);
        // The Flipper can only read statuses when Storage RPC is idle. Keep
        // the latest state; while uploading it measures actual SD file growth.
        if(!"installing".equals(phase) || (!shopTransferActive && !shopVerifying)) {
            String pct = percent < 0 ? "-" : Integer.toString(percent);
            String seconds = eta < 0 ? "-" : Long.toString(Math.min(eta, 35999));
            String bytes = total > 0 ? Long.toString(total) : "-";
            String path = target == null ? "-" : target;
            pendingShopStatus = shopRequestToken + "|" + phase + "|" + pct + "|" +
                    seconds + "|" + bytes + "|" + path;
            handler.postDelayed(shopProcess, 250);
        }
    }

    /** Use only directory names visible in the native Installed FAPs browser. */
    private static String shopCanonicalCategory(String category) {
        String key = category == null ? "" : category.toLowerCase(java.util.Locale.ROOT).trim();
        if (key.contains("usb")) return "USB";
        if (key.contains("nfc")) return "NFC";
        if (key.contains("infrared") || key.equals("ir")) return "Infrared";
        if (key.contains("sub-ghz") || key.contains("subghz") || key.contains("radio")) return "Sub-GHz";
        if (key.contains("rfid") || key.contains("125khz")) return "RFID";
        if (key.contains("bluetooth") || key.equals("ble")) return "Bluetooth";
        if (key.contains("gpio")) return "GPIO";
        if (key.contains("ibutton") || key.contains("1-wire")) return "iButton";
        if (key.contains("game")) return "Games";
        if (key.contains("tool") || key.contains("utilit")) return "Tools";
        return "Other";
    }

    /** The device sends an ID+token marker; this foreground service owns the job. */
    private void handleShopRequest(String request) {
        if(request == null || !request.matches("[A-Za-z0-9_-]{2,80}\\|[0-9]{1,12}-[0-9]{1,12}")) return;
        if(request.equals(shopRequestToken) ||
                request.equals(prefs.getString(SHOP_LAST_REQUEST, "")) ||
                shopNetworkBusy || shopTransferActive || shopVerifying) return;
        final String alias = request.substring(0, request.indexOf('|'));
        shopRequestToken = request;
        nativeShopHomeToken = null;
        nativeShopHomeOutcome = null;
        handler.removeCallbacks(checkNativeShopHome);
        shopRequestDevice = ble.connectedDeviceKey();
        final String targetDevice = shopRequestDevice;
        final String requestToken = request;
        shopTitle = alias;
        shopVerifiedPath = null;
        shopCatalogVersion = null;
        shopPhaseStartTime = android.os.SystemClock.elapsedRealtime();
        shopNetworkBusy = true;
        shopProgress("downloading", -1, -1, 0, null);
        if(shopClient == null) shopClient = new AppCatalogClient(this);
        shopClient.loadInstalledMetadata(alias, meta -> {
            if(stopping || !requestToken.equals(shopRequestToken) || targetDevice == null ||
                    !targetDevice.equals(ble.connectedDeviceKey())) {
                shopNetworkBusy = false;
                return;
            }
            final AppCatalogClient.Entry app = meta != null ? meta : new AppCatalogClient.Entry(
                    alias, alias, "Tools", null, null, null, null, null, null, null,
                    java.util.Collections.emptyList(), 0, 0);
            shopTitle = app.title == null ? alias : app.title;
            shopCatalogVersion = app.version;
            String api = prefs.getString(KEY_FIRMWARE_API, null);
            final long downloadStart = android.os.SystemClock.elapsedRealtime();
            shopClient.downloadAvailableOfficialFap(app, api, (bytes, total) -> {
                if(!requestToken.equals(shopRequestToken) || !shopNetworkBusy) return;
                long elapsed = android.os.SystemClock.elapsedRealtime() - downloadStart;
                if(total > 0 && bytes <= total) {
                    int percent = (int)Math.min(99, bytes * 100L / total);
                    long eta = bytes > 0 && elapsed > 0 ?
                            ((total - bytes) * elapsed / bytes) / 1000L : -1;
                    shopProgress("downloading", percent, eta, 0, null);
                } else shopProgress("downloading", -1, -1, 0, null);
            }, (data, publishedApi, compatible, error) -> {
                shopNetworkBusy = false;
                if(stopping || !requestToken.equals(shopRequestToken) ||
                        !targetDevice.equals(ble.connectedDeviceKey())) return;
                if(data == null || data.length < 33 || data.length > 2_000_000) {
                    shopReport("failed", error == null ? "FAP download failed" : error);
                    return;
                }
                String category = shopCanonicalCategory(app.category);
                pendingShopPath = "/ext/apps/" + category + "/" + alias + ".fap";
                shopVerifiedPath = pendingShopPath;
                // Android reaches 100% for HTTP first; Flipper keeps its ring
                // until the install phase is explicitly announced.
                shopProgress("downloading", 100, 0, 0, null);
                handler.postDelayed(() -> {
                    if (stopping || !requestToken.equals(shopRequestToken) ||
                            !targetDevice.equals(ble.connectedDeviceKey())) return;
                    pendingShopFap = data;
                    shopPhaseStartTime = android.os.SystemClock.elapsedRealtime();
                    // Give the Flipper the exact target before occupying BLE RPC
                    // with the FAP upload, then animate its physical install bar.
                    shopProgress("installing", 0, -1, data.length, pendingShopPath);
                }, 240);
            });
        });
    }

    private void shopReport(String state, String reason) {
        if(shopRequestToken == null) return;
        if("done".equals(state) || "failed".equals(state)) {
            // Both terminal outcomes must wait for the Flipper to close its
            // native animation/error view. Never leave a failed request orphaned.
            nativeShopHomeToken = shopRequestToken;
            nativeShopHomeOutcome = state;
            nativeShopHomeDeadline = android.os.SystemClock.elapsedRealtime() + 150000L;
            if("failed".equals(state))
                setStatus("Native Shop failed: " +
                        (reason == null || reason.isEmpty() ? "Unknown error" : reason));
            shopProgress(state, "done".equals(state) ? 100 : -1, 0, 0, null);
        } else shopProgress(state, -1, -1, 0, null);
    }

    private void continueBatchDelete() {
        if (!deletingBatch || queuedDeletes.isEmpty()) return;
        ble.delete(queuedDeletes.removeFirst(), false);
    }

    private void attemptUsb() {
        if (stopping || usbConfirmed || usb.isOpen() || cli.isRunning() ||
                "Bluetooth".equals(prefs.getString(KEY_MODE, "Auto"))) return;
        List<UsbDevice> candidates = usb.discover();
        if (candidates.isEmpty()) return;
        UsbDevice device = candidates.get(0);
        if (!usb.getManager().hasPermission(device)) {
            long now = System.currentTimeMillis();
            if (device.getDeviceName().equals(lastUsbPermissionDevice) &&
                    now - lastUsbPermissionTime < 90000) return;
            lastUsbPermissionDevice = device.getDeviceName();
            lastUsbPermissionTime = now;
            try {
                PendingIntent permission = PendingIntent.getBroadcast(this, 9,
                        new Intent(ACTION_USB_PERMISSION).setPackage(getPackageName()),
                        PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_MUTABLE);
                usb.getManager().requestPermission(device, permission);
                setStatus("Flipper USB detected — approve USB access on your phone");
            } catch (RuntimeException error) {
                setStatus("Cannot request Flipper USB permission — using Bluetooth");
            }
            return;
        }
        if (!usb.connect(device)) setStatus("Flipper USB CDC unavailable — using Bluetooth");
    }

    private boolean hasBlePermission() {
        if (Build.VERSION.SDK_INT >= 31) {
            return checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED
                    && checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) == PackageManager.PERMISSION_GRANTED;
        }
        return Build.VERSION.SDK_INT < 23 ||
                checkSelfPermission(Manifest.permission.ACCESS_FINE_LOCATION) == PackageManager.PERMISSION_GRANTED;
    }

    private void connectBle() {
        if (!hasBlePermission()) { setStatus("Bluetooth permission required"); return; }
        if (foundBle == null) { setStatus("Flipper Bluetooth device not found — check pairing"); return; }
        if (usbConfirmed) return;
        ble.connect(foundBle);
    }

    private void updateConnection() {
        if (stopping) return;
        String transport = usbConfirmed ? "USB" : (bleConfirmed ? "Bluetooth" : "");
        prefs.edit().putBoolean(KEY_CONNECTED, !transport.isEmpty())
                .putBoolean(KEY_WV1, false)
                .putString(KEY_TRANSPORT, transport).apply();
        NotificationManager manager = getSystemService(NotificationManager.class);
        if (manager != null && prefs.getBoolean(KEY_ACTIVE, false))
            manager.notify(NOTIFICATION_ID, notification(transport.isEmpty()
                    ? "Searching for Flipper" : (usbConfirmed ? "Flipper USB protobuf RPC" : "Flipper Bluetooth protobuf RPC")));
    }

    private synchronized void setStatus(String message) {
        if (stopping || message == null || message.equals(lastStatus)) return;
        lastStatus = message;
        String timestamp = new SimpleDateFormat("HH:mm:ss", Locale.US).format(new Date());
        String old = prefs.getString(KEY_LOG, "");
        String updated = old + timestamp + "  " + message + "\n";
        if (updated.length() > 8000) updated = updated.substring(updated.length() - 8000);
        prefs.edit().putString(KEY_STATUS, message).putString(KEY_LOG, updated).apply();
    }

    private void createChannel() {
        NotificationManager manager = getSystemService(NotificationManager.class);
        if (manager != null && Build.VERSION.SDK_INT >= 26) {
            NotificationChannel channel = new NotificationChannel(CHANNEL_ID, "Weave connection",
                    NotificationManager.IMPORTANCE_LOW);
            manager.createNotificationChannel(channel);
            NotificationChannel finder = new NotificationChannel(FIND_CHANNEL_ID,
                    "Find my Device", NotificationManager.IMPORTANCE_HIGH);
            finder.setDescription("Audible location alert with manual Stop action");
            manager.createNotificationChannel(finder);
            NotificationChannel shop = new NotificationChannel(SHOP_CHANNEL_ID,
                    "FAP installations", NotificationManager.IMPORTANCE_LOW);
            manager.createNotificationChannel(shop);
        }
    }

    private void clearShopNotification() {
        NotificationManager manager = getSystemService(NotificationManager.class);
        if(manager != null) manager.cancel(SHOP_NOTIFICATION_ID);
    }

    /** A separate system notification remains visible when the Activity is backgrounded.
     * The connection is already kept alive by this existing foreground service. */
    private void showShopNotification(String phase, int percent, long eta) {
        NotificationManager manager = getSystemService(NotificationManager.class);
        if(manager == null) return;
        Intent intent = new Intent(this, MainActivity.class);
        PendingIntent open = PendingIntent.getActivity(this, 21, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        final boolean finished = "done".equals(phase) || "failed".equals(phase);
        String state = "done".equals(phase) ? "Installed" : "failed".equals(phase) ?
                "Installation failed" : "installing".equals(phase) ?
                "Transferring to Flipper" : "Downloading";
        String detail = shopTitle + " · " + state;
        if(!finished && percent >= 0) detail += " · " + percent + "%";
        if(!finished && eta > 0) detail += String.format(Locale.US,
                " · %02d:%02d remaining", eta / 60, eta % 60);
        Notification.Builder builder = new Notification.Builder(this, SHOP_CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_weave_notification)
                .setContentTitle("Weave · FAP installation")
                .setContentText(detail).setContentIntent(open)
                .setOngoing(!finished).setAutoCancel(finished).setOnlyAlertOnce(true);
        if(!finished) builder.setProgress(100, Math.max(0, percent), percent < 0);
        manager.notify(SHOP_NOTIFICATION_ID, builder.build());
    }

    private Notification notification(String message) {
        Intent intent = new Intent(this, MainActivity.class);
        PendingIntent open = PendingIntent.getActivity(this, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);
        return new Notification.Builder(this, CHANNEL_ID)
                .setSmallIcon(R.drawable.ic_weave_notification)
                .setContentTitle("Weave")
                .setContentText(message)
                .setContentIntent(open)
                .setOngoing(true)
                .build();
    }

    @Override public void onDestroy() {
        NetworkGateway.reset();
        handler.removeCallbacks(bridgePoll);
        handler.removeCallbacks(deliverScreenFrame);
        pendingScreenFrame = null;
        screenDeliveryScheduled = false;
        handler.removeCallbacks(sendPendingFindFlipper);
        pendingFindFlipperToken = null;
        handler.removeCallbacks(networkHeartbeat);
        handler.removeCallbacks(shopVerify);
        clearShopNotification();
        prefs.edit().remove(KEY_TEXT_TOKEN).remove(KEY_TEXT_HEADER)
                .remove(KEY_TEXT_LIMIT).apply();
        stopDeviceRingNow();
        stopping = true;
        handler.removeCallbacks(bleCliPoll);
        handler.removeCallbacks(bleCliBootstrap);
        handler.removeCallbacks(sendPendingBleCli);
        bleCliCommands.clear();
        deletingBatch = false;
        queuedDeletes.clear();
        handler.removeCallbacks(reconnect);
        handler.removeCallbacks(activateReadyUsb);
        handler.removeCallbacks(drainCatalogQueue);
        handler.removeCallbacks(checkNativeShopHome);
        handler.removeCallbacks(checkRemoteInstallHome);
        handler.removeCallbacks(attemptCatalogUpload);
        handler.removeCallbacks(catalogVerifyTimeout);
        handler.removeCallbacks(shopProcess);
        handler.removeCallbacks(drainShopAckCleanup);
        if (bluetoothReceiverRegistered) {
            try { unregisterReceiver(bluetoothStateReceiver); } catch (RuntimeException ignored) { }
            bluetoothReceiverRegistered = false;
        }
        if (usbReceiverRegistered) {
            try { unregisterReceiver(usbEvents); } catch (RuntimeException ignored) { }
            usbReceiverRegistered = false;
        }
        usb.shutdown();
        ble.close();
        network.shutdownNow();
        prefs.edit().putBoolean(KEY_ACTIVE, false).putBoolean(KEY_CONNECTED, false)
                .putBoolean(KEY_WV1, false).putString(KEY_TRANSPORT, "")
                .remove(KEY_BATTERY).remove(KEY_FIRMWARE).remove(KEY_DEVICE_NAME)
                .remove(KEY_STORAGE_INT_TOTAL).remove(KEY_STORAGE_INT_FREE)
                .remove(KEY_STORAGE_EXT_TOTAL).remove(KEY_STORAGE_EXT_FREE)
                .remove(KEY_STORAGE_INT_ERROR).remove(KEY_STORAGE_EXT_ERROR)
                .putString(KEY_STATUS, "Service stopped").apply();
        super.onDestroy();
    }

    @Override public IBinder onBind(Intent intent) { return null; }
}
