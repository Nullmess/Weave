package io.github.nullmess.weave;

import android.content.Context;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.function.Consumer;

/** Official Flipper USB CDC-ACM CLI switched to length-delimited protobuf RPC.
 *  No custom interface, WV1 handshake, or unsupported USB descriptor required.
 */
final class UsbLink implements AutoCloseable {
    interface Callbacks {
        void status(String message);
        void ready();
        void bytes(byte[] data);
        void disconnected();
    }

    private static final int VID = 0x0483, PID = 0x5740;
    private static final int PROBE_ID = 714;
    private final UsbManager manager;
    private final Callbacks callbacks;
    private final ExecutorService writer = Executors.newSingleThreadExecutor();
    private final Object writeLock = new Object();
    private volatile UsbDeviceConnection connection;
    private UsbInterface dataInterface, controlInterface;
    private UsbEndpoint inbound, outbound;
    private volatile boolean running, ready;
    private Thread reader;
    private String connectedName;

    UsbLink(Context context, Callbacks callbacks) {
        manager = (UsbManager)context.getSystemService(Context.USB_SERVICE);
        this.callbacks = callbacks;
    }

    static boolean isFlipper(UsbDevice device) {
        return device != null && device.getVendorId() == VID && device.getProductId() == PID;
    }

    private static UsbInterface dataInterface(UsbDevice device) {
        if (!isFlipper(device)) return null;
        for (int i = 0; i < device.getInterfaceCount(); i++) {
            UsbInterface iface = device.getInterface(i);
            if (iface.getInterfaceClass() != UsbConstants.USB_CLASS_CDC_DATA) continue;
            boolean in = false, out = false;
            for (int e = 0; e < iface.getEndpointCount(); e++) {
                UsbEndpoint ep = iface.getEndpoint(e);
                if (ep.getType() != UsbConstants.USB_ENDPOINT_XFER_BULK) continue;
                if (ep.getDirection() == UsbConstants.USB_DIR_IN) in = true;
                if (ep.getDirection() == UsbConstants.USB_DIR_OUT) out = true;
            }
            if (in && out) return iface;
        }
        return null;
    }

    List<UsbDevice> discover() {
        List<UsbDevice> devices = new ArrayList<>();
        if (manager != null) for (UsbDevice device : manager.getDeviceList().values())
            if (dataInterface(device) != null) devices.add(device);
        return devices;
    }

    UsbManager getManager() { return manager; }
    boolean isOpen() { return running; }
    boolean isReady() { return ready && running; }
    boolean isSameDevice(UsbDevice device) {
        return device != null && running && device.getDeviceName().equals(connectedName);
    }
    String deviceKey() { return connectedName == null ? null : "USB:" + connectedName; }

    synchronized boolean connect(UsbDevice device) {
        if (isSameDevice(device)) return true;
        close();
        UsbInterface data = dataInterface(device);
        if (manager == null || data == null || !manager.hasPermission(device)) return false;
        UsbDeviceConnection opened = manager.openDevice(device);
        if (opened == null) return false;
        if (!opened.claimInterface(data, true)) { opened.close(); return false; }
        UsbInterface control = null;
        for (int i = 0; i < device.getInterfaceCount(); i++) {
            UsbInterface iface = device.getInterface(i);
            if (iface.getInterfaceClass() == UsbConstants.USB_CLASS_COMM) {
                control = iface;
                break;
            }
        }
        if (control != null) opened.claimInterface(control, true);
        UsbEndpoint in = null, out = null;
        for (int i = 0; i < data.getEndpointCount(); i++) {
            UsbEndpoint ep = data.getEndpoint(i);
            if (ep.getType() != UsbConstants.USB_ENDPOINT_XFER_BULK) continue;
            if (ep.getDirection() == UsbConstants.USB_DIR_IN) in = ep;
            if (ep.getDirection() == UsbConstants.USB_DIR_OUT) out = ep;
        }
        if (in == null || out == null) {
            if (control != null) opened.releaseInterface(control);
            opened.releaseInterface(data);
            opened.close();
            return false;
        }
        byte[] lineCoding = {0x00, (byte)0xc2, 0x01, 0, 0, 0, 8}; // 115200 8N1
        opened.controlTransfer(0x21, 0x20, 0, control == null ? 0 : control.getId(),
                lineCoding, lineCoding.length, 1000);
        opened.controlTransfer(0x21, 0x22, 3, control == null ? 0 : control.getId(),
                null, 0, 1000); // DTR and RTS
        dataInterface = data;
        controlInterface = control;
        inbound = in;
        outbound = out;
        connection = opened;
        connectedName = device.getDeviceName();
        running = true;
        ready = false;
        reader = new Thread(() -> readLoop(opened), "WeaveUsbRpc");
        reader.setDaemon(true);
        reader.start();
        callbacks.status("Flipper USB CDC detected — negotiating protobuf RPC");
        return true;
    }

