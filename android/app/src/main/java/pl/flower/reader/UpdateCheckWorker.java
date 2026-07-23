package pl.flower.reader;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.work.Worker;
import androidx.work.WorkerParameters;

import org.json.JSONArray;
import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Sprawdza dwie niezależne rzeczy z tego samego release'a na GitHubie:
 *   1. Nową wersję firmware czytnika (tag_name) — czytnik i tak sam się
 *      aktualizuje gdy ma zapisane WiFi domowe (App.cpp, ota_auto), to tylko
 *      drugorzędne powiadomienie dla kogoś kto tego nie skonfigurował.
 *   2. Nową wersję samej appki (asset flower-android-vX.Y.Z.apk) — appka
 *      jest sideloadowana, więc Android nie sprawdza tego sam jak dla appek
 *      ze Sklepu Play.
 */
public class UpdateCheckWorker extends Worker {

    private static final String RELEASES_API =
            "https://api.github.com/repos/GRKarol/czytnik01/releases/latest";
    private static final String DOWNLOAD_PAGE_URL = "https://flower.theworkpc.com/appdownload.html";
    private static final String PREFS_NAME = "flower_update_check";
    private static final String KEY_LAST_SEEN_TAG = "last_seen_tag";
    private static final String KEY_LAST_SEEN_APP_VERSION = "last_seen_app_version";
    private static final String CHANNEL_ID = "flower_updates";
    private static final int FIRMWARE_NOTIFICATION_ID = 1001;
    private static final int APP_NOTIFICATION_ID = 1002;
    private static final Pattern APK_VERSION_PATTERN =
            Pattern.compile("flower-android-v(\\d+\\.\\d+\\.\\d+)\\.apk");

    public UpdateCheckWorker(@NonNull Context context, @NonNull WorkerParameters params) {
        super(context, params);
    }

    @NonNull
    @Override
    public Result doWork() {
        JSONObject release;
        try {
            release = fetchLatestRelease();
        } catch (Exception e) {
            return Result.retry();
        }
        if (release == null) {
            return Result.success();
        }

        checkFirmwareVersion(release.optString("tag_name", null));
        checkAppVersion(release.optJSONArray("assets"));
        return Result.success();
    }

    private void checkFirmwareVersion(String latestTag) {
        if (latestTag == null || latestTag.isEmpty()) return;

        SharedPreferences prefs =
                getApplicationContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        String lastSeenTag = prefs.getString(KEY_LAST_SEEN_TAG, null);

        // Pierwsze uruchomienie: zapamiętaj punkt odniesienia, nie powiadamiaj —
        // inaczej appka pokazałaby powiadomienie o release'ie który już jest
        // aktualny od dawna, tylko dlatego że worker jeszcze go nie widział.
        if (lastSeenTag == null) {
            prefs.edit().putString(KEY_LAST_SEEN_TAG, latestTag).apply();
            return;
        }
        if (!latestTag.equals(lastSeenTag)) {
            prefs.edit().putString(KEY_LAST_SEEN_TAG, latestTag).apply();
            showFirmwareNotification(latestTag);
        }
    }

    private void checkAppVersion(JSONArray assets) {
        String latestAppVersion = extractLatestAppVersion(assets);
        if (latestAppVersion == null) return;

        String installedVersion = getInstalledVersionName();
        if (installedVersion == null) return;

        SharedPreferences prefs =
                getApplicationContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        String lastSeenAppVersion = prefs.getString(KEY_LAST_SEEN_APP_VERSION, null);

        if (lastSeenAppVersion == null) {
            prefs.edit().putString(KEY_LAST_SEEN_APP_VERSION, installedVersion).apply();
            lastSeenAppVersion = installedVersion;
        }

        if (isNewerVersion(latestAppVersion, lastSeenAppVersion)
                && isNewerVersion(latestAppVersion, installedVersion)) {
            prefs.edit().putString(KEY_LAST_SEEN_APP_VERSION, latestAppVersion).apply();
            showAppUpdateNotification(latestAppVersion);
        }
    }

    /** Szuka najwyższej wersji wśród assetów pasujących do flower-android-vX.Y.Z.apk. */
    private String extractLatestAppVersion(JSONArray assets) {
        if (assets == null) return null;
        String best = null;
        for (int i = 0; i < assets.length(); i++) {
            JSONObject asset = assets.optJSONObject(i);
            if (asset == null) continue;
            String name = asset.optString("name", "");
            Matcher m = APK_VERSION_PATTERN.matcher(name);
            if (m.matches()) {
                String version = m.group(1);
                if (best == null || isNewerVersion(version, best)) {
                    best = version;
                }
            }
        }
        return best;
    }

