package io.github.nullmess.weave;

import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.ParcelUuid;

import java.nio.charset.StandardCharsets;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.Collections;
import java.util.ArrayDeque;
import java.util.List;
import java.util.Set;
import java.util.UUID;

/**
 * Connects to Flipper Zero's actual, firmware-provided BLE Serial GATT service.
 * The official serial channel speaks protobuf RPC, NOT WV1 text frames. We only
 * read standard GATT Device Information / Battery characteristics until an RPC
 * client is implemented; there is no fake WV1 handshake or simulated data.
 */
final class BleLink implements AutoCloseable {
    interface Callbacks {
        void status(String message);
        void found(BluetoothDevice device);
        void connected(boolean connected);
        void information(String field, String value);
        void inventory(List<String> appIds, java.util.Map<String, String> paths, boolean complete, String message);
        void files(String path, List<FlipperRpc.Item> entries, boolean complete, String message);
        void storageInfo(String path, long total, long free, String error);
        void fileData(String path, byte[] bytes, String error);
        void screen(byte[] bitmap);
        void operation(String operation, boolean success, String message);
        void progress(String operation, int percent);
    }

    // targets/f7/ble_glue/services/serial_service_uuid.inc in the bundled firmware.
    static final UUID FLIPPER_SERIAL = UUID.fromString("8fe5b3d5-2e7f-4a98-2a48-7acc60fe0000");
    // Official BLE serial RPC: host writes to Flipper's RX, and subscribes to its TX indications.
    private static final UUID RPC_WRITE = UUID.fromString("19ed82ae-ed21-4c9d-4145-228e62fe0000");
    private static final UUID RPC_INDICATE = UUID.fromString("19ed82ae-ed21-4c9d-4145-228e61fe0000");
    private static final UUID RPC_FLOW = UUID.fromString("19ed82ae-ed21-4c9d-4145-228e63fe0000");
    private static final UUID CCCD = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb");
    private static final UUID DEVICE_INFO = UUID.fromString("0000180a-0000-1000-8000-00805f9b34fb");
    private static final UUID BATTERY = UUID.fromString("0000180f-0000-1000-8000-00805f9b34fb");
    private static final UUID BATTERY_LEVEL = UUID.fromString("00002a19-0000-1000-8000-00805f9b34fb");
    private static final UUID FIRMWARE_VERSION = UUID.fromString("00002a26-0000-1000-8000-00805f9b34fb");
    private static final UUID SOFTWARE_VERSION = UUID.fromString("00002a28-0000-1000-8000-00805f9b34fb");
    private static final UUID MANUFACTURER = UUID.fromString("00002a29-0000-1000-8000-00805f9b34fb");

    private final Context context;
    private final Callbacks callbacks;
    private final BluetoothAdapter adapter;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private BluetoothLeScanner scanner;
    private BluetoothGatt gatt;
    private boolean connected;
    private UsbLink usbTransport;
    private boolean connecting;
    private boolean scanning;
    private final ArrayDeque<BluetoothGattCharacteristic> reads = new ArrayDeque<>();
    private boolean reading;
    private BluetoothDevice lastDevice;
    private BluetoothGattCharacteristic rpcWrite;
    private boolean rpcReady, rpcSubscribing, inventoryRunning;
    private final ArrayDeque<String> inventoryPaths = new ArrayDeque<>();
    private final HashSet<String> inventoryVisited = new HashSet<>();
    private final HashSet<String> inventoryNames = new HashSet<>();
    private final java.util.Map<String, String> inventoryFullPaths = new java.util.HashMap<>();
    private final ByteArrayOutputStream rpcInput = new ByteArrayOutputStream();
    private final ArrayList<FlipperRpc.Item> replyItems = new ArrayList<>();
    private final ArrayDeque<byte[]> outgoing = new ArrayDeque<>();
    private final ArrayDeque<byte[]> interactiveOutgoing = new ArrayDeque<>();
    private final RpcHealth rpcHealth = new RpcHealth();
    private long pendingWriteStartedAt;
    private long lastHealthLogAt;
    private static final int MAX_RPC_BUFFER_BYTES = 1_100_000;
    private static final long GATT_WRITE_TIMEOUT_MS = 10000L;
    private final Runnable gattWriteTimeout = this::onGattWriteTimeout;

    private void onGattWriteTimeout() {
        if (!writeBusy || gatt == null || !connected || isUsbActive()) return;
        rpcHealth.failedWrite();
        callbacks.status("Flipper BLE write ACK timed out; resetting GATT");
        // La reconnexion reprend les jobs via WeaveService, jamais une demi-trame.
        disconnectGatt();
    }


    private void reportHealthIfDue() {
        long now = android.os.SystemClock.uptimeMillis();
        if (now - lastHealthLogAt < 60000L) return;
        lastHealthLogAt = now;
        android.util.Log.d("WeaveRPC", rpcHealth.summary());
    }

    private int heldInputMask;
    private byte[] pendingWrite;
    private int pendingOffset, credits, writeChunkSize = 20;
    private boolean writeBusy, flowSubscribed;
    private BluetoothGattCharacteristic rpcFlow;
    private String filePath, deferredFilesPath;
    private final ArrayDeque<String> storageInfoQueue = new ArrayDeque<>();
    private int storageInfoId;
    private String storageInfoPath;
    private final Runnable storageInfoTimeout = () -> finishStorageInfo(null, "Storage RPC timed out");
    private final Runnable storageInfoNext = this::startNextStorageInfo;
    // A service-discovered link is not usable until the protobuf channel subscribes.
    private final Runnable rpcHandshakeTimeout = this::onRpcHandshakeTimeout;

    private void onRpcHandshakeTimeout() {
        if (connected && !rpcReady) {
            callbacks.status("Flipper BLE RPC handshake timed out — recovering connection");
            disconnectGatt();
        }
    }
    private String readPath;
    private int readId;
    private final ByteArrayOutputStream readContents = new ByteArrayOutputStream();
    private final Runnable readTimeout = () -> finishRead("Flipper file download timed out");
    private final ArrayList<FlipperRpc.Item> fileItems = new ArrayList<>();
    private int fileId;
    private int deviceInfoId;
    private int apiMajor = -1, apiMinor = -1;
    private int deviceInfoRetries;
    private boolean firmwareVersionSeen;
    private boolean deviceInfoRefreshQueued;
    private final Runnable deviceInfoTimeout = this::onDeviceInfoTimeout;
    private final Runnable deviceInfoRetry = this::requestDeviceInfo;

    private void onDeviceInfoTimeout() {
        if (!rpcReady) return;
        // The API version can arrive without firmware_version. Retry specifically
        // for the installed version; do not leave Settings in "Reading" forever.
        deviceInfoId = 0;
        finishDeviceInfoRequest();
    }

    private void finishDeviceInfoRequest() {
        if (deviceInfoRefreshQueued) {
            deviceInfoRefreshQueued = false;
            deviceInfoRetries = 0;
            firmwareVersionSeen = false;
            requestDeviceInfo();
        } else if (!firmwareVersionSeen && deviceInfoRetries < 3 && rpcReady) {
            handler.removeCallbacks(deviceInfoRetry);
            handler.postDelayed(deviceInfoRetry, 500);
        } else if (!firmwareVersionSeen && rpcReady) {
            callbacks.information("firmware_error", "Flipper RPC did not return firmware_version");
        }
    }
    private String operationName;
    private int operationId;
    private byte[] uploading;
    private String uploadTarget;
    private int uploadTotalFrames, uploadSentFrames, lastProgressSent = -1;
    // BLE can stall with Android GATT 133 when thousands of writes are queued at once.
    // Keep at most one protobuf write frame in flight, with a short radio pacing gap.
    private int uploadNextOffset;
    private boolean uploadServerAcked;
    private boolean uploadCancelRequested, uploadCancelFrameQueued;
    private static final int UPLOAD_RPC_BYTES = 128;
    private static final int UPLOAD_WRITE_PAUSE_MS = 12;
    private final Runnable uploadNextFrame = this::queueNextUploadFrame;
    private final Runnable cancelUploadDeadline = this::onCancelUploadDeadline;

