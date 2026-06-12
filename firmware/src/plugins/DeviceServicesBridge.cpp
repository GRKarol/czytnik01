// firmware/src/plugins/DeviceServicesBridge.cpp
#include "plugins/DeviceServicesBridge.h"

#include <Arduino.h>
#include <SD_MMC.h>
#include <Wire.h>
#include <string.h>

#include "audio/AudioManager.h"
#include "audio/AudioRecorder.h"
#include "board/BoardConfig.h"
#include "display/DisplayManager.h"

static const char* TAG = "DeviceServicesBridge";

// ─── Static state (allows C function pointers to reach C++ objects) ─────────

static DisplayManager* sDisplay = nullptr;
static AudioManager* sAudio = nullptr;
static AudioRecorder* sRecorder = nullptr;
static String sStorageRoot;  // e.g. "/plugins/focus-timer/"

// IMU register constants (QMI8658 on Wire1)
namespace {
constexpr uint8_t kImuAddress = 0x6B;
constexpr uint8_t kImuAccelStartReg = 0x35;
constexpr float kAccelScale = 4.0f / 32768.0f;
}  // namespace

// ─── Path Validation (sandbox enforcement) ──────────────────────────────────

/// Returns true if the relative path is safe (no traversal).
/// Rejects any path containing ".." as a component.
static bool isPathSafe(const char* relativePath) {
    if (relativePath == nullptr || relativePath[0] == '\0') {
        return false;
    }

    // Reject absolute paths
    if (relativePath[0] == '/') {
        return false;
    }

    // Reject any occurrence of ".." as a path component
    const char* p = relativePath;
    while (*p) {
        // Check if we're at the start of a ".." component
        if (p[0] == '.' && p[1] == '.') {
            // It's a traversal if it's at start, after '/', or followed by '/' or end
            if ((p == relativePath || *(p - 1) == '/') &&
                (p[2] == '\0' || p[2] == '/')) {
                return false;
            }
        }
        ++p;
    }

    return true;
}

/// Build the full SD path from a relative path, with sandbox validation.
/// Returns empty String on failure.
static String resolveSandboxedPath(const char* relativePath) {
    if (!isPathSafe(relativePath)) {
        ESP_LOGW(TAG, "Path traversal rejected: %s", relativePath ? relativePath : "(null)");
        return String();
    }
    return sStorageRoot + relativePath;
}

// ─── Display Service Wrappers ───────────────────────────────────────────────

static void bridgeRenderFocusTimerScreen(const char* mode, const char* genre,
                                          const char* timer, const char* instruction,
                                          const char* footer, int progressPercent,
                                          bool breakAccent) {
    if (!sDisplay) return;
    sDisplay->renderFocusTimerScreen(
        mode ? mode : "", genre ? genre : "",
        timer ? timer : "", instruction ? instruction : "",
        footer ? footer : "", progressPercent, breakAccent);
}

static void bridgeRenderStatus(const char* title, const char* line1, const char* line2) {
    if (!sDisplay) return;
    sDisplay->renderStatus(title ? title : "", line1 ? line1 : "", line2 ? line2 : "");
}

static void bridgeRenderProgress(const char* title, const char* line1,
                                  const char* line2, int progressPercent) {
    if (!sDisplay) return;
    sDisplay->renderProgress(title ? title : "", line1 ? line1 : "",
                             line2 ? line2 : "", progressPercent);
}

static void bridgeRenderMenu(const char* const* items, uint8_t itemCount,
                              uint8_t selectedIndex) {
    if (!sDisplay || !items) return;
    sDisplay->renderMenu(items, static_cast<size_t>(itemCount),
                         static_cast<size_t>(selectedIndex));
}

static void bridgeRenderCenteredWord(const char* word) {
    if (!sDisplay) return;
    sDisplay->renderCenteredWord(word ? word : "");
}

static void bridgeSetDarkMode(bool dark) {
    if (!sDisplay) return;
    sDisplay->setDarkMode(dark);
}

static int bridgeLogicalWidth() {
    // Return fixed display dimensions based on panel config
    // In landscape mode (default): 640 wide
    return BoardConfig::DISPLAY_WIDTH;
}

static int bridgeLogicalHeight() {
    // In landscape mode (default): 172 tall
    return BoardConfig::DISPLAY_HEIGHT;
}

// ─── Audio Service Wrappers ─────────────────────────────────────────────────

