// firmware/src/plugins/PluginLibrary.h
#pragma once

#include <Arduino.h>
#include <vector>

/**
 * PluginLibrary — online plugin store that connects to GitHub to fetch a
 * registry of available plugins, download/install them to SD card, and
 * manage installed plugins.
 *
 * WiFi credentials (SSID/password) are passed by the caller — the library
 * does not read Preferences directly.
 */
class PluginLibrary {
 public:
    struct RegistryEntry {
        String id;
        String name;
        String description;
        String version;
        String author;
        uint32_t sdkVersion;
        String binaryUrl;
        String manifestUrl;
        uint32_t sizeBytes;
        String minFirmwareVersion;
    };

    struct InstalledPlugin {
        String id;
        String name;
        String version;
    };

    enum class FetchResult : uint8_t {
        Success,
        WifiError,
        HttpError,
        ParseError,
    };

    using ProgressCallback = void (*)(void* context, int progressPercent);

    bool begin();

    /// Set WiFi credentials (must be called before fetchRegistry/downloadPlugin)
    void setWifiCredentials(const String& ssid, const String& password);

    /// Fetch the plugin registry from GitHub. Connects WiFi if not already connected.
    FetchResult fetchRegistry();

    /// Download and install a plugin by ID from the registry.
    bool downloadPlugin(const char* pluginId, ProgressCallback cb = nullptr,
                        void* context = nullptr);

    /// Remove an installed plugin (deletes /plugins/{id}/ directory recursively).
    bool removePlugin(const char* pluginId);

    /// Scan /plugins/ directory for installed plugins with valid manifests.
    void scanInstalled();

    // Accessors
    const std::vector<RegistryEntry>& registry() const { return registry_; }
    const std::vector<InstalledPlugin>& installed() const { return installed_; }
    bool isInstalled(const char* pluginId) const;
    bool isUpdateAvailable(const char* pluginId) const;

 private:
    bool parseRegistry(const String& json);
    bool downloadFile(const String& url, const String& destPath,
                      size_t expectedSize, ProgressCallback cb, void* context);
    void cleanupPartialDownload(const char* pluginId);
    bool connectWifi();
    void disconnectWifi();
    String pluginBinaryPath(const char* pluginId) const;
    String pluginManifestPath(const char* pluginId) const;
    String pluginDirPath(const char* pluginId) const;
    bool parseManifest(const String& path, InstalledPlugin& info);
    int compareVersions(const String& a, const String& b) const;
    bool ensurePluginDir(const char* pluginId);
    void removeDirectoryRecursive(const String& path);

    std::vector<RegistryEntry> registry_;
    std::vector<InstalledPlugin> installed_;
    String wifiSsid_;
    String wifiPassword_;
    bool wifiConnected_ = false;
};
