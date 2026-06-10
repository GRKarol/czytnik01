// firmware/src/plugins/PluginLibrary.cpp
#include "plugins/PluginLibrary.h"

#include <HTTPClient.h>
#include <SD_MMC.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <cJSON.h>

static const char* TAG = "PluginLibrary";

namespace {

constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kWifiConnectPollMs = 250;
constexpr size_t kMaxRegistryJsonBytes = 32768;
constexpr size_t kDownloadBufferSize = 1024;

constexpr const char* kRegistryUrl =
    "https://github.com/GRKarol/czytnik01/releases/latest/download/plugins-registry.json";

/// Build the download URL for a plugin binary from the latest release.
String pluginBinaryUrl(const char* pluginId) {
    return String("https://github.com/GRKarol/czytnik01/releases/latest/download/") +
           pluginId + "-plugin.bin";
}

/// Build the download URL for a plugin manifest from the latest release.
String pluginManifestUrl(const char* pluginId) {
    return String("https://github.com/GRKarol/czytnik01/releases/latest/download/") +
           pluginId + "-manifest.json";
}

/// Read HTTP response body up to maxBytes.
String readResponseBody(HTTPClient& http, size_t maxBytes) {
    WiFiClient* stream = http.getStreamPtr();
    if (!stream) return "";

    const int contentLength = http.getSize();
    String body;
    const size_t reserveSize =
        contentLength > 0
            ? min(static_cast<size_t>(contentLength), maxBytes)
            : static_cast<size_t>(1024);
    body.reserve(reserveSize);

    uint8_t buffer[512];
    size_t totalRead = 0;

    while (http.connected() || stream->available()) {
        if (contentLength > 0 && totalRead >= static_cast<size_t>(contentLength)) break;

        const int avail = stream->available();
        if (avail <= 0) {
            delay(1);
            continue;
        }

        const size_t remaining = maxBytes - totalRead;
        if (remaining == 0) break;

        const size_t chunkSize = min(remaining, min(sizeof(buffer), static_cast<size_t>(avail)));
        const int bytesRead = stream->readBytes(buffer, chunkSize);
        if (bytesRead <= 0) break;

        totalRead += static_cast<size_t>(bytesRead);
        for (int i = 0; i < bytesRead; ++i) {
            body += static_cast<char>(buffer[i]);
        }
    }

    return body;
}

}  // namespace

// ─── Public ─────────────────────────────────────────────────────────────────

bool PluginLibrary::begin() {
    scanInstalled();
    return true;
}

void PluginLibrary::setWifiCredentials(const String& ssid, const String& password) {
    wifiSsid_ = ssid;
    wifiPassword_ = password;
}

PluginLibrary::FetchResult PluginLibrary::fetchRegistry() {
    if (!connectWifi()) {
        return FetchResult::WifiError;
    }

    WiFiClientSecure client;
    client.setInsecure();  // GitHub redirects across hosts; signed manifests are the hardening path
    client.setHandshakeTimeout(15);

    HTTPClient http;
    http.setUserAgent("Flower-PluginLib/1.0");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(15000);

    if (!http.begin(client, kRegistryUrl)) {
        ESP_LOGE(TAG, "HTTP begin failed for registry URL");
        disconnectWifi();
        return FetchResult::HttpError;
    }

    const int statusCode = http.GET();
    if (statusCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "Registry fetch failed: HTTP %d", statusCode);
        http.end();
        disconnectWifi();
        return FetchResult::HttpError;
    }

    String body = readResponseBody(http, kMaxRegistryJsonBytes);
    http.end();
    disconnectWifi();

    if (body.isEmpty()) {
        ESP_LOGE(TAG, "Empty registry response");
        return FetchResult::HttpError;
    }

    if (!parseRegistry(body)) {
        return FetchResult::ParseError;
    }

    return FetchResult::Success;
}

