package pl.flower.reader;

import com.getcapacitor.JSObject;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.CapacitorPlugin;

/**
 * Odbiera tekst/linki udostępnione z innych appek przez systemowe Android
 * "Udostępnij" (zob. intent-filter ACTION_SEND w AndroidManifest.xml).
 * MainActivity przekazuje tu tekst z onCreate()/onNewIntent() — cold start
 * (appka jeszcze nie działała) trafia do `pendingText` i JS odbiera go przez
 * getPending() przy starcie; warm start (appka już działa) leci od razu
 * jako event, bo JS ma już podpięty listener.
 */
@CapacitorPlugin(name = "ShareTarget")
public class ShareTargetPlugin extends Plugin {
    private static ShareTargetPlugin instance;
    private static String pendingText;

    @Override
    public void load() {
        instance = this;
    }

    static void stashPending(String text) {
        pendingText = text;
    }

    static void deliverLive(String text) {
        pendingText = text;
        if (instance != null) {
            JSObject data = new JSObject();
            data.put("text", text);
            instance.notifyListeners("shareReceived", data);
        }
    }

    @PluginMethod
    public void getPending(PluginCall call) {
        JSObject ret = new JSObject();
        ret.put("text", pendingText);
        pendingText = null;
        call.resolve(ret);
    }
}