static bool bridgeAudioBeep() {
    if (!sAudio) return false;
    return sAudio->beep();
}

static bool bridgeAudioAvailable() {
    if (!sAudio) return false;
    return sAudio->available();
}

// ─── Audio Recording/Playback Wrappers ──────────────────────────────────────

static bool bridgeAudioStartRecording(const char* relativePath) {
    if (!sRecorder) return false;
    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return false;
    return sRecorder->startRecording(fullPath.c_str());
}

static bool bridgeAudioStopRecording() {
    if (!sRecorder) return false;
    return sRecorder->stopRecording();
}

static bool bridgeAudioIsRecording() {
    if (!sRecorder) return false;
    return sRecorder->isRecording();
}

static uint32_t bridgeAudioRecordingElapsedMs() {
    if (!sRecorder) return 0;
    return sRecorder->recordingElapsedMs();
}

static bool bridgeAudioStartPlayback(const char* relativePath) {
    if (!sRecorder) return false;
    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return false;
    return sRecorder->startPlayback(fullPath.c_str());
}

static bool bridgeAudioStopPlayback() {
    if (!sRecorder) return false;
    return sRecorder->stopPlayback();
}

static bool bridgeAudioIsPlaying() {
    if (!sRecorder) return false;
    return sRecorder->isPlaying();
}

static uint32_t bridgeAudioPlaybackElapsedMs() {
    if (!sRecorder) return 0;
    return sRecorder->playbackElapsedMs();
}

static uint32_t bridgeAudioPlaybackTotalMs() {
    if (!sRecorder) return 0;
    return sRecorder->playbackTotalMs();
}

// ─── IMU Service Wrappers ───────────────────────────────────────────────────

static bool bridgeImuReadAccelerometer(float* x, float* y, float* z) {
    if (!x || !y || !z) return false;

    // Direct I2C read from QMI8658 on Wire1 (same approach as FocusTimer)
    Wire1.beginTransmission(kImuAddress);
    Wire1.write(kImuAccelStartReg);
    if (Wire1.endTransmission(false) != 0) {
        return false;
    }

    if (Wire1.requestFrom(static_cast<int>(kImuAddress), 6, 1) != 6) {
        return false;
    }

    uint8_t buffer[6];
    for (int i = 0; i < 6; ++i) {
        buffer[i] = Wire1.read();
    }

    const int16_t rawX = static_cast<int16_t>((buffer[1] << 8) | buffer[0]);
    const int16_t rawY = static_cast<int16_t>((buffer[3] << 8) | buffer[2]);
    const int16_t rawZ = static_cast<int16_t>((buffer[5] << 8) | buffer[4]);

    *x = rawX * kAccelScale;
    *y = rawY * kAccelScale;
    *z = rawZ * kAccelScale;
    return true;
}

static bool bridgeImuAvailable() {
    // Probe the IMU address on Wire1
    Wire1.beginTransmission(kImuAddress);
    return Wire1.endTransmission(true) == 0;
}

// ─── Orientation Service Wrappers ───────────────────────────────────────────

static PluginOrientation bridgeCurrentOrientation() {
    // The default orientation for this device is LandscapeFlipped
    // (UI_ROTATED_180 = true means buttons are at top in flipped mode).
    return PLUGIN_ORIENTATION_LANDSCAPE_FLIPPED;
}

static void bridgeSetUiOrientation(PluginOrientation orientation) {
    if (!sDisplay) return;

    BoardConfig::UiOrientation mapped;
    switch (orientation) {
        case PLUGIN_ORIENTATION_LANDSCAPE:
            // Plugin says "landscape" — map to the device's default (flipped)
            mapped = BoardConfig::UiOrientation::LandscapeFlipped;
            break;
        case PLUGIN_ORIENTATION_PORTRAIT_A:
            mapped = BoardConfig::UiOrientation::PortraitFlipped;
            break;
        case PLUGIN_ORIENTATION_PORTRAIT_B:
            mapped = BoardConfig::UiOrientation::Portrait;
            break;
        case PLUGIN_ORIENTATION_LANDSCAPE_FLIPPED:
            mapped = BoardConfig::UiOrientation::Landscape;
            break;
        default:
            mapped = BoardConfig::UiOrientation::LandscapeFlipped;
            break;
    }

    sDisplay->setUiOrientation(mapped);
}

// ─── Storage Service Wrappers ───────────────────────────────────────────────

