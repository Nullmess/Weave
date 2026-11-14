package io.github.nullmess.weave;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;

/** Optional SMS_RECEIVED fallback. Never accesses PDUs, phone numbers or text. */
public final class SmsAlertRelay extends BroadcastReceiver {
    private static final String LAST_SMS = "last_sms_alert_event_ms";
    static synchronized boolean shouldRelay(SharedPreferences prefs) {
        long now = System.currentTimeMillis();
        long last = prefs.getLong(LAST_SMS, 0L);
        if(now - last >= 0 && now - last < 2500L) return false;
        return prefs.edit().putLong(LAST_SMS, now).commit();
    }
    @Override public void onReceive(Context context, Intent intent) {
        if(intent == null || !"android.provider.Telephony.SMS_RECEIVED".equals(intent.getAction())) return;
        SharedPreferences prefs = context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE);
        if(!prefs.getBoolean(WeaveService.KEY_ENABLED, false) ||
                !prefs.getBoolean("alerts_bridge_enabled", false) || !shouldRelay(prefs)) return;
        Intent command = new Intent(context, WeaveService.class)
                .setAction(WeaveService.ACTION_ALERT_EVENT).putExtra("type", "sms");
        try { context.startService(command); }
        catch(RuntimeException exception) {
            if(Build.VERSION.SDK_INT >= 26) {
                try { context.startForegroundService(command); }
                catch(RuntimeException ignored) { }
            }
        }
    }
}
