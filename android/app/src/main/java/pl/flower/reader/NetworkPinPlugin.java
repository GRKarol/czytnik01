package pl.flower.reader;

import android.content.Context;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;

import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;

/**
 * Android traktuje sieci bez internetu (jak AP czytnika, 192.168.4.1) jako
 * "gorsze" i po chwili w tle przełącza cały ruch appki z powrotem na sieć
 * komórkową/inne WiFi z internetem — appka wtedy dostaje timeouty mimo że
 * telefon nadal jest fizycznie połączony z czytnikiem. bindProcessToNetwork
 * wymusza, żeby WSZYSTKIE requesty tego procesu szły przez konkretną sieć,
 * niezależnie od tego co Android uzna za "lepsze".
 */
@CapacitorPlugin(name = "NetworkPin")
public class NetworkPinPlugin extends Plugin {

    @PluginMethod
    public void pin(PluginCall call) {
        ConnectivityManager cm = getConnectivityManager();
        if (cm == null) {
            call.reject("ConnectivityManager niedostępny");
            return;
        }
        Network active = cm.getActiveNetwork();
        if (active == null) {
            call.reject("Brak aktywnej sieci");
            return;
        }
        NetworkCapabilities caps = cm.getNetworkCapabilities(active);
        if (caps == null || !caps.hasTransport(NetworkCapabilities.TRANSPORT_WIFI)) {
            call.reject("Aktywna sieć to nie WiFi");
            return;
        }
        boolean ok = cm.bindProcessToNetwork(active);
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

    private ConnectivityManager getConnectivityManager() {
        return (ConnectivityManager) getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
    }
}