bool PluginLibrary::downloadPlugin(const char* pluginId, ProgressCallback cb,
                                   void* context) {
    if (!pluginId || strlen(pluginId) == 0) return false;

    // Find registry entry for size info
    size_t expectedBinarySize = 0;
    String binaryUrl;
    String manifestUrl;

    for (const auto& entry : registry_) {
        if (entry.id == pluginId) {
            expectedBinarySize = entry.sizeBytes;
            binaryUrl = entry.binaryUrl.isEmpty()
                ? pluginBinaryUrl(pluginId)
                : entry.binaryUrl;
            manifestUrl = entry.manifestUrl.isEmpty()
                ? pluginManifestUrl(pluginId)
                : entry.manifestUrl;
            break;
        }
    }

    // Fallback URLs if plugin not in registry (manual install)
    if (binaryUrl.isEmpty()) {
        binaryUrl = pluginBinaryUrl(pluginId);
    }
    if (manifestUrl.isEmpty()) {
        manifestUrl = pluginManifestUrl(pluginId);
    }

    if (!connectWifi()) {
        return false;
    }

    // Ensure plugin directory exists
    if (!ensurePluginDir(pluginId)) {
        disconnectWifi();
        return false;
    }

    // Download binary to temp file, then rename on success
    String binPath = pluginBinaryPath(pluginId);
    String binTmpPath = binPath + ".tmp";

    if (cb) cb(context, 0);

    if (!downloadFile(binaryUrl, binTmpPath, expectedBinarySize, cb, context)) {
        ESP_LOGE(TAG, "Binary download failed for '%s'", pluginId);
        cleanupPartialDownload(pluginId);
        disconnectWifi();
        return false;
    }

    if (cb) cb(context, 85);

    // Download manifest
    String manifestPath = pluginManifestPath(pluginId);
    String manifestTmpPath = manifestPath + ".tmp";

    if (!downloadFile(manifestUrl, manifestTmpPath, 0, nullptr, nullptr)) {
        ESP_LOGE(TAG, "Manifest download failed for '%s'", pluginId);
        cleanupPartialDownload(pluginId);
        disconnectWifi();
        return false;
    }

    disconnectWifi();

    // Atomic rename: remove old files first, then rename temps
    SD_MMC.remove(binPath);
    if (!SD_MMC.rename(binTmpPath, binPath)) {
        ESP_LOGE(TAG, "Failed to rename binary tmp to final");
        cleanupPartialDownload(pluginId);
        return false;
    }

    SD_MMC.remove(manifestPath);
    if (!SD_MMC.rename(manifestTmpPath, manifestPath)) {
        ESP_LOGE(TAG, "Failed to rename manifest tmp to final");
        // Binary is already in place, but manifest rename failed — cleanup
        SD_MMC.remove(binPath);
        SD_MMC.remove(manifestTmpPath);
        return false;
    }

    if (cb) cb(context, 100);

    // Refresh installed list
    scanInstalled();

    ESP_LOGI(TAG, "Plugin '%s' installed successfully", pluginId);
    return true;
}

bool PluginLibrary::removePlugin(const char* pluginId) {
    if (!pluginId || strlen(pluginId) == 0) return false;

    String dirPath = pluginDirPath(pluginId);

    // Check if the directory exists
    File dir = SD_MMC.open(dirPath);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        ESP_LOGW(TAG, "Plugin directory not found: %s", dirPath.c_str());
        return false;
    }
    dir.close();

    // Recursively delete the plugin directory
    removeDirectoryRecursive(dirPath);

    // Refresh installed list
    scanInstalled();

    ESP_LOGI(TAG, "Plugin '%s' removed", pluginId);
    return true;
}

void PluginLibrary::scanInstalled() {
    installed_.clear();

    File dir = SD_MMC.open("/plugins");
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return;
    }

    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) {
            entry.close();
            continue;
        }

        // entry.name() returns the full path on ESP32 SD_MMC
        String entryName = entry.name();
        entry.close();

        // Extract just the directory name from the path
        int lastSlash = entryName.lastIndexOf('/');
        String dirName = (lastSlash >= 0)
            ? entryName.substring(lastSlash + 1)
            : entryName;

        String manifestPath = String("/plugins/") + dirName + "/manifest.json";
        InstalledPlugin info;
        if (parseManifest(manifestPath, info)) {
            installed_.push_back(info);
        }
    }

    dir.close();
}

