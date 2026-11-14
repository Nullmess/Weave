package io.github.nullmess.weave;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.Manifest;
import android.content.pm.PackageManager;
import android.content.SharedPreferences;
import android.os.Build;

/** Explicitly opt-in service restart after Android finishes booting. */
public final class BootReceiver extends BroadcastReceiver {
    @Override public void onReceive(Context context, Intent intent) {
        if (intent == null || !Intent.ACTION_BOOT_COMPLETED.equals(intent.getAction())) return;
        SharedPreferences settings = context.getSharedPreferences(WeaveService.PREFS, Context.MODE_PRIVATE);
        if (!settings.getBoolean(WeaveService.KEY_BOOT, false)
                || !settings.getBoolean(WeaveService.KEY_ENABLED, false)) return;
        if (Build.VERSION.SDK_INT >= 31 &&
                (context.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED
                || context.checkSelfPermission(Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED)) {
            settings.edit().putString(WeaveService.KEY_STATUS,
                    "Auto-start needs Nearby devices permission — open Weave").apply();
            return;
        }
        Intent service = new Intent(context, WeaveService.class).setAction(WeaveService.ACTION_START);
        try {
            if (Build.VERSION.SDK_INT >= 26) context.startForegroundService(service);
            else context.startService(service);
        } catch (RuntimeException error) {
            settings.edit().putString(WeaveService.KEY_STATUS,
                    "Automatic startup blocked by Android — open Weave to restart").apply();
        }
    }
}
