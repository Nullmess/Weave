package io.github.nullmess.weave;

import android.Manifest;
import android.app.Notification;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import java.util.Locale;

/** Explicitly opted-in alert bridge. Only CALL / SMS / CALL_END events leave
 * the phone, never numbers, contacts, message text or caller information. */
@SuppressWarnings("deprecation")
public final class WeaveAlertListener extends NotificationListenerService {
    private static final String KEY_LAST_SMS = "last_sms_alert_event_ms";
    private boolean callForwarded;
    private TelephonyManager telephony;
    private PhoneStateListener callListener;

    private boolean enabled() {
        SharedPreferences prefs = getSharedPreferences(WeaveService.PREFS, MODE_PRIVATE);
        return prefs.getBoolean(WeaveService.KEY_ENABLED, false) &&
                prefs.getBoolean("alerts_bridge_enabled", false);
    }

    private void relay(String type) {
        if(!enabled() && !"call_end".equals(type)) return;
        Intent intent = new Intent(this, WeaveService.class)
                .setAction(WeaveService.ACTION_ALERT_EVENT).putExtra("type", type);
        try {
            // This is normally delivered to the already-running Weave foreground
            // service, including when MainActivity is in the background.
            startService(intent);
        } catch(RuntimeException error) {
            // Android 8+ may reject starting a stopped service from background.
            // If allowed, request foreground startup; never crash the listener.
            if(Build.VERSION.SDK_INT >= 26) {
                try { startForegroundService(intent); } catch(RuntimeException ignored) { }
            }
        }
    }

    private void startCall() {
        if(!enabled() || callForwarded) return;
        callForwarded = true;
        relay("call");
    }

    private void endCall() {
        if(!callForwarded) return;
        callForwarded = false;
        relay("call_end");
    }

    private boolean hasPhoneState() {
        return Build.VERSION.SDK_INT < 23 ||
                checkSelfPermission(Manifest.permission.READ_PHONE_STATE) ==
                PackageManager.PERMISSION_GRANTED;
    }

    private void ensureCallListener() {
        if(callListener != null || !hasPhoneState()) return;
        try {
            telephony = (TelephonyManager)getSystemService(TELEPHONY_SERVICE);
            if(telephony == null) return; // tablets without telephony
            callListener = new PhoneStateListener() {
                @Override public void onCallStateChanged(int state, String ignoredNumber) {
                    if(state == TelephonyManager.CALL_STATE_RINGING) startCall();
                    else if(state == TelephonyManager.CALL_STATE_OFFHOOK ||
                            state == TelephonyManager.CALL_STATE_IDLE) endCall();
                }
            };
            telephony.listen(callListener, PhoneStateListener.LISTEN_CALL_STATE);
        } catch(RuntimeException error) {
            callListener = null;
            telephony = null;
        }
    }

    @Override public void onListenerConnected() {
        super.onListenerConnected();
        ensureCallListener();
    }

    @Override public void onListenerDisconnected() {
        endCall();
        if(telephony != null && callListener != null) {
            try { telephony.listen(callListener, PhoneStateListener.LISTEN_NONE); }
            catch(RuntimeException ignored) { }
        }
        callListener = null;
        telephony = null;
        super.onListenerDisconnected();
    }

    private boolean isDialerCall(StatusBarNotification event) {
        if(event == null || event.getNotification() == null) return false;
        String pkg = event.getPackageName().toLowerCase(Locale.ROOT);
        return (pkg.equals("com.google.android.dialer") ||
                pkg.equals("com.android.dialer") ||
                pkg.equals("com.samsung.android.dialer") ||
                pkg.equals("com.samsung.android.incallui")) &&
                Notification.CATEGORY_CALL.equals(event.getNotification().category);
    }

    @Override public void onNotificationPosted(StatusBarNotification event) {
        if(!enabled() || event == null || event.getNotification() == null) return;
        ensureCallListener();
        if(isDialerCall(event)) {
            // A phone-state listener distinguishes RINGING from OFFHOOK.
            // Fall back to notification arrival if permission is unavailable.
            if(callListener == null) startCall();
            return;
        }
        String pkg = event.getPackageName().toLowerCase(Locale.ROOT);
        // Messaging providers vary; category is more reliable than the app name.
        // Forward only SMS/MMS notifications, not WhatsApp/Signal/other chats.
        boolean smsProvider = pkg.equals("com.google.android.apps.messaging") ||
                pkg.equals("com.samsung.android.messaging") ||
                pkg.equals("com.android.mms") || pkg.equals("com.android.messaging") ||
                pkg.equals("com.sonyericsson.conversations") ||
                pkg.equals("com.huawei.message") || pkg.equals("com.miui.mms");
        if(smsProvider && Notification.CATEGORY_MESSAGE.equals(event.getNotification().category) &&
                SmsAlertRelay.shouldRelay(getSharedPreferences(WeaveService.PREFS, MODE_PRIVATE))) {
            relay("sms");
        }
    }

    @Override public void onNotificationRemoved(StatusBarNotification event) {
        // Fallback: some dialers remove their incoming notification on answer.
        // Phone-state tracking remains authoritative when permission is granted.
        if(callListener == null && isDialerCall(event)) endCall();
    }
}
