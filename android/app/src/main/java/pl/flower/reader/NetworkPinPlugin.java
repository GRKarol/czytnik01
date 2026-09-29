package pl.flower.reader;

import android.content.Context;
import android.content.Intent;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkRequest;
import android.net.wifi.WifiNetworkSpecifier;
import android.os.Build;
import android.os.PatternMatcher;
import android.provider.Settings;

import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;

/**
 * Android traktuje sieci bez internetu (jak AP czytnika, 192.168.4.1) jako
 * "gorsze" i kieruje ruch appki przez sieć komórkową albo inne WiFi z
 * internetem — appka dostaje wtedy timeouty, choć telefon jest połączony z
 * czytnikiem. bindProcessToNetwork wymusza, żeby requesty tego procesu szły
 * przez sieć czytnika.
 *
 * connectToReader() (Android 10+) prosi system o sieć "Flower-…" przez
 * WifiNetworkSpecifier: użytkownik klika jedną sieć w systemowym okienku,
 * nie musi wychodzić do ustawień WiFi, a telefon zostaje przy swoim
 * internecie dla innych aplikacji.
 */
@CapacitorPlugin(name = "NetworkPin")
public class NetworkPinPlugin extends Plugin {
    private static final String READER_SSID_PREFIX = "Flower-";
    private static final int REQUEST_TIMEOUT_MS = 45000;

    private ConnectivityManager.NetworkCallback readerCallback;
    private Network readerNetwork;
    // The JS call still waiting for the system dialog; answered "not
    // connected" when a newer request (the QR fallback) replaces it.
    private PluginCall pendingJoinCall;

    @PluginMethod
    public void pin(PluginCall call) {
        ConnectivityManager cm = getConnectivityManager();
        if (cm == null) {
            call.reject("ConnectivityManager niedostępny");
            return;
        }
        Network target = readerNetwork != null ? readerNetwork : findWifiNetwork(cm);
        if (target == null) {
            call.reject("Telefon nie jest połączony z żadną siecią WiFi");
            return;
        }
        boolean ok = cm.bindProcessToNetwork(target);
        JSObject ret = new JSObject();
        ret.put("pinned", ok);
        call.resolve(ret);
    }

    @PluginMethod
    public void unpin(PluginCall call) {
        ConnectivityManager cm = getConnectivityManager();
        boolean ok = cm == null || cm.bindProcessToNetwork(null);
        JSObject ret = new JSObject();
        ret.put("unpinned", ok);
        call.resolve(ret);
    }

    /** Asks Android to join the reader's "Flower-…" network and pins to it. */
    @PluginMethod
    public void connectToReader(PluginCall call) {
        ConnectivityManager cm = getConnectivityManager();
        if (cm == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) {
            JSObject ret = new JSObject();
            ret.put("connected", false);
            ret.put("supported", false);
            call.resolve(ret);
            return;
        }
        WifiNetworkSpecifier specifier = new WifiNetworkSpecifier.Builder()
                .setSsidPattern(new PatternMatcher(READER_SSID_PREFIX, PatternMatcher.PATTERN_PREFIX))
                .build();
        requestReader(cm, call, specifier);
    }

    /**
     * QR fallback: the exact network name (and password, if the code has
     * one) read from the reader's screen, for when the "Flower-…" list
     * stays empty.
     */
    @PluginMethod
    public void connectToNetwork(PluginCall call) {
        ConnectivityManager cm = getConnectivityManager();
        String ssid = call.getString("ssid", "");
        String password = call.getString("password", "");
        if (cm == null || Build.VERSION.SDK_INT < Build.VERSION_CODES.Q || ssid == null || ssid.isEmpty()) {
            JSObject ret = new JSObject();
            ret.put("connected", false);
            ret.put("supported", cm != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q);
            call.resolve(ret);
            return;
        }
        WifiNetworkSpecifier.Builder builder = new WifiNetworkSpecifier.Builder().setSsid(ssid);
        if (password != null && !password.isEmpty()) {
            builder.setWpa2Passphrase(password);
        }
        requestReader(cm, call, builder.build());
    }

    private void requestReader(ConnectivityManager cm, PluginCall call, WifiNetworkSpecifier specifier) {
        releaseReaderRequest(cm);
        pendingJoinCall = call;
        NetworkRequest request = new NetworkRequest.Builder()
                .addTransportType(NetworkCapabilities.TRANSPORT_WIFI)
                .removeCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .setNetworkSpecifier(specifier)
                .build();

        final boolean[] answered = {false};
        readerCallback = new ConnectivityManager.NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                readerNetwork = network;
                cm.bindProcessToNetwork(network);
                if (!answered[0]) {
                    answered[0] = true;
                    pendingJoinCall = null;
                    JSObject ret = new JSObject();
                    ret.put("connected", true);
                    ret.put("supported", true);
                    call.resolve(ret);
                }
            }

            @Override
            public void onUnavailable() {
                readerNetwork = null;
                if (!answered[0]) {
                    answered[0] = true;
                    pendingJoinCall = null;
                    JSObject ret = new JSObject();
                    ret.put("connected", false);
                    ret.put("supported", true);
                    call.resolve(ret);
                }
            }

            @Override
            public void onLost(Network network) {
                if (network.equals(readerNetwork)) {
                    readerNetwork = null;
                    cm.bindProcessToNetwork(null);
                }
            }
        };
        cm.requestNetwork(request, readerCallback, REQUEST_TIMEOUT_MS);
    }

    /** Drops the reader network request (the phone leaves "Flower-…"). */
    @PluginMethod
    public void releaseReader(PluginCall call) {
        ConnectivityManager cm = getConnectivityManager();
        if (cm != null) {
            releaseReaderRequest(cm);
            cm.bindProcessToNetwork(null);
        }
        call.resolve();
    }

    @PluginMethod
    public void openWifiSettings(PluginCall call) {
        Intent intent = new Intent(Settings.ACTION_WIFI_SETTINGS);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        getContext().startActivity(intent);
        call.resolve();
    }

    private void releaseReaderRequest(ConnectivityManager cm) {
        if (pendingJoinCall != null) {
            JSObject ret = new JSObject();
            ret.put("connected", false);
            ret.put("supported", true);
            ret.put("replaced", true);
            pendingJoinCall.resolve(ret);
            pendingJoinCall = null;
        }
        if (readerCallback != null) {
            try {
                cm.unregisterNetworkCallback(readerCallback);
            } catch (IllegalArgumentException ignored) {
                // already unregistered
            }
            readerCallback = null;
        }
        readerNetwork = null;
    }

    // The active network when it is WiFi, otherwise any WiFi network: with
    // mobile data on, Android makes the cellular network the active one as
    // soon as the WiFi network turns out to have no internet.
    private Network findWifiNetwork(ConnectivityManager cm) {
        Network active = cm.getActiveNetwork();
        if (isWifi(cm, active)) {
            return active;
        }
        for (Network network : cm.getAllNetworks()) {
            if (isWifi(cm, network)) {
                return network;
            }
        }
        return null;
    }

    private boolean isWifi(ConnectivityManager cm, Network network) {
        if (network == null) {
            return false;
        }
        NetworkCapabilities caps = cm.getNetworkCapabilities(network);
        return caps != null && caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI);
    }

    private ConnectivityManager getConnectivityManager() {
        return (ConnectivityManager) getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
    }
}