    private void readLoop(UsbDeviceConnection owner) {
        ByteArrayOutputStream probeBuffer = new ByteArrayOutputStream();
        final byte[] packet = new byte[1024];
        long started = android.os.SystemClock.elapsedRealtime();
        long nextProbe = started + 500;
        try {
            // The stock CDC port boots into CLI mode. Switch it explicitly.
            if (!writeDirect("start_rpc_session\r".getBytes(java.nio.charset.StandardCharsets.US_ASCII)))
                return;
            while (running && connection == owner) {
                long now = android.os.SystemClock.elapsedRealtime();
                if (!ready && now - started > 12000) {
                    callbacks.status("Flipper USB RPC handshake timed out — using Bluetooth");
                    break;
                }
                if (!ready && now >= nextProbe) {
                    writeDirect(FlipperRpc.deviceInfoRequest(PROBE_ID));
                    nextProbe = now + 2200;
                }
                UsbDeviceConnection current = connection;
                if (current == null) break;
                int size = current.bulkTransfer(inbound, packet, packet.length, 250);
                if (size < 0) continue; // Timeout and disconnect are indistinguishable here;
                if (size == 0) continue; // the device-detached receiver resolves disconnect.
                if (ready) {
                    byte[] content = new byte[size];
                    System.arraycopy(packet, 0, content, 0, size);
                    callbacks.bytes(content);
                } else {
                    probeBuffer.write(packet, 0, size);
                    // CLI echo/prompt is not protobuf. Scan for a *valid* reply to our
                    // unique command ID, and ignore any earlier textual CLI output.
                    byte[] bytes = probeBuffer.toByteArray();
                    int remainingOffset = -1;
                    for (int offset = 0; offset < bytes.length; offset++) {
                        try {
                            int[] prefix = {0};
                            int count = FlipperRpc.frameLength(bytes, offset, prefix);
                            if (count <= 0 || count > 8192 ||
                                    bytes.length - offset - prefix[0] < count) continue;
                            byte[] content = new byte[count];
                            System.arraycopy(bytes, offset + prefix[0], content, 0, count);
                            FlipperRpc.Reply response = FlipperRpc.parseReply(content);
                            if (response.commandId == PROBE_ID && response.status == 0 &&
                                    response.type == FlipperRpc.SYSTEM_DEVICE_INFO_RESPONSE) {
                                remainingOffset = offset + prefix[0] + count;
                                break;
                            }
                        } catch (IOException | RuntimeException ignored) { }
                    }
                    if (remainingOffset >= 0) {
                        ready = true;
                        callbacks.ready();
                        if (remainingOffset < bytes.length) {
                            byte[] tail = new byte[bytes.length - remainingOffset];
                            System.arraycopy(bytes, remainingOffset, tail, 0, tail.length);
                            callbacks.bytes(tail);
                        }
                        probeBuffer.reset();
                    } else if (bytes.length > 8192) {
                        probeBuffer.reset();
                        probeBuffer.write(bytes, bytes.length - 256, 256);
                    }
                }
            }
        } catch (RuntimeException error) {
            callbacks.status("USB RPC connection lost");
        } finally {
            if (connection == owner) {
                close();
                callbacks.disconnected();
            }
        }
    }

    private boolean writeDirect(byte[] frame) {
        synchronized (writeLock) {
            UsbDeviceConnection current = connection;
            if (!running || current == null || outbound == null || frame == null) return false;
            int offset = 0;
            while (offset < frame.length) {
                int len = Math.min(512, frame.length - offset);
                byte[] data = new byte[len];
                System.arraycopy(frame, offset, data, 0, len);
                int sent = current.bulkTransfer(outbound, data, data.length, 1200);
                if (sent != len) return false;
                offset += sent;
            }
            return true;
        }
    }

    void writeAsync(byte[] frame, Consumer<Boolean> complete) {
        if (!isReady()) { complete.accept(false); return; }
        try {
            writer.execute(() -> complete.accept(writeDirect(frame)));
        } catch (java.util.concurrent.RejectedExecutionException ignored) {
            complete.accept(false);
        }
    }

    @Override public synchronized void close() {
        running = false;
        ready = false;
        UsbDeviceConnection old = connection;
        connection = null;
        connectedName = null;
        if (old != null) {
            try { if (dataInterface != null) old.releaseInterface(dataInterface); }
            catch (RuntimeException ignored) { }
            try { if (controlInterface != null) old.releaseInterface(controlInterface); }
            catch (RuntimeException ignored) { }
            old.close();
        }
        dataInterface = null;
        controlInterface = null;
        inbound = null;
        outbound = null;
    }

    void shutdown() { close(); writer.shutdownNow(); }
}