bool PluginLibrary::isInstalled(const char* pluginId) const {
    for (const auto& p : installed_) {
        if (p.id == pluginId) return true;
    }
    return false;
}

bool PluginLibrary::isUpdateAvailable(const char* pluginId) const {
    // Find installed version
    String installedVersion;
    for (const auto& p : installed_) {
        if (p.id == pluginId) {
            installedVersion = p.version;
            break;
        }
    }
    if (installedVersion.isEmpty()) return false;

    // Find registry version
    for (const auto& r : registry_) {
        if (r.id == pluginId) {
            return compareVersions(r.version, installedVersion) > 0;
        }
    }

    return false;
}

// ─── Private ────────────────────────────────────────────────────────────────

bool PluginLibrary::parseRegistry(const String& json) {
    cJSON* root = cJSON_Parse(json.c_str());
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse registry JSON");
        return false;
    }

    // Check registry_version exists
    cJSON* regVersion = cJSON_GetObjectItemCaseSensitive(root, "registry_version");
    if (!regVersion || !cJSON_IsNumber(regVersion)) {
        ESP_LOGE(TAG, "Missing or invalid registry_version");
        cJSON_Delete(root);
        return false;
    }

    cJSON* plugins = cJSON_GetObjectItemCaseSensitive(root, "plugins");
    if (!plugins || !cJSON_IsArray(plugins)) {
        ESP_LOGE(TAG, "Missing or invalid plugins array");
        cJSON_Delete(root);
        return false;
    }

    registry_.clear();

    cJSON* item = nullptr;
    cJSON_ArrayForEach(item, plugins) {
        if (!cJSON_IsObject(item)) continue;

        cJSON* id = cJSON_GetObjectItemCaseSensitive(item, "id");
        cJSON* name = cJSON_GetObjectItemCaseSensitive(item, "name");
        cJSON* description = cJSON_GetObjectItemCaseSensitive(item, "description");
        cJSON* version = cJSON_GetObjectItemCaseSensitive(item, "version");
        cJSON* author = cJSON_GetObjectItemCaseSensitive(item, "author");
        cJSON* sdkVersion = cJSON_GetObjectItemCaseSensitive(item, "sdk_version");
        cJSON* binaryUrl = cJSON_GetObjectItemCaseSensitive(item, "binary_url");
        cJSON* manifestUrl = cJSON_GetObjectItemCaseSensitive(item, "manifest_url");
        cJSON* sizeBytes = cJSON_GetObjectItemCaseSensitive(item, "size_bytes");
        cJSON* minFw = cJSON_GetObjectItemCaseSensitive(item, "min_firmware_version");

        // Validate required fields
        if (!id || !cJSON_IsString(id) || strlen(id->valuestring) == 0) continue;
        if (!name || !cJSON_IsString(name)) continue;
        if (!version || !cJSON_IsString(version)) continue;
        if (!sdkVersion || !cJSON_IsNumber(sdkVersion)) continue;

        RegistryEntry entry;
        entry.id = id->valuestring;
        entry.name = name->valuestring;
        entry.description = (description && cJSON_IsString(description))
            ? description->valuestring : "";
        entry.version = version->valuestring;
        entry.author = (author && cJSON_IsString(author)) ? author->valuestring : "";
        entry.sdkVersion = static_cast<uint32_t>(sdkVersion->valueint);
        entry.binaryUrl = (binaryUrl && cJSON_IsString(binaryUrl))
            ? binaryUrl->valuestring : "";
        entry.manifestUrl = (manifestUrl && cJSON_IsString(manifestUrl))
            ? manifestUrl->valuestring : "";
        entry.sizeBytes = (sizeBytes && cJSON_IsNumber(sizeBytes))
            ? static_cast<uint32_t>(sizeBytes->valueint) : 0;
        entry.minFirmwareVersion = (minFw && cJSON_IsString(minFw))
            ? minFw->valuestring : "";

        registry_.push_back(entry);
    }

    cJSON_Delete(root);

    ESP_LOGI(TAG, "Registry parsed: %u plugins", (unsigned)registry_.size());
    return true;
}

