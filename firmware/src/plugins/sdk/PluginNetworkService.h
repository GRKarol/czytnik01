// firmware/src/plugins/sdk/PluginNetworkService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum PluginNetworkStatus {
    PLUGIN_NETWORK_IDLE = 0,
    PLUGIN_NETWORK_CONNECTING,
    PLUGIN_NETWORK_FETCHING,
    PLUGIN_NETWORK_DONE,
    PLUGIN_NETWORK_ERROR,
} PluginNetworkStatus;

/// Async HTTP(S) GET, so a plugin can fetch a URL without ever blocking its
/// own update() call — the plugin task loop runs update()/draw() at ~30fps
/// under an 8s watchdog (see PluginLoader), and a synchronous HTTPClient::GET()
/// on a slow feed can easily take longer than that. The bridge implementation
/// runs the connect+fetch on its own FreeRTOS task; the plugin just polls
/// fetchStatus() from update().
typedef struct PluginNetworkService {
    /// True if a WiFi network is saved in the device's Settings (App's own
    /// WiFi credentials, reused here) — lets a plugin show "connect to
    /// WiFi first" instead of always trying startFetch() and failing.
    bool (*hasSavedNetwork)(void);

    /// Starts an async GET of `url`. Returns false immediately if a fetch
    /// is already in flight or no network is saved (call hasSavedNetwork()
    /// first to tell the two apart). Never blocks.
    bool (*startFetch)(const char* url);

    PluginNetworkStatus (*fetchStatus)(void);

    /// Valid only while fetchStatus() == PLUGIN_NETWORK_DONE, and only
    /// until the next startFetch()/cancelFetch() call — copy out whatever
    /// you need (e.g. hand it straight to your own parser) before either.
    const char* (*fetchResultBody)(void);
    uint32_t (*fetchResultLength)(void);

    /// Human-readable reason, valid while fetchStatus() == PLUGIN_NETWORK_ERROR.
    const char* (*fetchErrorMessage)(void);

    /// Aborts any in-flight fetch and returns to Idle. Call this from your
    /// plugin's shutdown() so a fetch task doesn't outlive the plugin (same
    /// reasoning as DictaphoneCore::shutdown() stopping an orphaned
    /// AudioRecorder — see that file for what happens if you don't).
    void (*cancelFetch)(void);
} PluginNetworkService;

#ifdef __cplusplus
}
#endif