    @SuppressLint("MissingPermission")
    private void onCancelUploadDeadline() {
        if (!uploadCancelRequested || uploading == null) return;
        callbacks.status("Transfer cancel timed out; resetting RPC link");
        // Le fichier staging protege l'ancienne FAP apres la coupure.
        if (isUsbActive() && usbTransport != null) {
            usbTransport.close();
            onUsbDisconnected();
        } else if (gatt != null) {
            try { gatt.disconnect(); } catch (RuntimeException ignored) { disconnectGatt(); }
        } else finishOperation(false, "Cancelled");
    }
    private byte[] retryTransferBytes;
    private String retryTransferPath, retryTransferLabel, retryTransferDevice;
    private boolean transferRetried;
    private final Runnable retryTransferDeadline = this::onRetryTransferDeadline;

    private void onRetryTransferDeadline() {
        if (retryTransferBytes == null) return;
        String label = retryTransferLabel;
        clearRetryTransfer();
        callbacks.operation(label, false, "GATT 133: Flipper did not reconnect to retry transfer");
    }

    private void clearRetryTransfer() {
        handler.removeCallbacks(retryTransferDeadline);
        retryTransferBytes = null;
        retryTransferPath = null;
        retryTransferLabel = null;
        retryTransferDevice = null;
    }

    private void retryTransferWhenReady() {
        if (!rpcReady || retryTransferBytes == null) return;
        String deviceKey = connectedDeviceKey();
        if (deviceKey == null || !deviceKey.equals(retryTransferDevice)) {
            String label = retryTransferLabel;
            clearRetryTransfer();
            callbacks.operation(label, false, "Flipper identity changed; canceled upload retry");
            return;
        }
        byte[] saved = retryTransferBytes;
        String path = retryTransferPath;
        String label = retryTransferLabel;
        clearRetryTransfer();
        callbacks.status("Flipper BLE reconnected: retrying upload from the beginning once");
        handler.postDelayed(() -> uploadFile(path, saved, label), 550);
    }
    private final Runnable fileTimeout = () -> finishFiles(false, "Flipper RPC directory request timed out");
    private final Runnable operationTimeout = () -> finishOperation(false, "Flipper RPC operation timed out");
    private int commandId, pendingId;
    private String pendingPath;
    private final Runnable inventoryTimeout = () -> finishInventory(false, "Flipper RPC inventory timed out");
    // Display streaming is a session-wide GUI service, including while a .fap runs.
    // Recover a stream whose first frame never arrived, without continuously restarting
    // a healthy but static display (which may legitimately produce no new frames).
    private boolean screenFrameSeen;
    private boolean screenRecoveryAttempted;
    private boolean screenStreamEnabled;
    private boolean screenSessionActive;
    private long screenRestartGeneration;
    private boolean screenRestartPending;
    private long lastScreenFrameAt, lastScreenRepairAt;
    private long lastInteractiveInputAt;
    private int screenInputsAwaitingFrame;
    private final Runnable screenInputWatchdog = this::onScreenInputWatchdog;

    private void onScreenInputWatchdog() {
        if (!connected || !rpcReady || !screenStreamEnabled || uploading != null) return;
        long now = android.os.SystemClock.uptimeMillis();
        // Une touche sur un ecran statique ne prouve pas une panne du Display.
        if (screenInputsAwaitingFrame < 4 || now - lastScreenFrameAt < 6500 ||
                now - lastScreenRepairAt < 30000) return;
        lastScreenRepairAt = now;
        screenInputsAwaitingFrame = 0;
        callbacks.status("Flipper screen stream stalled after input; restarting GUI RPC");
        restartScreenStream();
    }
    private final Runnable screenInitialTimeout = this::onScreenInitialTimeout;

    private void onScreenInitialTimeout() {
        if (!connected || !rpcReady || screenFrameSeen) return;
        if (!screenRecoveryAttempted) {
            screenRecoveryAttempted = true;
            callbacks.status("Flipper screen stream: no first frame, retrying");
            restartScreenStream();
        } else {
            callbacks.status("Flipper BLE connected, but no screen frames received");
        }
    }