bool PluginLibrary::downloadFile(const String& url, const String& destPath,
                                 size_t expectedSize, ProgressCallback cb,
                                 void* context) {
    // Remove any existing temp file
    SD_MMC.remove(destPath);

    WiFiClientSecure client;
    client.setInsecure();
    client.setHandshakeTimeout(15);

    HTTPClient http;
    http.setUserAgent("Flower-PluginLib/1.0");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    http.setTimeout(30000);

    if (!http.begin(client, url)) {
        ESP_LOGE(TAG, "HTTP begin failed: %s", url.c_str());
        return false;
    }

    const int statusCode = http.GET();
    if (statusCode != HTTP_CODE_OK) {
        ESP_LOGE(TAG, "Download failed: HTTP %d for %s", statusCode, url.c_str());
        http.end();
        return false;
    }

    const int contentLength = http.getSize();
    const size_t totalSize = (contentLength > 0)
        ? static_cast<size_t>(contentLength)
        : expectedSize;

    WiFiClient* stream = http.getStreamPtr();
    if (!stream) {
        http.end();
        return false;
    }

    // Open destination file for writing
    File file = SD_MMC.open(destPath, FILE_WRITE);
    if (!file) {
        ESP_LOGE(TAG, "Cannot create file: %s", destPath.c_str());
        http.end();
        return false;
    }

    uint8_t buffer[kDownloadBufferSize];
    size_t totalWritten = 0;
    bool success = true;

    while (http.connected() || stream->available()) {
        if (contentLength > 0 && totalWritten >= static_cast<size_t>(contentLength)) break;

        const int avail = stream->available();
        if (avail <= 0) {
            delay(1);
            continue;
        }

        const size_t chunkSize = min(sizeof(buffer), static_cast<size_t>(avail));
        const int bytesRead = stream->readBytes(buffer, chunkSize);
        if (bytesRead <= 0) break;

        const size_t written = file.write(buffer, bytesRead);
        if (written != static_cast<size_t>(bytesRead)) {
            ESP_LOGE(TAG, "SD write error at offset %u", (unsigned)totalWritten);
            success = false;
            break;
        }

        totalWritten += written;

        // Report progress (scale to 5–80% range for binary, leave room for manifest)
        if (cb && totalSize > 0) {
            int percent = static_cast<int>((totalWritten * 80) / totalSize);
            if (percent > 80) percent = 80;
            cb(context, 5 + percent);
        }
    }

    file.close();
    http.end();

    // Verify we got the expected amount of data
    if (contentLength > 0 && totalWritten != static_cast<size_t>(contentLength)) {
        ESP_LOGE(TAG, "Incomplete download: got %u of %d bytes",
                 (unsigned)totalWritten, contentLength);
        SD_MMC.remove(destPath);
        return false;
    }

    if (totalWritten == 0) {
        ESP_LOGE(TAG, "Downloaded 0 bytes from %s", url.c_str());
        SD_MMC.remove(destPath);
        return false;
    }

    return success;
}

void PluginLibrary::cleanupPartialDownload(const char* pluginId) {
    String binTmp = pluginBinaryPath(pluginId) + ".tmp";
    String manTmp = pluginManifestPath(pluginId) + ".tmp";

    SD_MMC.remove(binTmp);
    SD_MMC.remove(manTmp);

    // If the directory is now empty (no plugin.bin, no manifest.json),
    // remove it too
    String dirPath = pluginDirPath(pluginId);
    File dir = SD_MMC.open(dirPath);
    if (dir && dir.isDirectory()) {
        File entry = dir.openNextFile();
        if (!entry) {
            // Empty directory — remove it
            dir.close();
            SD_MMC.rmdir(dirPath);
            return;
        }
        entry.close();
    }
    if (dir) dir.close();
}

