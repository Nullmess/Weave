package io.github.nullmess.weave;

import android.content.Context;
import android.hardware.usb.UsbConstants;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbDeviceConnection;
import android.hardware.usb.UsbEndpoint;
import android.hardware.usb.UsbInterface;
import android.hardware.usb.UsbManager;

import java.nio.charset.StandardCharsets;

/** The native Flipper CLI is USB CDC ACM, not the protobuf BLE RPC channel. */
final class FlipperUsbCli implements AutoCloseable {
    interface Callbacks { void status(String message); void data(String text); }
    private final UsbManager usb;
    private final Callbacks callbacks;
    private UsbDeviceConnection connection;
    private UsbInterface dataInterface, controlInterface;
    private UsbEndpoint in, out;
    private Thread reader;
    private volatile boolean running;

    FlipperUsbCli(Context context, Callbacks callbacks) {
        usb = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        this.callbacks = callbacks;
    }
    static UsbDevice detect(Context context) {
        UsbManager usb = (UsbManager) context.getSystemService(Context.USB_SERVICE);
        if (usb == null) return null;
        for (UsbDevice device : usb.getDeviceList().values()) {
            if (device.getVendorId() != 0x0483 || device.getProductId() != 0x5740) continue;
            for (int i = 0; i < device.getInterfaceCount(); i++) {
                if (device.getInterface(i).getInterfaceClass() == 0x0a) return device;
            }
        }
        return null;
    }
    boolean isRunning() { return running; }
    synchronized boolean connect(UsbDevice device) {
        close();
        if (device == null || !usb.hasPermission(device)) { callbacks.status("USB permission required"); return false; }
        UsbInterface data = null, control = null;
        for (int i = 0; i < device.getInterfaceCount(); i++) {
            UsbInterface iface = device.getInterface(i);
            if (iface.getInterfaceClass() == 0x0a) data = iface;
            if (iface.getInterfaceClass() == UsbConstants.USB_CLASS_COMM) control = iface;
        }
        if (data == null) { callbacks.status("Flipper CDC serial interface not available"); return false; }
        UsbDeviceConnection candidate = usb.openDevice(device);
        if (candidate == null || !candidate.claimInterface(data, true)) {
            if (candidate != null) candidate.close();
            callbacks.status("Cannot open Flipper serial port"); return false;
        }
        connection = candidate;
        dataInterface = data;
        controlInterface = control;
        if (control != null) candidate.claimInterface(control, true);
        for (int i = 0; i < data.getEndpointCount(); i++) {
            UsbEndpoint endpoint = data.getEndpoint(i);
            if (endpoint.getType() != UsbConstants.USB_ENDPOINT_XFER_BULK) continue;
            if (endpoint.getDirection() == UsbConstants.USB_DIR_IN) in = endpoint;
            if (endpoint.getDirection() == UsbConstants.USB_DIR_OUT) out = endpoint;
        }
        if (in == null || out == null) { close(); callbacks.status("Flipper CDC endpoints unavailable"); return false; }
        int controlId = control == null ? 0 : control.getId();
        byte[] lineCoding = {0x00, (byte)0xc2, 0x01, 0x00, 0x00, 0x00, 0x08}; // 115200 8N1
        candidate.controlTransfer(0x21, 0x20, 0, controlId, lineCoding, 7, 1000);
        candidate.controlTransfer(0x21, 0x22, 3, controlId, null, 0, 1000); // DTR + RTS
        running = true;
        reader = new Thread(() -> {
            callbacks.status("Flipper USB CLI connected");
            byte[] dataBuffer = new byte[1024];
            while (running) {
                UsbDeviceConnection current = connection;
                if (current == null) break;
                int count;
                try { count = current.bulkTransfer(in, dataBuffer, dataBuffer.length, 300); }
                catch (RuntimeException error) { break; }
                if (count > 0) callbacks.data(new String(dataBuffer, 0, count, StandardCharsets.UTF_8));
            }
            callbacks.status("Flipper USB CLI disconnected");
        }, "WeaveFlipperCli");
        reader.start();
        return true;
    }
    boolean send(String command) {
        if (!running || connection == null || out == null || command == null || command.length() > 1024) return false;
        byte[] bytes = (command + "\r").getBytes(StandardCharsets.UTF_8);
        return connection.bulkTransfer(out, bytes, bytes.length, 2000) == bytes.length;
    }
    @Override public synchronized void close() {
        running = false;
        UsbDeviceConnection old = connection;
        connection = null;
        if (old != null) {
            try { if (dataInterface != null) old.releaseInterface(dataInterface); }
            catch (RuntimeException ignored) { }
            try { if (controlInterface != null) old.releaseInterface(controlInterface); }
            catch (RuntimeException ignored) { }
            old.close();
        }
        in = null; out = null; dataInterface = null; controlInterface = null;
    }
}