    // The normal startScreen() intentionally does nothing when already active.
    // A manual Home refresh must instead close and reopen the GUI RPC stream.
    // Interactive queue priority prevents background inventory RPC from leaving
    // the restart stuck behind a large list of low-priority requests.
    void recoverScreen() {
        handler.post(() -> {
            if (!connected || !rpcReady || uploading != null || operationId != 0 ||
                    !context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE)
                            .getBoolean("screen_requested", false)) return;
            screenStreamEnabled = true;
            screenRecoveryAttempted = false;
            restartScreenStream();
        });
    }

    private void restartScreenStream() {
        if (!connected || !rpcReady || !screenStreamEnabled || uploading != null ||
                operationId != 0) return;
        final long generation = ++screenRestartGeneration;
        screenRestartPending = true;
        handler.removeCallbacks(screenInitialTimeout);
        handler.removeCallbacks(screenInputWatchdog);
        screenSessionActive = false;
        screenFrameSeen = false;
        queueInteractivePacket(FlipperRpc.streamRequest(++commandId, false));
        handler.postDelayed(() -> {
            if (generation != screenRestartGeneration || !connected || !rpcReady ||
                    !screenStreamEnabled || uploading != null || operationId != 0) return;
            setDisplayConnectionPriority(true);
            screenSessionActive = true;
            queueInteractivePacket(FlipperRpc.streamRequest(++commandId, true));
            screenRestartPending = false;
            handler.removeCallbacks(screenInitialTimeout);
            handler.postDelayed(screenInitialTimeout, 10000);
        }, 380);
    }


    BleLink(Context context, Callbacks callbacks) {
        this.context = context;
        this.callbacks = callbacks;
        BluetoothManager manager = (BluetoothManager) context.getSystemService(Context.BLUETOOTH_SERVICE);
        adapter = manager == null ? null : manager.getAdapter();
    }

    boolean isConnected() { return connected; }
    boolean isConnecting() { return connecting; }
    boolean isAvailable() { return adapter != null && adapter.isEnabled(); }

    @SuppressLint("MissingPermission")
    BluetoothDevice pairedFlipper() {
        if (!isAvailable()) return null;
        try {
            Set<BluetoothDevice> bonded = adapter.getBondedDevices();
            if (lastDevice != null && lastDevice.getBondState() == BluetoothDevice.BOND_BONDED)
                return lastDevice;
            String preferred = context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE)
                    .getString("preferred_flipper_ble", "");
            for (BluetoothDevice device : bonded) {
                if (preferred.equalsIgnoreCase(device.getAddress())) return device;
            }
            for (BluetoothDevice device : bonded) {
                String name = device.getName();
                if (isFlipperName(name)) return device;
            }
        } catch (SecurityException denied) { callbacks.status("Bluetooth permission required"); }
        return null;
    }

    // Flipper advertises the 16-bit service 0x3080 OR'ed with its hardware
    // color (0x3080..0x3083). The actual RPC GATT service has a 128-bit UUID.
    // A renamed Flipper must remain discoverable without a name prefix.
    private static boolean advertisesFlipperService(List<ParcelUuid> advertised) {
        if (advertised == null) return false;
        for (ParcelUuid parcel : advertised) {
            UUID uuid = parcel.getUuid();
            if (FLIPPER_SERIAL.equals(uuid)) return true;
            long msb = uuid.getMostSignificantBits();
            if (uuid.getLeastSignificantBits() == 0x800000805f9b34fbL
                    && (msb & 0xffffffffL) == 0x00001000L
                    && ((msb >>> 32) & 0xfffffffcL) == 0x00003080L) return true;
        }
        return false;
    }

    private static boolean isFlipperName(String name) {
        return name != null && (name.toLowerCase(java.util.Locale.ROOT).startsWith("flipper")
                || name.toLowerCase(java.util.Locale.ROOT).startsWith("weave"));
    }

    private final ScanCallback scanCallbacks = new ScanCallback() {
        @Override public void onScanResult(int callbackType, ScanResult result) {
            BluetoothDevice device = result.getDevice();
            List<ParcelUuid> advertised = result.getScanRecord() == null ? null : result.getScanRecord().getServiceUuids();
            boolean realService = advertisesFlipperService(advertised);
            String name = result.getScanRecord() == null ? null : result.getScanRecord().getDeviceName();
            if (!realService && !isFlipperName(name)) {
                try { if (!isFlipperName(device.getName())) return; }
                catch (SecurityException denied) { return; }
            }
            callbacks.found(device);
            stopScan();
        }
        @Override public void onScanFailed(int errorCode) {
            scanning = false;
            callbacks.status("BLE scan failed (" + errorCode + ")");
        }
    };
    private final Runnable scanTimeout = this::stopScan;
    // Method reference defers accessing constructor-initialized `callbacks`.
    // A lambda body in the field initializer triggered javac's definite-assignment error.
    private final Runnable connectTimeout = this::onConnectTimeout;

    private void onConnectTimeout() {
        if (connecting && !connected) {
            callbacks.status("Flipper BLE connection timed out — retrying");
            disconnectGatt();
        }
    }

    @SuppressLint("MissingPermission")
    void scan() {
        if (!isAvailable()) { callbacks.status("Enable Bluetooth on this phone"); return; }
        if (scanning || connecting || connected) return;
        try {
            scanner = adapter.getBluetoothLeScanner();
            if (scanner == null) { callbacks.status("BLE scanner unavailable"); return; }
            // Scan by identity then verify the actual Serial service on discovery.
            // Some firmware advertisements omit the complete 128-bit UUID.
            scanner.startScan(null, new ScanSettings.Builder()
                    .setScanMode(ScanSettings.SCAN_MODE_LOW_POWER).build(), scanCallbacks);
            scanning = true;
            handler.removeCallbacks(scanTimeout);
            handler.postDelayed(scanTimeout, 12000);
            callbacks.status("Searching for Flipper BLE…");
        } catch (SecurityException denied) { callbacks.status("Bluetooth permission required"); }
        catch (RuntimeException error) { callbacks.status("Could not start BLE scan"); }
    }

    @SuppressLint("MissingPermission")
    private void stopScan() {
        handler.removeCallbacks(scanTimeout);
        if (scanner != null && scanning) {
            try { scanner.stopScan(scanCallbacks); } catch (RuntimeException ignored) { }
        }
        scanner = null;
        scanning = false;
    }

    @SuppressLint("MissingPermission")
    void connect(BluetoothDevice device) {
        if (device == null || connected || connecting) return;
        rpcHealth.reconnect();
        stopScan();
        disconnectGatt();
        lastDevice = device;
        connecting = true;
        try {
            gatt = device.connectGatt(context, false, gattCallbacks, BluetoothDevice.TRANSPORT_LE);
            if (gatt == null) { connecting = false; callbacks.status("Unable to open Flipper GATT"); return; }
            handler.removeCallbacks(connectTimeout);
            handler.postDelayed(connectTimeout, 17000);
            callbacks.status("Connecting to Flipper BLE…");
        } catch (SecurityException denied) {
            connecting = false;
            callbacks.status("Bluetooth connect permission required");
        } catch (RuntimeException error) {
            connecting = false;
            callbacks.status("Flipper BLE connection failed");
        }
    }

    @SuppressLint("MissingPermission")
    private boolean prepareUploadRetry(BluetoothGatt source) {
        if (uploading == null || transferRetried || uploadCancelRequested) return false;
        // Restart from byte 0 after reconnect; partial .fap writes must never be resumed.
        retryTransferBytes = uploading;
        retryTransferPath = uploadTarget;
        retryTransferLabel = operationName;
        try { retryTransferDevice = source.getDevice().getAddress(); }
        catch (SecurityException denied) { retryTransferDevice = null; }
        transferRetried = true;
        handler.removeCallbacks(operationTimeout);
        handler.removeCallbacks(uploadNextFrame);
        operationId = 0;
        operationName = null;
        uploading = null;
        uploadTarget = null;
        handler.removeCallbacks(retryTransferDeadline);
        handler.postDelayed(retryTransferDeadline, 45000);
        callbacks.status("GATT 133 during upload — reconnecting for one full retry");
        return true;
    }

    private final BluetoothGattCallback gattCallbacks = new BluetoothGattCallback() {
        @Override public void onConnectionStateChange(BluetoothGatt source, int status, int newState) {
            if (source != gatt) return;
            if (status == BluetoothGatt.GATT_SUCCESS && newState == BluetoothProfile.STATE_CONNECTED) {
                callbacks.status("Bluetooth link open — verifying Flipper services…");
                if (!source.discoverServices()) {
                    callbacks.status("Could not discover Flipper BLE services");
                    disconnectGatt();
                }
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED || status != BluetoothGatt.GATT_SUCCESS) {
                if (status == 133) prepareUploadRetry(source);
                callbacks.status(status == 0 ? "Flipper BLE disconnected — retrying" :
                        (status == 133 ? "Android GATT 133: radio link dropped during transfer — retry after reconnect" :
                        "Flipper BLE disconnected (GATT " + status + ") — retrying"));
                disconnectGatt();
            }
        }

        @Override public void onServicesDiscovered(BluetoothGatt source, int status) {
            if (source != gatt) return;
            if (status != BluetoothGatt.GATT_SUCCESS || source.getService(FLIPPER_SERIAL) == null) {
                callbacks.status("Not a Flipper Serial BLE service (check Bluetooth mode)");
                disconnectGatt();
                return;
            }
            handler.removeCallbacks(connectTimeout);
            connecting = false;
            connected = true;
            // Wait until notifications and flow control are established before reporting ready.
            handler.removeCallbacks(rpcHandshakeTimeout);
            handler.postDelayed(rpcHandshakeTimeout, 18000);
            callbacks.status("Flipper BLE connected — official GATT (WV1 gateway not active)");
            try {
                String name = source.getDevice().getName();
                if (name != null) callbacks.information("name", name);
            } catch (SecurityException ignored) { }
            if (!source.requestMtu(247)) refreshInformation();
            // Some Android BLE stacks omit onMtuChanged; continue discovery regardless.
            handler.postDelayed(() -> {
                if (source == gatt && connected && !rpcReady && !rpcSubscribing)
                    refreshInformation();
            }, 3500);
        }

        @Override public void onCharacteristicRead(BluetoothGatt source,
                BluetoothGattCharacteristic characteristic, int status) {
            processRead(source, characteristic, characteristic.getValue(), status);
        }

        @Override public void onCharacteristicRead(BluetoothGatt source,
                BluetoothGattCharacteristic characteristic, byte[] value, int status) {
            if (android.os.Build.VERSION.SDK_INT >= 33) processRead(source, characteristic, value, status);
        }

        @Override public void onDescriptorWrite(BluetoothGatt source, BluetoothGattDescriptor descriptor, int status) {
            if (source != gatt || !CCCD.equals(descriptor.getUuid())) return;
            if (status != BluetoothGatt.GATT_SUCCESS) {
                rpcSubscribing = false;
                callbacks.status("Flipper RPC notification subscription failed");
                disconnectGatt();
                return;
            }
            if (!flowSubscribed) {
                flowSubscribed = true;
                subscribeFlow();
            } else {
                rpcSubscribing = false;
                if (rpcFlow != null) source.readCharacteristic(rpcFlow);
            }
        }
        @Override public void onCharacteristicWrite(BluetoothGatt source,
                BluetoothGattCharacteristic characteristic, int status) {
            if (source != gatt || !RPC_WRITE.equals(characteristic.getUuid())) return;
            writeBusy = false;
            handler.removeCallbacks(gattWriteTimeout);
            if (pendingWriteStartedAt != 0) {
                rpcHealth.writeLatency(android.os.SystemClock.uptimeMillis() - pendingWriteStartedAt);
                pendingWriteStartedAt = 0;
            }
            if (status != BluetoothGatt.GATT_SUCCESS) {
                rpcHealth.failedWrite();
                if (status == 133 && prepareUploadRetry(source)) {
                    disconnectGatt();
                    return;
                }
                outgoing.clear(); interactiveOutgoing.clear(); pendingWrite = null;
                finishInventory(false, "Flipper RPC write failed: " + status);
                finishFiles(false, "Flipper RPC write failed: " + status);
                finishOperation(false, "Flipper RPC write failed: " + status);
                return;
            }
            if (uploadServerAcked && uploading != null && !writeBusy && pendingWrite == null
                    && outgoing.isEmpty() && interactiveOutgoing.isEmpty() && (uploadCancelFrameQueued || uploadNextOffset == uploading.length)) {
                finishOperation(!uploadCancelRequested, uploadCancelRequested ? "Cancelled" : null);
                if (screenStreamEnabled) handler.postDelayed(BleLink.this::startScreen, 500);
                return;
            }
            if (uploading != null && uploadTotalFrames > 0) {
                // This is radio transfer progress, NOT a successful installation.
                int percent = Math.min(99, (uploadSentFrames * 100) / uploadTotalFrames);
                if (percent >= lastProgressSent + 2) {
                    lastProgressSent = percent;
                    callbacks.progress(operationName, percent);
                }
                if (!writeBusy && pendingWrite == null && outgoing.isEmpty() && interactiveOutgoing.isEmpty()
                        && (uploadCancelRequested || uploadNextOffset < uploading.length)) {
                    handler.removeCallbacks(uploadNextFrame);
                    handler.postDelayed(uploadNextFrame, UPLOAD_WRITE_PAUSE_MS);
                    return;
                }
            }
            // Pacing prevents flooding Android's controller even with large MTUs.
            if (uploading != null) handler.postDelayed(BleLink.this::sendNextChunk, UPLOAD_WRITE_PAUSE_MS);
            else sendNextChunk();
        }
        @Override public void onCharacteristicChanged(BluetoothGatt source, BluetoothGattCharacteristic characteristic) {
            if (android.os.Build.VERSION.SDK_INT < 33) onBleNotification(source, characteristic, characteristic.getValue());
        }
        @Override public void onCharacteristicChanged(BluetoothGatt source,
                BluetoothGattCharacteristic characteristic, byte[] value) {
            if (android.os.Build.VERSION.SDK_INT >= 33) onBleNotification(source, characteristic, value);
        }
        @Override public void onMtuChanged(BluetoothGatt source, int mtu, int status) {
            if (source != gatt) return;
            if (status == BluetoothGatt.GATT_SUCCESS) writeChunkSize = Math.max(20, Math.min(120, mtu - 3));
            refreshInformation();
        }
    };

    private synchronized void processRead(BluetoothGatt source, BluetoothGattCharacteristic characteristic,
                                          byte[] value, int status) {
        if (source != gatt) return;
        if (status == BluetoothGatt.GATT_SUCCESS && value != null) {
            if (RPC_FLOW.equals(characteristic.getUuid())) {
                grantCredits(value);
                rpcReady = true;
                handler.removeCallbacks(rpcHandshakeTimeout);
                String verifiedAddress = connectedDeviceKey();
                if (verifiedAddress != null)
                    context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE)
                            .edit().putString("preferred_flipper_ble", verifiedAddress).apply();
                callbacks.connected(true);
                callbacks.status("Flipper protobuf RPC ready");
                // Re-read battery/name now that WeaveService accepts verified GATT values.
                handler.postDelayed(this::refreshInformation, 700);
                if (context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE)
                        .getBoolean("screen_requested", false)) startScreen();
                requestDeviceInfo();
                retryTransferWhenReady();
                if (deferredFilesPath != null) {
                    String wanted = deferredFilesPath;
                    deferredFilesPath = null;
                    listFiles(wanted);
                } else requestInventory();
                requestStorageInfo("/ext");
                requestStorageInfo("/int");
                reading = false;
                return;
            }
            // GATT Firmware Revision describes the hardware target and GATT
            // Software Revision describes the git build. Neither is the
            // installed Weave release: use RPC firmware_version exclusively.
            if (BATTERY_LEVEL.equals(characteristic.getUuid()) && value.length > 0) {
                int percent = value[0] & 0xff;
                if (percent <= 100) callbacks.information("battery", String.valueOf(percent));
            } else if (MANUFACTURER.equals(characteristic.getUuid())) {
                String manufacturer = new String(value, StandardCharsets.UTF_8).trim();
                if (!manufacturer.isEmpty()) callbacks.information("manufacturer", manufacturer);
            }
        }
        reading = false;
        readNext();
    }

    @SuppressLint("MissingPermission")
    synchronized void refreshInformation() {
        if (!connected || gatt == null) return;
        reads.clear();
        BluetoothGattService battery = gatt.getService(BATTERY);
        if (battery != null && battery.getCharacteristic(BATTERY_LEVEL) != null)
            reads.add(battery.getCharacteristic(BATTERY_LEVEL));
        BluetoothGattService info = gatt.getService(DEVICE_INFO);
        if (info != null) {
            if (info.getCharacteristic(FIRMWARE_VERSION) != null)
                reads.add(info.getCharacteristic(FIRMWARE_VERSION));
            if (info.getCharacteristic(SOFTWARE_VERSION) != null)
                reads.add(info.getCharacteristic(SOFTWARE_VERSION));
            if (info.getCharacteristic(MANUFACTURER) != null)
                reads.add(info.getCharacteristic(MANUFACTURER));
        }
        readNext();
    }

    @SuppressLint("MissingPermission")
    private synchronized void readNext() {
        if (reading || gatt == null) return;
        BluetoothGattCharacteristic next = reads.poll();
        if (next != null) {
            try {
                reading = gatt.readCharacteristic(next);
                if (!reading) readNext();
            } catch (RuntimeException error) { reading = false; readNext(); }
        } else if (connected && !rpcReady && !rpcSubscribing) {
            subscribeRpc();
        }
    }

    @SuppressLint("MissingPermission")
    private void subscribeRpc() {
        BluetoothGatt current = gatt;
        if (current == null) return;
        BluetoothGattService svc = current.getService(FLIPPER_SERIAL);
        if (svc == null) return;
        rpcWrite = svc.getCharacteristic(RPC_WRITE);
        BluetoothGattCharacteristic indications = svc.getCharacteristic(RPC_INDICATE);
        if (rpcWrite == null || indications == null) {
            callbacks.inventory(Collections.emptyList(), Collections.emptyMap(), false, "Flipper RPC characteristics unavailable");
            return;
        }
        BluetoothGattDescriptor descriptor = indications.getDescriptor(CCCD);
        if (descriptor == null || !current.setCharacteristicNotification(indications, true)) return;
        rpcSubscribing = true;
        try {
            if (android.os.Build.VERSION.SDK_INT >= 33) {
                if (current.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_INDICATION_VALUE) != 0)
                    rpcSubscribing = false;
            } else {
                descriptor.setValue(BluetoothGattDescriptor.ENABLE_INDICATION_VALUE);
                if (!current.writeDescriptor(descriptor)) rpcSubscribing = false;
            }
        } catch (RuntimeException error) { rpcSubscribing = false; }
    }

    @SuppressLint("MissingPermission")
    private void subscribeFlow() {
        if (gatt == null) return;
        BluetoothGattService service = gatt.getService(FLIPPER_SERIAL);
        rpcFlow = service == null ? null : service.getCharacteristic(RPC_FLOW);
        if (rpcFlow == null) {
            callbacks.status("Flipper BLE flow-control characteristic unavailable");
            return;
        }
        BluetoothGattDescriptor descriptor = rpcFlow.getDescriptor(CCCD);
        if (descriptor == null || !gatt.setCharacteristicNotification(rpcFlow, true)) {
            callbacks.status("Flipper BLE flow-control subscription unavailable");
            return;
        }
        try {
            if (android.os.Build.VERSION.SDK_INT >= 33)
                gatt.writeDescriptor(descriptor, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
            else {
                descriptor.setValue(BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE);
                gatt.writeDescriptor(descriptor);
            }
        } catch (RuntimeException error) { callbacks.status("Flipper BLE flow-control subscription failed"); }
    }

    private void grantCredits(byte[] bytes) {
        if (bytes == null || bytes.length < 4) return;
        long amount = ((long)(bytes[0] & 255) << 24) | ((long)(bytes[1] & 255) << 16)
                | ((long)(bytes[2] & 255) << 8) | (bytes[3] & 255);
        if (amount <= 0 || amount > 131072) return;
        credits = (int)amount;
        sendNextChunk();
    }

    private void onBleNotification(BluetoothGatt source, BluetoothGattCharacteristic characteristic, byte[] bytes) {
        if (source != gatt || bytes == null) return;
        if (RPC_FLOW.equals(characteristic.getUuid())) grantCredits(bytes);
        else onRpcBytes(source, characteristic, bytes);
    }

    /** Activate USB only after its real protobuf handshake succeeds. Until then
     *  the existing Bluetooth connection remains usable. */
    void activateUsb(UsbLink link) {
        if (link == null || !link.isReady()) return;
        if (connected && usbTransport == link) return;
        // Do not report a false offline transition while handing over BLE to USB.
        connected = false;
        disconnectGatt();
        usbTransport = link;
        connected = true;
        rpcReady = true;
        commandId = 0;
        callbacks.connected(true);
        callbacks.status("Flipper protobuf RPC ready over USB CDC");
        if (context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE)
                .getBoolean("screen_requested", false)) startScreen();
        requestDeviceInfo();
        requestInventory();
        requestStorageInfo("/ext");
        requestStorageInfo("/int");
    }

    boolean isUsbActive() { return usbTransport != null && usbTransport.isReady(); }

    void onUsbRpcBytes(byte[] data) {
        handler.post(() -> {
            if (isUsbActive() && data != null) acceptRpcBytes(data);
        });
    }

    void onUsbDisconnected() {
        handler.post(() -> {
            if (usbTransport == null) return;
            usbTransport = null;
            disconnectGatt();
            callbacks.status("USB disconnected — switching back to Bluetooth");
        });
    }

    private void afterUsbWrite(boolean success) {
        writeBusy = false;
        if (!isUsbActive() || !success) {
            if (usbTransport != null) usbTransport.close();
            onUsbDisconnected();
            return;
        }
        if (uploadServerAcked && uploading != null && pendingWrite == null &&
                outgoing.isEmpty() && interactiveOutgoing.isEmpty() && (uploadCancelFrameQueued || uploadNextOffset == uploading.length)) {
            finishOperation(!uploadCancelRequested, uploadCancelRequested ? "Cancelled" : null);
            if (screenStreamEnabled) handler.postDelayed(this::startScreen, 500);
            return;
        }
        if (uploading != null && uploadTotalFrames > 0) {
            int percent = Math.min(99, (uploadSentFrames * 100) / uploadTotalFrames);
            if (percent >= lastProgressSent + 2) {
                lastProgressSent = percent;
                callbacks.progress(operationName, percent);
            }
            if (outgoing.isEmpty() && interactiveOutgoing.isEmpty() && (uploadCancelRequested || uploadNextOffset < uploading.length)) {
                handler.postDelayed(uploadNextFrame, 2);
            }
        }
        sendNextChunk();
    }

    private synchronized void queuePacket(byte[] packet) {
        if (packet == null || packet.length == 0) return;
        outgoing.addLast(packet);
        sendNextChunk();
    }

    private synchronized void queueInteractivePacket(byte[] packet) {
        if (packet == null || packet.length == 0) return;
        interactiveOutgoing.addLast(packet);
        sendNextChunk();
    }

    @SuppressLint("MissingPermission")
    private void setDisplayConnectionPriority(boolean high) {
        BluetoothGatt current = gatt;
        if (current == null || !connected || isUsbActive()) return;
        try {
            current.requestConnectionPriority(high ? BluetoothGatt.CONNECTION_PRIORITY_HIGH :
                    BluetoothGatt.CONNECTION_PRIORITY_BALANCED);
        } catch (SecurityException | IllegalStateException ignored) { /* BLE non disponible. */ }
    }

    void startScreen() {
        screenStreamEnabled = true;
        if (!connected || !rpcReady || uploading != null || screenSessionActive) return;
        ++screenRestartGeneration; // Cancel a delayed start from an older restart.
        screenRestartPending = false;
        setDisplayConnectionPriority(true);
        screenSessionActive = true;
        screenFrameSeen = false;
        queueInteractivePacket(FlipperRpc.streamRequest(++commandId, true));
        handler.removeCallbacks(screenInitialTimeout);
        handler.postDelayed(screenInitialTimeout, 10000);
    }

    void stopScreen() {
        screenStreamEnabled = false;
        ++screenRestartGeneration;
        screenRestartPending = false;
        handler.removeCallbacks(screenInputWatchdog);
        handler.removeCallbacks(screenInitialTimeout);
        setDisplayConnectionPriority(false);
        if (!rpcReady || !screenSessionActive) return;
        screenSessionActive = false;
        queueInteractivePacket(FlipperRpc.streamRequest(++commandId, false));
    }

    // Les inputs prioritaires passent avant les futures lectures RPC, sans couper une trame.
    // Le firmware accepte RebootRequest.DFU via PB.Main RPC natif.
    boolean enterFirmwareRecovery() {
        if (!connected || !rpcReady || operationId != 0 || uploading != null) return false;
        queueInteractivePacket(FlipperRpc.rebootDfuRequest(++commandId));
        return true;
    }

    void inputEvent(int key, int type) {
        if (key < 0 || key > 5 || type < 0 || type > 4) return;
        handler.post(() -> {
            if (!connected || !rpcReady) return;
            if (type == 0) heldInputMask |= (1 << key);
            else if (type == 1) heldInputMask &= ~(1 << key);
            // REPEAT est facultatif : ne jamais retarder LONG/RELEASE pour lui.
            if (type == 4 && interactiveOutgoing.size() >= 2) return;
            if (type == 2 || type == 3) {
                lastInteractiveInputAt = android.os.SystemClock.uptimeMillis();
                screenInputsAwaitingFrame = Math.min(6, screenInputsAwaitingFrame + 1);
            }
            queueInteractivePacket(FlipperRpc.inputRequest(++commandId, key, type));
            if (type == 1 && screenStreamEnabled && uploading == null) {
                handler.removeCallbacks(screenInputWatchdog);
                handler.postDelayed(screenInputWatchdog, 7200);
            }
        });
    }

    // One complete input sequence at a time.  The Flipper associates SHORT and RELEASE
    // with the preceding PRESS; back-to-back BLE writes previously overlapped rapidly
    // repeated taps and made some button events disappear.
    private final ArrayDeque<Integer> keyQueue = new ArrayDeque<>();
    private boolean keySequenceRunning;

    // Synthetic long-Back reaches the existing desktop global power menu.
    // Key 6 is an internal queue sentinel and is transmitted as Back (5).
    void longBack() {
        if (!rpcReady) return;
        handler.post(() -> {
            if (keyQueue.size() >= 12) return;
            keyQueue.addLast(6);
            if (!keySequenceRunning) sendQueuedKey();
        });
    }

    void pressKey(int key) {
        if (!rpcReady || key < 0 || key > 5) return;
        handler.post(() -> {
            if (keyQueue.size() >= 12) return;
            keyQueue.addLast(key);
            if (!keySequenceRunning) sendQueuedKey();
        });
    }

    private void sendQueuedKey() {
        if (!rpcReady || !connected) {
            keyQueue.clear();
            keySequenceRunning = false;
            return;
        }
        Integer key = keyQueue.pollFirst();
        if (key == null) { keySequenceRunning = false; return; }
        keySequenceRunning = true;
        lastInteractiveInputAt = android.os.SystemClock.uptimeMillis();
        screenInputsAwaitingFrame = Math.min(6, screenInputsAwaitingFrame + 1);
        final boolean longBack = key == 6;
        final int physicalKey = longBack ? 5 : key;
        queueInteractivePacket(FlipperRpc.inputRequest(++commandId, physicalKey, 0)); // PRESS
        if (screenStreamEnabled && uploading == null) {
            handler.removeCallbacks(screenInputWatchdog);
            handler.postDelayed(screenInputWatchdog, 7200);
        }
        handler.postDelayed(() -> {
            if (connected && rpcReady)
                queueInteractivePacket(FlipperRpc.inputRequest(++commandId, physicalKey, longBack ? 3 : 2));
        }, longBack ? 350 : 55); // LONG=3, SHORT=2
        handler.postDelayed(() -> {
            if (connected && rpcReady)
                queueInteractivePacket(FlipperRpc.inputRequest(++commandId, physicalKey, 1)); // RELEASE
            sendQueuedKey();
        }, longBack ? 425 : 110);
    }

    void requestStorageInfo(String path) {
        if (!"/int".equals(path) && !"/ext".equals(path)) return;
        handler.post(() -> {
            if (!path.equals(storageInfoPath) && !storageInfoQueue.contains(path))
                storageInfoQueue.addLast(path);
            startNextStorageInfo();
        });
    }

    private void startNextStorageInfo() {
        handler.removeCallbacks(storageInfoNext);
        if (!rpcReady || storageInfoQueue.isEmpty() || storageInfoId != 0) return;
        // Background capacity queries never interrupt a file upload, CLI or inventory.
        if (!isIdleForShop()) {
            handler.postDelayed(storageInfoNext, 650);
            return;
        }
        storageInfoPath = storageInfoQueue.pollFirst();
        storageInfoId = ++commandId;
        handler.postDelayed(storageInfoTimeout, 9500);
        queuePacket(FlipperRpc.storageInfoRequest(storageInfoId, storageInfoPath));
    }

    private void finishStorageInfo(FlipperRpc.StorageSpace info, String error) {
        if (storageInfoId == 0) return;
        handler.removeCallbacks(storageInfoTimeout);
        String path = storageInfoPath;
        storageInfoId = 0;
        storageInfoPath = null;
        callbacks.storageInfo(path, info == null ? -1 : info.total,
                info == null ? -1 : info.free, error);
        handler.postDelayed(storageInfoNext, 150);
    }

    void listFiles(String path) {
        if (!connected || path == null || !(path.equals("/int") || path.startsWith("/int/")
                || path.equals("/ext") || path.startsWith("/ext/"))) return;
        if (!rpcReady) { deferredFilesPath = path; return; }
        if (inventoryRunning) finishInventory(false, "Inventory paused for file browsing");
        if (fileId != 0) finishFiles(false, "New directory selected");
        filePath = path;
        fileItems.clear();
        fileId = ++commandId;
        handler.postDelayed(fileTimeout, 12000);
        queuePacket(FlipperRpc.listRequest(fileId, filePath));
    }

    private void finishFiles(boolean success, String reason) {
        if (fileId == 0) return;
        handler.removeCallbacks(fileTimeout);
        String path = filePath;
        filePath = null;
        fileId = 0;
        callbacks.files(path, new ArrayList<>(fileItems), success, reason);
        fileItems.clear();
    }

    boolean isRpcReady() { return connected && rpcReady; }

    void readFile(String path) {
        if (!rpcReady || readId != 0 || path == null || !path.startsWith("/")) {
            callbacks.fileData(path, null, "Flipper file read unavailable");
            return;
        }
        if (inventoryRunning) finishInventory(false, "Inventory paused for file download");
        readPath = path;
        readId = ++commandId;
        readContents.reset();
        // Un fichier mailbox n'est pas un long-poll : eviter 30 min de RPC bloque.
        handler.postDelayed(readTimeout, path.equals(NetworkMailbox.REQUEST) ? 20000 : 30000);
        queuePacket(FlipperRpc.readRequest(readId, path));
    }
    private void finishRead(String error) {
        if (readId == 0) return;
        handler.removeCallbacks(readTimeout);
        callbacks.fileData(readPath, error == null ? readContents.toByteArray() : null, error);
        readPath = null;
        readId = 0;
        readContents.reset();
    }

    /** Explicit Settings refresh retries RPC firmware-version discovery even
     *  if the initial connection's device-info retries were exhausted. */
    void refreshDeviceInfo() {
        if (!rpcReady) return;
        handler.removeCallbacks(deviceInfoRetry);
        if (deviceInfoId != 0) {
            // Do not drop a user-requested refresh because another device-info
            // request was already active during the swipe.
            deviceInfoRefreshQueued = true;
            return;
        }
        deviceInfoRetries = 0;
        firmwareVersionSeen = false;
        requestDeviceInfo();
    }

    void requestDeviceInfo() {
        if (!rpcReady || deviceInfoId != 0 || deviceInfoRetries >= 3) return;
        deviceInfoRetries++;
        deviceInfoId = ++commandId;
        handler.removeCallbacks(deviceInfoTimeout);
        handler.postDelayed(deviceInfoTimeout, 6000);
        queuePacket(FlipperRpc.deviceInfoRequest(deviceInfoId));
    }
    void launch(String verifiedPath) {
        if (!rpcReady || operationId != 0 || verifiedPath == null) {
            callbacks.operation("Launch", false, "Flipper RPC unavailable or another operation is running");
            return;
        }
        if (inventoryRunning) finishInventory(false, "Inventory paused while starting an app");
        try { beginOperation("Launch", FlipperRpc.launchRequest(++commandId, verifiedPath)); }
        catch (IllegalArgumentException badPath) {
            callbacks.operation("Launch", false, "Invalid installed .fap path");
        }
    }
    void launchWeaveInstallScreen() {
        if (!rpcReady || operationId != 0 || uploading != null) {
            callbacks.operation("Remote Install Screen", false, "Flipper is busy");
            return;
        }
        beginOperation("Remote Install Screen", FlipperRpc.weaveInstallScreenRequest(++commandId));
    }
    void mkdir(String path) { beginOperation("New folder", FlipperRpc.mkdirRequest(++commandId, path)); }
    void firmwareMkdir(String path, String label) {
        beginOperation(label, FlipperRpc.mkdirRequest(++commandId, path));
    }
    void prepareFirmwareUpdate(String manifest) {
        try { beginOperation("Firmware Prepare", FlipperRpc.systemUpdateRequest(++commandId, manifest)); }
        catch (IllegalArgumentException invalid) {
            callbacks.operation("Firmware Prepare", false, invalid.getMessage());
        }
    }
    void rebootForFirmwareUpdate() {
        if (!rpcReady || operationId != 0 || uploading != null) {
            callbacks.operation("Firmware Reboot", false, "Flipper RPC is busy");
            return;
        }
        // The Flipper normally disconnects before it can acknowledge a reboot.
        queuePacket(FlipperRpc.rebootUpdateRequest(++commandId));
        callbacks.operation("Firmware Reboot", true, "Reboot sent; waiting for new firmware");
    }
    void delete(String path, boolean recursive) {
        beginOperation("Delete", FlipperRpc.deleteRequest(++commandId, path, recursive));
    }
    void rename(String from, String to) {
        beginOperation("Rename", FlipperRpc.renameRequest(++commandId, from, to));
    }
    private void beginOperation(String label, byte[] request) {
        if (!rpcReady || operationId != 0) {
            callbacks.operation(label, false, "Flipper RPC not ready or another operation is running");
            return;
        }
        operationName = label;
        operationId = commandId;
        handler.postDelayed(operationTimeout, 18000);
        queuePacket(request);
    }
    private void finishOperation(boolean success, String message) {
        if (operationId == 0) return;
        handler.removeCallbacks(operationTimeout);
        handler.removeCallbacks(cancelUploadDeadline);
        String kind = operationName;
        operationId = 0;
        operationName = null;
        if (uploading != null) {
            handler.removeCallbacks(uploadNextFrame);
            uploading = null;
            uploadTarget = null;
            uploadNextOffset = 0;
            uploadServerAcked = false;
            uploadCancelRequested = false;
            uploadCancelFrameQueued = false;
        }
        if ("Install".equals(kind) || "Import".equals(kind)) transferRetried = false;
        callbacks.operation(kind, success, message);
    }

    @SuppressLint("MissingPermission")
    String connectedDeviceKey() {
        if (isUsbActive()) return usbTransport.deviceKey();
        BluetoothGatt current = gatt;
        if (current == null || !connected) return null;
        try { return current.getDevice().getAddress(); }
        catch (SecurityException denied) { return null; }
    }

    void upload(String path, byte[] data) { uploadFile(path, data, "Install"); }
    void uploadFile(String path, byte[] data, String label) {
        if (!rpcReady || operationId != 0 || data == null || data.length == 0 || data.length > (label != null && label.equals("Firmware Upload") ? 16_000_000 :
                NetworkMailbox.RESPONSE.equals(path) ? NetworkMailbox.MAX_BODY + 100 : 2_000_000)
                || path == null || !(path.startsWith("/ext/") || path.startsWith("/int/"))) {
            callbacks.operation(label, false, "RPC unavailable or invalid file");
            return;
        }
        uploading = data;
        uploadTarget = path;
        operationName = label;
        operationId = ++commandId;
        uploadTotalFrames = (data.length + UPLOAD_RPC_BYTES - 1) / UPLOAD_RPC_BYTES;
        uploadSentFrames = 0;
        uploadNextOffset = 0;
        uploadServerAcked = false;
        uploadCancelRequested = false;
        uploadCancelFrameQueued = false;
        lastProgressSent = -1;
        if (inventoryRunning) finishInventory(false, "Inventory paused during file upload");
        // The stream is restored after the final RPC acknowledgment, not while streaming data.
        ++screenRestartGeneration;
        screenRestartPending = false;
        handler.removeCallbacks(screenInitialTimeout);
        screenSessionActive = false;
        queuePacket(FlipperRpc.streamRequest(++commandId, false));
        handler.postDelayed(operationTimeout, ("Firmware Upload".equals(label) ||
                NetworkMailbox.RESPONSE.equals(path)) ? 1800000 : 300000);
        // Never enqueue the whole file: generate each frame when the previous one left GATT.
        handler.postDelayed(uploadNextFrame, 75);
    }

    // Terminer le RPC sur la SD avant de nettoyer le fichier temporaire.
    boolean cancelCatalogUpload() {
        if (uploading == null || operationId == 0 || !"Install".equals(operationName)) return false;
        uploadCancelRequested = true;
        handler.removeCallbacks(uploadNextFrame);
        handler.post(uploadNextFrame);
        handler.removeCallbacks(cancelUploadDeadline);
        handler.postDelayed(cancelUploadDeadline, 8000);
        return true;
    }

    private void queueNextUploadFrame() {
        if (uploading == null || operationId == 0 || !connected || !rpcReady
                || writeBusy || pendingWrite != null || !outgoing.isEmpty()
                || !interactiveOutgoing.isEmpty() || heldInputMask != 0) return;
        if (uploadCancelRequested) {
            if (!uploadCancelFrameQueued) {
                uploadCancelFrameQueued = true;
                queuePacket(FlipperRpc.writeRequest(operationId, uploadTarget,
                        new byte[0], 0, 0, false));
            }
            return;
        }
        if (uploadNextOffset >= uploading.length) return;
        int length = Math.min(UPLOAD_RPC_BYTES, uploading.length - uploadNextOffset);
        int offset = uploadNextOffset;
        uploadNextOffset += length;
        queuePacket(FlipperRpc.writeRequest(operationId, uploadTarget, uploading,
                offset, length, uploadNextOffset < uploading.length));
    }

    /** Ask the connected Flipper for the REAL contents of /ext/apps via protobuf RPC. */
    /** Background RPC must never enter a PRESS/SHORT/RELEASE sequence. */
    boolean isIdleForShop() {
        return connected && rpcReady && operationId == 0 && readId == 0 && fileId == 0
                && storageInfoId == 0 && !inventoryRunning && uploading == null && pendingWrite == null
                && !keySequenceRunning && keyQueue.isEmpty() && heldInputMask == 0
                && outgoing.isEmpty() && interactiveOutgoing.isEmpty() && !writeBusy;
    }

    void requestInventory() {
        if (!connected || !rpcReady || inventoryRunning) return;
        inventoryRunning = true;
        inventoryPaths.clear(); inventoryVisited.clear(); inventoryNames.clear(); inventoryFullPaths.clear();
        inventoryPaths.add("/ext/apps");
        requestNextPath();
    }

    private void requestNextPath() {
        handler.removeCallbacks(inventoryTimeout);
        if (!inventoryRunning || !connected) return;
        if (inventoryPaths.isEmpty()) {
            finishInventory(true, null);
            return;
        }
        pendingPath = inventoryPaths.poll();
        inventoryVisited.add(pendingPath);
        replyItems.clear();
        pendingId = ++commandId;
        handler.postDelayed(inventoryTimeout, 12000);
        queuePacket(FlipperRpc.listRequest(pendingId, pendingPath));
    }

    @SuppressLint("MissingPermission")
    private synchronized void sendNextChunk() {
        if (isUsbActive()) {
            if (writeBusy) return;
            byte[] frame = interactiveOutgoing.pollFirst();
            if (frame == null) frame = outgoing.pollFirst();
            if (frame == null) return;
            writeBusy = true;
            if (uploading != null) uploadSentFrames++;
            usbTransport.writeAsync(frame, ok -> handler.post(() -> afterUsbWrite(ok)));
            return;
        }
        if (writeBusy || !connected || gatt == null || rpcWrite == null || credits <= 0) return;
        if (pendingWrite == null) {
            pendingWrite = interactiveOutgoing.pollFirst();
            if (pendingWrite == null) pendingWrite = outgoing.pollFirst();
            pendingOffset = 0;
        }
        if (pendingWrite == null) return;
        int length = Math.min(Math.min(writeChunkSize, credits), pendingWrite.length - pendingOffset);
        if (length <= 0) return;
        byte[] chunk = new byte[length];
        System.arraycopy(pendingWrite, pendingOffset, chunk, 0, length);
        try {
            boolean sent;
            if (android.os.Build.VERSION.SDK_INT >= 33)
                sent = gatt.writeCharacteristic(rpcWrite, chunk, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == 0;
            else {
                rpcWrite.setWriteType(BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT);
                rpcWrite.setValue(chunk);
                sent = gatt.writeCharacteristic(rpcWrite);
            }
            if (sent) {
                pendingWriteStartedAt = android.os.SystemClock.uptimeMillis();
                handler.removeCallbacks(gattWriteTimeout);
                handler.postDelayed(gattWriteTimeout, GATT_WRITE_TIMEOUT_MS);
                rpcHealth.sent(length, outgoing.size() + interactiveOutgoing.size());
                reportHealthIfDue();
                writeBusy = true;
                credits -= length;
                pendingOffset += length;
                if (pendingOffset == pendingWrite.length) {
                    pendingWrite = null;
                    if (uploading != null) uploadSentFrames++;
                }
            } else {
                rpcHealth.failedWrite();
                finishInventory(false, "Flipper RPC send failed");
                finishFiles(false, "Flipper RPC send failed");
                finishOperation(false, "Flipper RPC send failed");
            }
        } catch (RuntimeException error) {
            rpcHealth.failedWrite();
            finishInventory(false, "Flipper RPC unavailable");
            finishFiles(false, "Flipper RPC unavailable");
            finishOperation(false, "Flipper RPC unavailable");
        }
    }

    private synchronized void onRpcBytes(BluetoothGatt source, BluetoothGattCharacteristic characteristic, byte[] data) {
        if (source != gatt || !RPC_INDICATE.equals(characteristic.getUuid()) || data == null) return;
        acceptRpcBytes(data);
    }

    private synchronized void acceptRpcBytes(byte[] data) {
        if (data == null || data.length == 0) return;
        rpcHealth.received(data.length);
        if (rpcInput.size() + data.length > MAX_RPC_BUFFER_BYTES) {
            rpcInput.reset();
            rpcHealth.failedWrite();
            callbacks.status("RPC frame overflow: discarded incomplete response");
            finishRead("RPC frame overflow");
            finishFiles(false, "RPC frame overflow");
            finishInventory(false, "RPC frame overflow");
            return;
        }
        rpcInput.write(data, 0, data.length);
        byte[] bytes = rpcInput.toByteArray();
        int offset = 0;
        try {
            while (offset < bytes.length) {
                int[] prefix = {0};
                int length = FlipperRpc.frameLength(bytes, offset, prefix);
                if (length < 0 || bytes.length - offset - prefix[0] < length) break;
                byte[] payload = new byte[length];
                System.arraycopy(bytes, offset + prefix[0], payload, 0, length);
                offset += prefix[0] + length;
                processRpcReply(payload);
            }
            if (offset > 0) {
                rpcInput.reset();
                rpcInput.write(bytes, offset, bytes.length - offset);
            }
        } catch (IOException error) {
            rpcInput.reset();
            callbacks.status("Invalid RPC frame: " + error.getMessage());
            finishRead("Invalid Flipper RPC response");
            finishFiles(false, "Invalid Flipper RPC response");
            finishInventory(false, "Invalid Flipper RPC response");
        }
    }

    private void processRpcReply(byte[] frame) throws IOException {
        FlipperRpc.Reply raw = FlipperRpc.parseReply(frame);
        if (raw.type == FlipperRpc.GUI_FRAME) {
            // Frames from the old GUI session can arrive after STOP was queued.
            // Do not accept them as proof that a forced refresh succeeded.
            if (screenRestartPending || !screenStreamEnabled || !screenSessionActive) return;
            byte[] bitmap = FlipperRpc.parseScreen(raw.content);
            if (bitmap.length == 1024) {
                screenFrameSeen = true;
                rpcHealth.screen();
                lastScreenFrameAt = android.os.SystemClock.uptimeMillis();
                if (lastInteractiveInputAt > 0) {
                    rpcHealth.inputToFrame(lastScreenFrameAt - lastInteractiveInputAt);
                    lastInteractiveInputAt = 0;
                }
                screenInputsAwaitingFrame = 0;
                screenRecoveryAttempted = false;
                handler.removeCallbacks(screenInitialTimeout);
                callbacks.screen(bitmap);
            }
            return;
        }
        if (raw.commandId == deviceInfoId && deviceInfoId != 0) {
            if (raw.status == 0 && raw.type == FlipperRpc.SYSTEM_DEVICE_INFO_RESPONSE) {
                String[] prop = FlipperRpc.deviceInfoPair(raw.content);
                if ("firmware_api_major".equals(prop[0])) {
                    try { apiMajor = Integer.parseInt(prop[1]); } catch (Exception ignored) { }
                } else if ("firmware_api_minor".equals(prop[0])) {
                    try { apiMinor = Integer.parseInt(prop[1]); } catch (Exception ignored) { }
                } else if ("firmware_version".equals(prop[0])) {
                    if (prop[1] != null && prop[1].matches(".*[0-9]+\\.[0-9]+\\.[0-9]+.*")) {
                        firmwareVersionSeen = true;
                        callbacks.information("firmware", prop[1]);
                    }
                }
                if (apiMajor > 0 && apiMinor >= 0)
                    callbacks.information("api_version", apiMajor + "." + apiMinor);
            }
            if (!raw.hasNext) {
                handler.removeCallbacks(deviceInfoTimeout);
                deviceInfoId = 0;
                finishDeviceInfoRequest();
            }
            return;
        }
        if (raw.commandId == storageInfoId && storageInfoId != 0) {
            if (raw.hasNext) return;
            if (raw.status != 0 || raw.type != FlipperRpc.STORAGE_INFO_RESPONSE) {
                finishStorageInfo(null, "Flipper storage error " + raw.status);
            } else {
                try { finishStorageInfo(FlipperRpc.parseStorageInfo(raw.content), null); }
                catch (IOException badInfo) { finishStorageInfo(null, badInfo.getMessage()); }
            }
            return;
        }
        if (raw.commandId == fileId && fileId != 0) {
            if (raw.status != 0) { finishFiles(false, "Flipper directory error " + raw.status); return; }
            if (raw.type == FlipperRpc.STORAGE_LIST_RESPONSE)
                fileItems.addAll(FlipperRpc.parseFiles(raw.content));
            if (!raw.hasNext) finishFiles(true, null);
            return;
        }
        if (raw.commandId == readId && readId != 0) {
            if (raw.status != 0) { finishRead("Flipper file read error " + raw.status); return; }
            if (raw.type == FlipperRpc.STORAGE_READ_RESPONSE) {
                byte[] data = FlipperRpc.readFileData(raw.content);
                int transferLimit = NetworkMailbox.REQUEST.equals(readPath)
                        ? NetworkMailbox.MAX_BODY + 300 : 2_000_000;
                if (readContents.size() + data.length > transferLimit) {
                    finishRead("File exceeds RPC transfer limit"); return;
                }
                readContents.write(data, 0, data.length);
            }
            if (!raw.hasNext) finishRead(null);
            return;
        }
        if (raw.commandId == operationId && operationId != 0) {
            // The firmware normally replies only after the last write request. An early
            // storage error must still stop the transfer immediately.
            if (uploading != null && raw.status == 0 &&
                    ((uploadCancelRequested && !uploadCancelFrameQueued) ||
                     (!uploadCancelRequested && uploadNextOffset < uploading.length) ||
                     writeBusy || pendingWrite != null || !outgoing.isEmpty() || !interactiveOutgoing.isEmpty())) {
                if (uploadCancelFrameQueued || uploadNextOffset == uploading.length) uploadServerAcked = true;
                return;
            }
            boolean launched = raw.status == 0 && "Launch".equals(operationName);
            boolean wasUpload = uploading != null;
            boolean validFirmware = true;
            String firmwareError = null;
            if ("Firmware Prepare".equals(operationName)) {
                try {
                    int code = FlipperRpc.systemUpdateCode(raw.content);
                    validFirmware = raw.type == FlipperRpc.SYSTEM_UPDATE_RESPONSE && code == 0;
                    if (!validFirmware) firmwareError = "Flipper refused update preparation (code " + code + ")";
                } catch (IOException invalid) {
                    validFirmware = false;
                    firmwareError = "Invalid firmware update response";
                }
            }
            finishOperation(raw.status == 0 && !uploadCancelRequested && validFirmware,
                    uploadCancelRequested ? "Cancelled" : firmwareError != null ? firmwareError :
                    raw.status == 0 ? null : (raw.status == 15 ?
                    "Flipper RPC error 15: invalid parameters (" + operationName + ")" :
                    "Flipper RPC error " + raw.status));
            if (wasUpload && connected && rpcReady && screenStreamEnabled)
                handler.postDelayed(this::startScreen, 500);
            // Starting a .fap changes the foreground view. Re-register the global
            // GUI framebuffer stream after its loader has returned; never disconnect BLE.
            if (launched && screenStreamEnabled) handler.postDelayed(this::restartScreenStream, 900);
            return;
        }
        if (!inventoryRunning) return;
        FlipperRpc.ListReply response = FlipperRpc.parseListReply(frame);
        if (response.commandId != pendingId) return;
        if (response.status != 0) {
            finishInventory(false, "Flipper storage listing rejected (" + response.status + ")");
            return;
        }
        replyItems.addAll(response.items);
        if (response.hasNext) return;
        for (FlipperRpc.Item item : replyItems) {
            if (".".equals(item.name) || "..".equals(item.name) || item.name.contains("/")) continue;
            if (item.directory && inventoryVisited.size() + inventoryPaths.size() < 100
                    && pendingPath.split("/").length < 6) {
                String path = pendingPath + "/" + item.name;
                if (!inventoryVisited.contains(path) && !inventoryPaths.contains(path)) inventoryPaths.add(path);
            } else if (!item.directory && item.name.toLowerCase(java.util.Locale.ROOT).endsWith(".fap")) {
                String appName = item.name.substring(0, item.name.length() - 4);
                inventoryNames.add(appName);
                inventoryFullPaths.put(appName, pendingPath + "/" + item.name);
            }
        }
        handler.post(this::requestNextPath);
    }

    private void finishInventory(boolean complete, String message) {
        if (!inventoryRunning) return;
        inventoryRunning = false;
        handler.removeCallbacks(inventoryTimeout);
        pendingPath = null;
        callbacks.inventory(new ArrayList<>(inventoryNames),
                new java.util.HashMap<>(inventoryFullPaths), complete, message);
    }

    @SuppressLint("MissingPermission")
    private void disconnectGatt() {
        ++screenRestartGeneration;
        screenRestartPending = false;
        handler.post(() -> { keyQueue.clear(); keySequenceRunning = false; });
        handler.removeCallbacks(connectTimeout);
        handler.removeCallbacks(inventoryTimeout);
        handler.removeCallbacks(fileTimeout);
        handler.removeCallbacks(readTimeout);
        handler.removeCallbacks(operationTimeout);
        handler.removeCallbacks(uploadNextFrame);
        handler.removeCallbacks(cancelUploadDeadline);
        handler.removeCallbacks(screenInitialTimeout);
        handler.removeCallbacks(screenInputWatchdog);
        handler.removeCallbacks(deviceInfoTimeout);
        handler.removeCallbacks(rpcHandshakeTimeout);
        handler.removeCallbacks(storageInfoTimeout);
        handler.removeCallbacks(storageInfoNext);
        handler.removeCallbacks(gattWriteTimeout);
        storageInfoId = 0;
        storageInfoPath = null;
        storageInfoQueue.clear();
        screenFrameSeen = false;
        screenRecoveryAttempted = false;
        lastInteractiveInputAt = 0;
        pendingWriteStartedAt = 0;
        if (inventoryRunning) finishInventory(false, "Flipper disconnected while reading apps");
        finishFiles(false, "Flipper disconnected");
        finishRead("Flipper disconnected");
        finishOperation(false, "Flipper disconnected");
        outgoing.clear(); interactiveOutgoing.clear(); pendingWrite = null; pendingOffset = 0;
        heldInputMask = 0;
        screenSessionActive = false;
        writeBusy = false; credits = 0; flowSubscribed = false;
        rpcReady = false; rpcSubscribing = false; rpcWrite = null; rpcFlow = null;
        handler.removeCallbacks(deviceInfoRetry);
        deviceInfoId = 0; apiMajor = -1; apiMinor = -1; deviceInfoRetries = 0;
        firmwareVersionSeen = false; deviceInfoRefreshQueued = false;
        rpcInput.reset();
        BluetoothGatt old = gatt;
        gatt = null;
        connecting = false;
        boolean wasConnected = connected;
        connected = false;
        synchronized (this) { reads.clear(); reading = false; }
        if (old != null) {
            try { old.disconnect(); } catch (RuntimeException ignored) { }
            try { old.close(); } catch (RuntimeException ignored) { }
        }
        if (wasConnected) callbacks.connected(false);
    }

    @Override public void close() {
        stopScan();
        if (retryTransferBytes != null) {
            String label = retryTransferLabel;
            clearRetryTransfer();
            callbacks.operation(label, false, "Upload canceled: Bluetooth link closed");
        }
        transferRetried = false;
        disconnectGatt();
    }
}