static bool bridgeStorageFileExists(const char* relativePath) {
    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return false;
    return SD_MMC.exists(fullPath);
}

static int bridgeStorageReadFile(const char* relativePath, uint8_t* buffer, uint32_t maxLen) {
    if (!buffer || maxLen == 0) return -1;

    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return -1;

    File file = SD_MMC.open(fullPath, FILE_READ);
    if (!file) return -1;

    int bytesRead = file.read(buffer, maxLen);
    file.close();
    return bytesRead;
}

static bool bridgeStorageWriteFile(const char* relativePath, const uint8_t* data, uint32_t len) {
    if (!data && len > 0) return false;

    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return false;

    File file = SD_MMC.open(fullPath, FILE_WRITE);
    if (!file) return false;

    size_t written = file.write(data, len);
    file.close();
    return written == len;
}

static bool bridgeStorageDeleteFile(const char* relativePath) {
    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return false;

    return SD_MMC.remove(fullPath);
}

static bool bridgeStorageMkdir(const char* relativePath) {
    String fullPath = resolveSandboxedPath(relativePath);
    if (fullPath.isEmpty()) return false;

    return SD_MMC.mkdir(fullPath);
}

// ─── Public API ─────────────────────────────────────────────────────────────

void DeviceServicesBridge::setup(const char* pluginId,
                                  const char* storageRoot,
                                  DisplayManager* display,
                                  AudioManager* audio,
                                  AudioRecorder* recorder,
                                  PluginDisplayService* displayService,
                                  PluginAudioService* audioService,
                                  PluginImuService* imuService,
                                  PluginStorageService* storageService,
                                  PluginOrientationService* orientationService) {
    // Store manager pointers for static wrappers
    sDisplay = display;
    sAudio = audio;
    sRecorder = recorder;
    sStorageRoot = storageRoot ? storageRoot : "";

    // Populate display service function pointers
    if (displayService) {
        displayService->renderFocusTimerScreen = bridgeRenderFocusTimerScreen;
        displayService->renderStatus = bridgeRenderStatus;
        displayService->renderProgress = bridgeRenderProgress;
        displayService->renderMenu = bridgeRenderMenu;
        displayService->renderCenteredWord = bridgeRenderCenteredWord;
        displayService->setDarkMode = bridgeSetDarkMode;
        displayService->logicalWidth = bridgeLogicalWidth;
        displayService->logicalHeight = bridgeLogicalHeight;
    }

    // Populate audio service function pointers
    if (audioService) {
        audioService->beep = bridgeAudioBeep;
        audioService->available = bridgeAudioAvailable;
        audioService->startRecording = bridgeAudioStartRecording;
        audioService->stopRecording = bridgeAudioStopRecording;
        audioService->isRecording = bridgeAudioIsRecording;
        audioService->recordingElapsedMs = bridgeAudioRecordingElapsedMs;
        audioService->startPlayback = bridgeAudioStartPlayback;
        audioService->stopPlayback = bridgeAudioStopPlayback;
        audioService->isPlaying = bridgeAudioIsPlaying;
        audioService->playbackElapsedMs = bridgeAudioPlaybackElapsedMs;
        audioService->playbackTotalMs = bridgeAudioPlaybackTotalMs;
    }

    // Populate IMU service function pointers
    if (imuService) {
        imuService->readAccelerometer = bridgeImuReadAccelerometer;
        imuService->available = bridgeImuAvailable;
    }

    // Populate storage service function pointers
    if (storageService) {
        storageService->fileExists = bridgeStorageFileExists;
        storageService->readFile = bridgeStorageReadFile;
        storageService->writeFile = bridgeStorageWriteFile;
        storageService->deleteFile = bridgeStorageDeleteFile;
        storageService->mkdir = bridgeStorageMkdir;
    }

    // Populate orientation service function pointers
    if (orientationService) {
        orientationService->currentOrientation = bridgeCurrentOrientation;
        orientationService->setUiOrientation = bridgeSetUiOrientation;
    }

    ESP_LOGI(TAG, "Device services bridge set up for plugin '%s'", pluginId ? pluginId : "");
}

void DeviceServicesBridge::teardown() {
    sDisplay = nullptr;
    sAudio = nullptr;
    sRecorder = nullptr;
    sStorageRoot = "";

    ESP_LOGI(TAG, "Device services bridge torn down");
}
