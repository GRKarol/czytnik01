package pl.flower.reader;

import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;

import androidx.annotation.NonNull;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.work.Worker;
import androidx.work.WorkerParameters;

import org.json.JSONObject;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.charset.StandardCharsets;

/**
 * Czytnik sam sprawdza i instaluje aktualizacje firmware, gdy ma zapisane
 * WiFi domowe (App.cpp, ota_auto). Ten worker to niezależny, drugorzędny
 * mechanizm po stronie appki — dla przypadku gdy ktoś nie skonfigurował
 * WiFi na czytniku i chce wiedzieć, że jest nowy release, żeby zainstalować
 * go ręcznie przez tryb Sync.
 */
public class UpdateCheckWorker extends Worker {

    private static final String RELEASES_API =
            "https://api.github.com/repos/GRKarol/czytnik01/releases/latest";
    private static final String PREFS_NAME = "flower_update_check";
    private static final String KEY_LAST_SEEN_TAG = "last_seen_tag";
    private static final String CHANNEL_ID = "flower_updates";
    private static final int NOTIFICATION_ID = 1001;

    public UpdateCheckWorker(@NonNull Context context, @NonNull WorkerParameters params) {
        super(context, params);
    }

    @NonNull
    @Override
    public Result doWork() {
        String latestTag;
        try {
            latestTag = fetchLatestTag();
        } catch (Exception e) {
            return Result.retry();
        }
        if (latestTag == null || latestTag.isEmpty()) {
            return Result.success();
        }

        SharedPreferences prefs =
                getApplicationContext().getSharedPreferences(PREFS_NAME, Context.MODE_PRIVATE);
        String lastSeenTag = prefs.getString(KEY_LAST_SEEN_TAG, null);

        // Pierwsze uruchomienie: zapamiętaj punkt odniesienia, nie powiadamiaj —
        // inaczej appka pokazałaby powiadomienie o release'ie który już jest
        // aktualny od dawna, tylko dlatego że worker jeszcze go nie widział.
        if (lastSeenTag == null) {
            prefs.edit().putString(KEY_LAST_SEEN_TAG, latestTag).apply();
            return Result.success();
        }
        if (!latestTag.equals(lastSeenTag)) {
            prefs.edit().putString(KEY_LAST_SEEN_TAG, latestTag).apply();
            showNotification(latestTag);
        }
        return Result.success();
    }

    private String fetchLatestTag() throws Exception {
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
            JSONObject json = new JSONObject(sb.toString());
            return json.optString("tag_name", null);
        } finally {
            conn.disconnect();
        }
    }

    private void showNotification(String tag) {
        Context context = getApplicationContext();
        NotificationManager manager = context.getSystemService(NotificationManager.class);
        if (manager == null) {
            return;
        }
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = manager.getNotificationChannel(CHANNEL_ID);
            if (channel == null) {
                channel = new NotificationChannel(
                        CHANNEL_ID,
                        "Aktualizacje firmware",
                        NotificationManager.IMPORTANCE_DEFAULT);
                channel.setDescription("Powiadomienia o nowych wersjach firmware czytnika Flower.");
                manager.createNotificationChannel(channel);
            }
        }

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
            NotificationManagerCompat.from(context).notify(NOTIFICATION_ID, notification.build());
        } catch (SecurityException ignored) {
            // Brak zgody POST_NOTIFICATIONS (Android 13+) — po prostu nie pokazujemy powiadomienia.
        }
    }
}