bool PluginLibrary::connectWifi() {
    if (WiFi.status() == WL_CONNECTED) {
        wifiConnected_ = true;
        return true;
    }

    if (wifiSsid_.isEmpty()) {
        ESP_LOGE(TAG, "WiFi SSID not configured");
        return false;
    }

    WiFi.persistent(false);
    WiFi.setAutoReconnect(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSsid_.c_str(), wifiPassword_.c_str());

    const uint32_t startMs = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < kWifiConnectTimeoutMs) {
        delay(kWifiConnectPollMs);
    }

    wifiConnected_ = (WiFi.status() == WL_CONNECTED);
    if (!wifiConnected_) {
        ESP_LOGE(TAG, "WiFi connection failed (SSID: %s)", wifiSsid_.c_str());
        WiFi.disconnect(true, false);
        WiFi.mode(WIFI_OFF);
    }

    return wifiConnected_;
}

void PluginLibrary::disconnectWifi() {
    if (wifiConnected_) {
        WiFi.disconnect(true, false);
        WiFi.mode(WIFI_OFF);
        wifiConnected_ = false;
    }
}

String PluginLibrary::pluginBinaryPath(const char* pluginId) const {
    return String("/plugins/") + pluginId + "/plugin.bin";
}

String PluginLibrary::pluginManifestPath(const char* pluginId) const {
    return String("/plugins/") + pluginId + "/manifest.json";
}

String PluginLibrary::pluginDirPath(const char* pluginId) const {
    return String("/plugins/") + pluginId;
}

bool PluginLibrary::ensurePluginDir(const char* pluginId) {
    // Ensure /plugins/ exists
    if (!SD_MMC.exists("/plugins")) {
        SD_MMC.mkdir("/plugins");
    }

    String dirPath = pluginDirPath(pluginId);
    if (!SD_MMC.exists(dirPath)) {
        if (!SD_MMC.mkdir(dirPath)) {
            ESP_LOGE(TAG, "Failed to create plugin dir: %s", dirPath.c_str());
            return false;
        }
    }
    return true;
}

bool PluginLibrary::parseManifest(const String& path, InstalledPlugin& info) {
    File file = SD_MMC.open(path, FILE_READ);
    if (!file || file.isDirectory()) {
        if (file) file.close();
        return false;
    }

    // Read manifest content (manifests are small — well under 2KB)
    size_t fileSize = file.size();
    if (fileSize == 0 || fileSize > 4096) {
        file.close();
        return false;
    }

    String content;
    content.reserve(fileSize);
    while (file.available()) {
        content += static_cast<char>(file.read());
    }
    file.close();

    cJSON* root = cJSON_Parse(content.c_str());
    if (!root) {
        ESP_LOGW(TAG, "Invalid manifest JSON: %s", path.c_str());
        return false;
    }

    cJSON* id = cJSON_GetObjectItemCaseSensitive(root, "id");
    cJSON* name = cJSON_GetObjectItemCaseSensitive(root, "name");
    cJSON* version = cJSON_GetObjectItemCaseSensitive(root, "version");

    if (!id || !cJSON_IsString(id) || strlen(id->valuestring) == 0 ||
        !name || !cJSON_IsString(name) ||
        !version || !cJSON_IsString(version)) {
        cJSON_Delete(root);
        return false;
    }

    info.id = id->valuestring;
    info.name = name->valuestring;
    info.version = version->valuestring;

    cJSON_Delete(root);
    return true;
}

int PluginLibrary::compareVersions(const String& a, const String& b) const {
    int aMajor = 0, aMinor = 0, aPatch = 0;
    int bMajor = 0, bMinor = 0, bPatch = 0;

    sscanf(a.c_str(), "%d.%d.%d", &aMajor, &aMinor, &aPatch);
    sscanf(b.c_str(), "%d.%d.%d", &bMajor, &bMinor, &bPatch);

    if (aMajor != bMajor) return aMajor - bMajor;
    if (aMinor != bMinor) return aMinor - bMinor;
    return aPatch - bPatch;
}

void PluginLibrary::removeDirectoryRecursive(const String& path) {
    File dir = SD_MMC.open(path);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        // It might be a file — try removing it
        SD_MMC.remove(path);
        return;
    }

    File entry;
    while ((entry = dir.openNextFile())) {
        String entryPath = entry.name();
        bool isDir = entry.isDirectory();
        entry.close();

        if (isDir) {
            removeDirectoryRecursive(entryPath);
        } else {
            SD_MMC.remove(entryPath);
        }
    }

    dir.close();
    SD_MMC.rmdir(path);
}
