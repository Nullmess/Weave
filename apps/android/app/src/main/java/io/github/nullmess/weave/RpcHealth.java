package io.github.nullmess.weave;

import java.util.Arrays;

/** Diagnostic RPC local, sans trafic BLE ni donnees personnelles. */
final class RpcHealth {
    private final long[] samples = new long[64];
    private int sampleCount, nextSample;
    private final long[] inputSamples = new long[64];
    private int inputCount, nextInputSample;
    private long sentBytes, receivedBytes, screenFrames, failedWrites, reconnects;
    private int peakQueue;

    synchronized void sent(int bytes, int queueDepth) {
        sentBytes += Math.max(0, bytes);
        peakQueue = Math.max(peakQueue, queueDepth);
    }

    synchronized void received(int bytes) { receivedBytes += Math.max(0, bytes); }
    synchronized void screen() { screenFrames++; }
    synchronized void failedWrite() { failedWrites++; }
    synchronized void reconnect() { reconnects++; }

    synchronized void writeLatency(long millis) {
        if (millis < 0) return;
        samples[nextSample] = millis;
        nextSample = (nextSample + 1) % samples.length;
        if (sampleCount < samples.length) sampleCount++;
    }

    synchronized void inputToFrame(long millis) {
        if (millis < 0 || millis > 30000) return;
        inputSamples[nextInputSample] = millis;
        nextInputSample = (nextInputSample + 1) % inputSamples.length;
        if (inputCount < inputSamples.length) inputCount++;
    }

    synchronized String summary() {
        long[] ordered = Arrays.copyOf(samples, sampleCount);
        Arrays.sort(ordered);
        long p95 = sampleCount == 0 ? 0 : ordered[(int)Math.ceil(sampleCount * .95) - 1];
        long[] inputs = Arrays.copyOf(inputSamples, inputCount);
        Arrays.sort(inputs);
        long inputP95 = inputCount == 0 ? 0 : inputs[(int)Math.ceil(inputCount * .95) - 1];
        return "tx=" + sentBytes + " rx=" + receivedBytes + " frames=" + screenFrames
                + " writeP95Ms=" + p95 + " inputFrameP95Ms=" + inputP95 + " queuePeak=" + peakQueue
                + " writeErrors=" + failedWrites + " reconnects=" + reconnects;
    }
}
