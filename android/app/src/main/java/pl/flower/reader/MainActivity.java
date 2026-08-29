package pl.flower.reader;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.work.Constraints;
import androidx.work.ExistingPeriodicWorkPolicy;
import androidx.work.ExistingWorkPolicy;
import androidx.work.NetworkType;
import androidx.work.OneTimeWorkRequest;
import androidx.work.PeriodicWorkRequest;
import androidx.work.WorkManager;

import com.getcapacitor.BridgeActivity;

import java.util.concurrent.TimeUnit;

public class MainActivity extends BridgeActivity {
    private static final String UPDATE_CHECK_WORK_NAME = "flower_update_check";

    @Override
    public void onCreate(Bundle savedInstanceState) {
        registerPlugin(NetworkPinPlugin.class);
        registerPlugin(ShareTargetPlugin.class);
        super.onCreate(savedInstanceState);
        requestNotificationPermissionIfNeeded();
        scheduleUpdateCheck();
        handleShareIntent(getIntent(), false);
    }

    @Override
    public void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        setIntent(intent);
        handleShareIntent(intent, true);
    }

    private void handleShareIntent(Intent intent, boolean live) {
        if (intent == null || !Intent.ACTION_SEND.equals(intent.getAction())) {
            return;
        }
        String type = intent.getType();
        if (type == null || !type.startsWith("text/")) {
            return;
        }
        String text = intent.getStringExtra(Intent.EXTRA_TEXT);
        if (text == null || text.trim().isEmpty()) {
            return;
        }
        if (live) {
            ShareTargetPlugin.deliverLive(text);
        } else {
            ShareTargetPlugin.stashPending(text);
        }
    }

    private void requestNotificationPermissionIfNeeded() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.TIRAMISU) {
            return;
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(
                    this, new String[] {Manifest.permission.POST_NOTIFICATIONS}, 1001);
        }
    }

    private void scheduleUpdateCheck() {
        Constraints constraints = new Constraints.Builder()
                .setRequiredNetworkType(NetworkType.CONNECTED)
                .build();

        // PeriodicWorkRequest NIE odpala się od razu po enqueue — pierwsze
        // uruchomienie przychodzi dopiero po pełnym interwale (do 12h).
        // Bez osobnego jednorazowego requesta powiadomienie o aktualizacji
        // mogłoby się nie pojawić przez pół dnia od pierwszego uruchomienia
        // appki, co wygląda jak "to w ogóle nie działa" — dokładnie to
        // zgłosił Karol. UNIQUE + KEEP na jednorazowym też zapobiega
        // wielokrotnemu odpaleniu przy kolejnych onCreate() tej samej sesji.
        OneTimeWorkRequest immediateCheck =
                new OneTimeWorkRequest.Builder(UpdateCheckWorker.class)
                        .setConstraints(constraints)
                        .build();
        WorkManager.getInstance(this)
                .enqueueUniqueWork(
                        UPDATE_CHECK_WORK_NAME + "_immediate", ExistingWorkPolicy.KEEP, immediateCheck);

        PeriodicWorkRequest request =
                new PeriodicWorkRequest.Builder(UpdateCheckWorker.class, 12, TimeUnit.HOURS)
                        .setConstraints(constraints)
                        .build();
        WorkManager.getInstance(this)
                .enqueueUniquePeriodicWork(
                        UPDATE_CHECK_WORK_NAME, ExistingPeriodicWorkPolicy.KEEP, request);
    }
}