    private String getInstalledVersionName() {
        try {
            PackageManager pm = getApplicationContext().getPackageManager();
            PackageInfo info = pm.getPackageInfo(getApplicationContext().getPackageName(), 0);
            return info.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return null;
        }
    }

    /** Proste porównanie "X.Y.Z" po numerach, bez zależności zewnętrznych. */
    private static boolean isNewerVersion(String a, String b) {
        String[] pa = a.split("\\.");
        String[] pb = b.split("\\.");
        int len = Math.max(pa.length, pb.length);
        for (int i = 0; i < len; i++) {
            int va = i < pa.length ? parseIntSafe(pa[i]) : 0;
            int vb = i < pb.length ? parseIntSafe(pb[i]) : 0;
            if (va != vb) return va > vb;
        }
        return false;
    }

    private static int parseIntSafe(String s) {
        try {
            return Integer.parseInt(s.trim());
        } catch (NumberFormatException e) {
            return 0;
        }
    }

    private JSONObject fetchLatestRelease() throws Exception {
        URL url = new URL(RELEASES_API);
        HttpURLConnection conn = (HttpURLConnection) url.openConnection();
        conn.setRequestProperty("Accept", "application/vnd.github+json");
        conn.setRequestProperty("User-Agent", "Flower-Reader-App");
        conn.setConnectTimeout(15000);
        conn.setReadTimeout(15000);
        try {
            if (conn.getResponseCode() != 200) {
                return null;
            }
            StringBuilder sb = new StringBuilder();
            try (BufferedReader reader = new BufferedReader(
                    new InputStreamReader(conn.getInputStream(), StandardCharsets.UTF_8))) {
                String line;
                while ((line = reader.readLine()) != null) {
                    sb.append(line);
                }
            }
            return new JSONObject(sb.toString());
        } finally {
            conn.disconnect();
        }
    }

    private NotificationChannel ensureChannel(NotificationManager manager) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = manager.getNotificationChannel(CHANNEL_ID);
            if (channel == null) {
                channel = new NotificationChannel(
                        CHANNEL_ID,
                        "Aktualizacje firmware",
                        NotificationManager.IMPORTANCE_DEFAULT);
                channel.setDescription("Powiadomienia o nowych wersjach firmware czytnika i appki Flower.");
                manager.createNotificationChannel(channel);
            }
            return channel;
        }
        return null;
    }

    private void showFirmwareNotification(String tag) {
        Context context = getApplicationContext();
        NotificationManager manager = context.getSystemService(NotificationManager.class);
        if (manager == null) return;
        ensureChannel(manager);

        Intent launchIntent = new Intent(context, MainActivity.class);
        launchIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                context, 0, launchIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder notification = new NotificationCompat.Builder(context, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.stat_sys_download_done)
                .setContentTitle("Nowy firmware: " + tag)
                .setContentText("Połącz się z czytnikiem (tryb Sync), żeby zainstalować.")
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setContentIntent(pendingIntent)
                .setAutoCancel(true);

        try {
            NotificationManagerCompat.from(context).notify(FIRMWARE_NOTIFICATION_ID, notification.build());
        } catch (SecurityException ignored) {
            // Brak zgody POST_NOTIFICATIONS (Android 13+) — po prostu nie pokazujemy powiadomienia.
        }
    }

    private void showAppUpdateNotification(String version) {
        Context context = getApplicationContext();
        NotificationManager manager = context.getSystemService(NotificationManager.class);
        if (manager == null) return;
        ensureChannel(manager);

        Intent viewIntent = new Intent(Intent.ACTION_VIEW, Uri.parse(DOWNLOAD_PAGE_URL));
        viewIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                context, 1, viewIntent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder notification = new NotificationCompat.Builder(context, CHANNEL_ID)
                .setSmallIcon(android.R.drawable.stat_sys_download_done)
                .setContentTitle("Nowa wersja appki: v" + version)
                .setContentText("Kliknij, żeby pobrać ze strony Flower.")
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setContentIntent(pendingIntent)
                .setAutoCancel(true);

        try {
            NotificationManagerCompat.from(context).notify(APP_NOTIFICATION_ID, notification.build());
        } catch (SecurityException ignored) {
            // Brak zgody POST_NOTIFICATIONS (Android 13+) — po prostu nie pokazujemy powiadomienia.
        }
    }
}
