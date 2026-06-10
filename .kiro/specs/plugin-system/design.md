# Design Document: Plugin System

## Overview

This design replaces the current OTA variant-switching mechanism with a native plugin system for the ESP32-S3 e-reader. Plugins are position-independent C++ binaries stored on SD card, dynamically loaded into PSRAM at runtime, and executed in isolated FreeRTOS tasks. A Plugin Library UI allows users to browse, download, install, and remove plugins over WiFi from a GitHub-hosted registry.

## Architecture

### Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Main Firmware                              │
│                                                                   │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────┐  │
│  │ App (Menu)   │  │ Plugin       │  │ Plugin Library         │  │
│  │              │──▶│ Loader       │  │ (Online Store)         │  │
│  └──────────────┘  └──────┬───────┘  └───────────┬───────────┘  │
│                           │                       │               │
│                           ▼                       ▼               │
│  ┌──────────────────────────────┐  ┌──────────────────────────┐  │
│  │ Plugin Task (FreeRTOS)       │  │ Plugin Downloader        │  │
│  │ ┌────────────────────────┐   │  │ (GitHub Release fetch)   │  │
│  │ │ Plugin Binary (PSRAM)  │   │  └──────────┬───────────────┘  │
│  │ │ ┌──────────────────┐   │   │             │               │
│  │ │ │ Plugin SDK IF     │   │   │             ▼               │
│  │ │ └──────────────────┘   │   │  ┌──────────────────────────┐  │
│  │ └────────────────────────┘   │  │ StorageManager (SD Card) │  │
│  └──────────────────────────────┘  └──────────────────────────┘  │
│                                                                   │
│  ┌──────────────────────────────────────────────────────────────┐ │
│  │                    Device Services                            │ │
│  │  DisplayManager │ AudioManager │ IMU │ Orientation │ SD I/O  │ │
│  └──────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────┐
│  SD Card: /plugins/             │
│  ├── focus-timer/               │
│  │   ├── plugin.bin             │
│  │   └── manifest.json          │
│  └── rss/                       │
│      ├── plugin.bin             │
│      ├── manifest.json          │
│      └── articles/              │
└─────────────────────────────────┘

┌─────────────────────────────────┐
│  GitHub Release Assets          │
│  ├── plugins-registry.json      │
│  ├── focus-timer-plugin.bin     │
│  └── rss-plugin.bin             │
└─────────────────────────────────┘
```

### Data Flow

1. **Plugin Discovery**: Plugin Library → WiFi → GitHub API → fetch `plugins-registry.json`
2. **Plugin Download**: Plugin Library → HTTPS GET binary_url → write `/plugins/{id}/plugin.bin` + `manifest.json`
3. **Plugin Loading**: User menu selection → Plugin Loader reads SD → allocate PSRAM → validate header → invoke `plugin_init`
4. **Plugin Execution**: Plugin Task loop calls `plugin_update` → plugin calls Device Services → `plugin_draw`
5. **Plugin Unloading**: Exit signal → `plugin_destroy` → free PSRAM → delete FreeRTOS task → return to menu

## Components and Interfaces

### Plugin SDK Interface

#### Core Types

```cpp
// firmware/src/plugins/sdk/PluginSdk.h
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// SDK version — increment on breaking changes
#define PLUGIN_SDK_VERSION 1

// Forward declarations for device service types
typedef struct PluginDisplayService PluginDisplayService;
typedef struct PluginAudioService PluginAudioService;
typedef struct PluginImuService PluginImuService;
typedef struct PluginStorageService PluginStorageService;
typedef struct PluginOrientationService PluginOrientationService;

// Result codes
typedef enum {
    PLUGIN_OK = 0,
    PLUGIN_ERROR = -1,
    PLUGIN_ERROR_INIT = -2,
    PLUGIN_ERROR_MEMORY = -3,
} PluginResult;

// Button event
typedef struct {
    uint8_t buttonId;   // 0 = boot, 1 = power
    bool pressed;       // true = press, false = release
    uint32_t timestampMs;
} PluginButtonEvent;

// Touch event
typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t phase;      // 0 = begin, 1 = move, 2 = end
    uint32_t timestampMs;
} PluginTouchEvent;

// Plugin info returned by plugin_get_info
typedef struct {
    const char* name;
    const char* version;
    uint32_t sdkVersion;
} PluginInfo;

// All device services available to a plugin
typedef struct {
    PluginDisplayService* display;
    PluginAudioService* audio;
    PluginImuService* imu;
    PluginStorageService* storage;
    PluginOrientationService* orientation;
    uint32_t firmwareVersion;
} PluginContext;

// Lifecycle function pointer types
typedef PluginResult (*PluginInitFn)(PluginContext* ctx);
typedef void (*PluginDestroyFn)(void);
typedef void (*PluginUpdateFn)(uint32_t nowMs);
typedef void (*PluginHandleButtonFn)(const PluginButtonEvent* event);
typedef void (*PluginHandleTouchFn)(const PluginTouchEvent* event);
typedef void (*PluginDrawFn)(void);
typedef PluginInfo (*PluginGetInfoFn)(void);

// Binary header at fixed offset 0 in the plugin .bin file
typedef struct {
    uint32_t magic;           // 0x504C5547 ("PLUG")
    uint32_t sdkVersion;      // must match PLUGIN_SDK_VERSION
    uint32_t binarySize;      // total .bin file size in bytes
    uint32_t entryOffset;     // offset to PluginVTable from start
    uint32_t reserved[4];     // future use, must be 0
} PluginBinaryHeader;

#define PLUGIN_HEADER_MAGIC 0x504C5547u

// Virtual table of plugin functions (located at entryOffset)
typedef struct {
    PluginInitFn init;
    PluginDestroyFn destroy;
    PluginUpdateFn update;
    PluginHandleButtonFn handleButton;
    PluginHandleTouchFn handleTouch;
    PluginDrawFn draw;
    PluginGetInfoFn getInfo;
} PluginVTable;

// Watchdog feed — plugin must call this periodically
typedef void (*PluginWatchdogFeedFn)(void);

#ifdef __cplusplus
}
#endif
```

### Device Services

```cpp
// firmware/src/plugins/sdk/PluginDisplayService.h
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginDisplayService {
    void (*renderFocusTimerScreen)(const char* mode, const char* genre,
                                   const char* timer, const char* instruction,
                                   const char* footer, int progressPercent,
                                   bool breakAccent);
    void (*renderStatus)(const char* title, const char* line1, const char* line2);
    void (*renderProgress)(const char* title, const char* line1,
                           const char* line2, int progressPercent);
    void (*renderMenu)(const char* const* items, uint8_t itemCount,
                       uint8_t selectedIndex);
    void (*renderCenteredWord)(const char* word);
    void (*setDarkMode)(bool dark);
    int (*logicalWidth)(void);
    int (*logicalHeight)(void);
} PluginDisplayService;

#ifdef __cplusplus
}
#endif
```

```cpp
// firmware/src/plugins/sdk/PluginAudioService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginAudioService {
    bool (*beep)(void);
    bool (*available)(void);
} PluginAudioService;

#ifdef __cplusplus
}
#endif
```

```cpp
// firmware/src/plugins/sdk/PluginImuService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginImuService {
    bool (*readAccelerometer)(float* x, float* y, float* z);
    bool (*available)(void);
} PluginImuService;

#ifdef __cplusplus
}
#endif
```

```cpp
// firmware/src/plugins/sdk/PluginStorageService.h
#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PluginStorageService {
    // Sandboxed file operations — paths relative to /plugins/{id}/
    bool (*fileExists)(const char* relativePath);
    int (*readFile)(const char* relativePath, uint8_t* buffer, uint32_t maxLen);
    bool (*writeFile)(const char* relativePath, const uint8_t* data, uint32_t len);
    bool (*deleteFile)(const char* relativePath);
    bool (*mkdir)(const char* relativePath);
} PluginStorageService;

#ifdef __cplusplus
}
#endif
```

```cpp
// firmware/src/plugins/sdk/PluginOrientationService.h
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PLUGIN_ORIENTATION_LANDSCAPE = 0,
    PLUGIN_ORIENTATION_PORTRAIT_A = 1,
    PLUGIN_ORIENTATION_PORTRAIT_B = 2,
    PLUGIN_ORIENTATION_LANDSCAPE_FLIPPED = 3,
} PluginOrientation;

typedef struct PluginOrientationService {
    PluginOrientation (*currentOrientation)(void);
    void (*setUiOrientation)(PluginOrientation orientation);
} PluginOrientationService;

#ifdef __cplusplus
}
#endif
```

### Plugin Loader Implementation

### PluginLoader Class

```cpp
// firmware/src/plugins/PluginLoader.h
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "plugins/sdk/PluginSdk.h"

class PluginLoader {
 public:
    enum class State : uint8_t {
        Idle = 0,
        Loading,
        Running,
        Error,
    };

    enum class ErrorCode : uint8_t {
        None = 0,
        FileNotFound,
        InvalidHeader,
        SdkMismatch,
        OutOfMemory,
        InitFailed,
        WatchdogTimeout,
        Crashed,
    };

    struct LoadResult {
        bool success;
        ErrorCode error;
        const char* message;
    };

    static constexpr uint32_t kDefaultStackSize = 8192;
    static constexpr uint32_t kDefaultWatchdogTimeoutMs = 5000;
    static constexpr uint32_t kPluginTaskPriority = 2;

    bool begin();
    LoadResult load(const char* pluginId);
    void unload();
    void feedWatchdog();

    State state() const { return state_; }
    ErrorCode lastError() const { return lastError_; }
    const char* lastErrorMessage() const { return lastErrorMessage_; }
    size_t psramUsed() const { return psramUsed_; }
    size_t psramAvailable() const;
    bool isRunning() const { return state_ == State::Running; }

 private:
    static void pluginTaskEntry(void* param);
    void pluginTaskLoop();
    void watchdogCheck(uint32_t nowMs);
    bool validateHeader(const PluginBinaryHeader* header, size_t fileSize);
    bool resolveVTable(uint8_t* base, uint32_t entryOffset);
    void setupDeviceServices(const char* pluginId);
    void teardownDeviceServices();
    void terminatePluginTask();
    void freePluginMemory();

    State state_ = State::Idle;
    ErrorCode lastError_ = ErrorCode::None;
    const char* lastErrorMessage_ = "";

    // Plugin memory
    uint8_t* binaryBuffer_ = nullptr;
    size_t binarySize_ = 0;
    size_t psramUsed_ = 0;

    // Plugin interface
    PluginVTable* vtable_ = nullptr;
    PluginContext context_ = {};

    // Device service implementations (bridge to firmware)
    PluginDisplayService displayService_ = {};
    PluginAudioService audioService_ = {};
    PluginImuService imuService_ = {};
    PluginStorageService storageService_ = {};
    PluginOrientationService orientationService_ = {};

    // FreeRTOS task
    TaskHandle_t pluginTask_ = nullptr;
    uint32_t watchdogLastFeedMs_ = 0;
    uint32_t watchdogTimeoutMs_ = kDefaultWatchdogTimeoutMs;
    uint32_t stackSize_ = kDefaultStackSize;
    volatile bool exitRequested_ = false;
    SemaphoreHandle_t pluginMutex_ = nullptr;

    // Sandboxed storage root path
    String pluginStorageRoot_;
};
```

### Loading Sequence

```cpp
// Pseudocode for PluginLoader::load()
LoadResult PluginLoader::load(const char* pluginId) {
    // 1. Construct path: /plugins/{pluginId}/plugin.bin
    String path = String("/plugins/") + pluginId + "/plugin.bin";

    // 2. Check file exists and get size
    File file = SD.open(path, FILE_READ);
    if (!file) return {false, ErrorCode::FileNotFound, "Binary not found"};
    size_t fileSize = file.size();

    // 3. Check PSRAM capacity
    size_t available = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (fileSize > available) {
        return {false, ErrorCode::OutOfMemory, "Insufficient PSRAM"};
    }

    // 4. Allocate PSRAM buffer
    binaryBuffer_ = (uint8_t*)heap_caps_malloc(fileSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_EXEC);
    if (!binaryBuffer_) return {false, ErrorCode::OutOfMemory, "PSRAM alloc failed"};
    binarySize_ = fileSize;
    psramUsed_ = fileSize;

    // 5. Read binary into PSRAM
    file.read(binaryBuffer_, fileSize);
    file.close();

    // 6. Validate header
    PluginBinaryHeader* header = (PluginBinaryHeader*)binaryBuffer_;
    if (!validateHeader(header, fileSize)) {
        freePluginMemory();
        return {false, lastError_, lastErrorMessage_};
    }

    // 7. Resolve VTable
    if (!resolveVTable(binaryBuffer_, header->entryOffset)) {
        freePluginMemory();
        return {false, ErrorCode::InvalidHeader, "VTable resolution failed"};
    }

    // 8. Setup device services bridge
    setupDeviceServices(pluginId);

    // 9. Call plugin_init
    PluginResult initResult = vtable_->init(&context_);
    if (initResult != PLUGIN_OK) {
        teardownDeviceServices();
        freePluginMemory();
        return {false, ErrorCode::InitFailed, "plugin_init failed"};
    }

    // 10. Create FreeRTOS task
    exitRequested_ = false;
    watchdogLastFeedMs_ = millis();
    xTaskCreatePinnedToCore(
        pluginTaskEntry, "plugin", stackSize_ / sizeof(StackType_t),
        this, kPluginTaskPriority, &pluginTask_, 1 /*core*/
    );

    state_ = State::Running;
    return {true, ErrorCode::None, ""};
}
```

### Linker Script for PIC Binaries

```ld
/* plugins/plugin.ld — Linker script for position-independent plugin binaries */
ENTRY(_plugin_entry)

SECTIONS
{
    /* Plugin header at offset 0 */
    .plugin_header 0x0 : {
        KEEP(*(.plugin_header))
    }

    /* Code section — position independent */
    .text : {
        *(.text .text.*)
        *(.rodata .rodata.*)
    }

    /* VTable section */
    .plugin_vtable : {
        KEEP(*(.plugin_vtable))
        _plugin_vtable_start = .;
    }

    /* Data sections */
    .data : {
        *(.data .data.*)
    }

    .bss : {
        _bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        _bss_end = .;
    }

    /* Discard unwanted sections */
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
```

### Plugin PlatformIO Build Configuration

```ini
; plugins/focus-timer/platformio.ini
[env:focus-timer-plugin]
platform = espressif32@6.3.2
board = esp32-s3-r8-opi
framework = arduino
build_type = release
build_flags =
    -fPIC
    -fno-exceptions
    -fno-rtti
    -nostdlib
    -DPLUGIN_BUILD=1
build_unflags =
    -fno-PIC
extra_scripts =
    post:../../tools/pio_plugin_build.py
board_build.ldscript = ../../plugins/plugin.ld
lib_deps =
build_src_filter = +<src/>
lib_extra_dirs =
    ../../firmware/src/plugins/sdk
```

### Plugin Library (Online Store)

### PluginLibrary Class

```cpp
// firmware/src/plugins/PluginLibrary.h
#pragma once

#include <Arduino.h>
#include <vector>

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
    FetchResult fetchRegistry();
    bool downloadPlugin(const char* pluginId, ProgressCallback cb = nullptr,
                        void* context = nullptr);
    bool removePlugin(const char* pluginId);
    void scanInstalled();

    const std::vector<RegistryEntry>& registry() const { return registry_; }
    const std::vector<InstalledPlugin>& installed() const { return installed_; }
    bool isInstalled(const char* pluginId) const;
    bool isUpdateAvailable(const char* pluginId) const;

 private:
    bool parseRegistry(const String& json);
    bool downloadFile(const String& url, const String& destPath,
                      ProgressCallback cb, void* context);
    void cleanupPartialDownload(const char* pluginId);
    bool connectWifi();
    void disconnectWifi();
    String pluginBinaryPath(const char* pluginId) const;
    String pluginManifestPath(const char* pluginId) const;
    String pluginDirPath(const char* pluginId) const;
    bool parseManifest(const String& path, InstalledPlugin& info);
    int compareVersions(const String& a, const String& b) const;

    std::vector<RegistryEntry> registry_;
    std::vector<InstalledPlugin> installed_;
    uint32_t registryVersion_ = 0;
    bool wifiConnected_ = false;
};
```

### Registry JSON Format

```json
{
  "registry_version": 1,
  "plugins": [
    {
      "id": "focus-timer",
      "name": "Klepsydra",
      "description": "Reading session timer with orientation-based input",
      "version": "1.0.0",
      "author": "GRKarol",
      "sdk_version": 1,
      "binary_url": "https://github.com/GRKarol/czytnik01/releases/download/v1.0.0/focus-timer-plugin.bin",
      "manifest_url": "https://github.com/GRKarol/czytnik01/releases/download/v1.0.0/focus-timer-manifest.json",
      "size_bytes": 32768,
      "min_firmware_version": "1.0.0"
    },
    {
      "id": "rss",
      "name": "RSS Feeds",
      "description": "Download and read articles from RSS/Atom feeds",
      "version": "1.0.0",
      "author": "GRKarol",
      "sdk_version": 1,
      "binary_url": "https://github.com/GRKarol/czytnik01/releases/download/v1.0.0/rss-plugin.bin",
      "manifest_url": "https://github.com/GRKarol/czytnik01/releases/download/v1.0.0/rss-manifest.json",
      "size_bytes": 45056,
      "min_firmware_version": "1.0.0"
    }
  ]
}
```

### Manifest JSON Format

```json
{
  "id": "focus-timer",
  "name": "Klepsydra",
  "version": "1.0.0",
  "author": "GRKarol",
  "sdk_version": 1,
  "description": "Reading session timer with orientation-based input",
  "permissions": ["imu", "audio", "display", "orientation"]
}
```

### Plugin Task and Crash Isolation

### FreeRTOS Task Design

```cpp
// Plugin runs on Core 1 with its own stack in PSRAM
void PluginLoader::pluginTaskEntry(void* param) {
    PluginLoader* self = static_cast<PluginLoader*>(param);
    self->pluginTaskLoop();
    vTaskDelete(nullptr);
}

void PluginLoader::pluginTaskLoop() {
    while (!exitRequested_) {
        uint32_t now = millis();

        // Feed watchdog
        watchdogLastFeedMs_ = now;

        // Call plugin update
        if (vtable_->update) vtable_->update(now);

        // Call plugin draw
        if (vtable_->draw) vtable_->draw();

        // Yield to other tasks (target ~30fps for e-ink)
        vTaskDelay(pdMS_TO_TICKS(33));
    }

    // Clean exit: call destroy
    if (vtable_->destroy) vtable_->destroy();
}
```

### Watchdog Implementation

The main firmware task periodically checks the plugin watchdog:

```cpp
void PluginLoader::watchdogCheck(uint32_t nowMs) {
    if (state_ != State::Running || pluginTask_ == nullptr) return;

    uint32_t elapsed = nowMs - watchdogLastFeedMs_;
    if (elapsed > watchdogTimeoutMs_) {
        // Plugin is hung — terminate
        terminatePluginTask();
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::WatchdogTimeout;
        lastErrorMessage_ = "Plugin timed out";
    }
}

void PluginLoader::terminatePluginTask() {
    if (pluginTask_ != nullptr) {
        vTaskDelete(pluginTask_);
        pluginTask_ = nullptr;
    }
}
```

### Exception Handling

ESP-IDF provides panic handlers. The firmware registers a custom handler that detects if the crash originated in the plugin task:

```cpp
// In firmware init — register exception hook
esp_register_shutdown_handler(pluginPanicHandler);

// On ESP32-S3, we override the panic handler to check task identity
void pluginPanicHandler() {
    TaskHandle_t crashed = xTaskGetCurrentTaskHandle();
    if (crashed == pluginLoader.pluginTask_) {
        // Plugin crashed — mark for recovery on next main loop iteration
        pluginLoader.markCrashed();
    }
}
```

### SD Card Storage Layout

```
/plugins/                          <- Created by StorageManager on init
├── focus-timer/
│   ├── plugin.bin                 <- Plugin binary (PIC)
│   ├── manifest.json              <- Plugin metadata
│   └── [runtime data files]       <- Plugin-created files (sandboxed)
├── rss/
│   ├── plugin.bin
│   ├── manifest.json
│   ├── config.json                <- RSS feed URLs
│   └── articles/                  <- Downloaded articles
│       ├── article_001.txt
│       └── article_002.txt
└── [future plugins...]
```

### Plugin Persistence

The installed plugins list is tracked by scanning the `/plugins/` directory on boot. Each subdirectory with a valid `manifest.json` is considered installed. No separate NVS bitmask is needed — the filesystem is the source of truth.

```cpp
void PluginLibrary::scanInstalled() {
    installed_.clear();
    File dir = SD.open("/plugins");
    if (!dir || !dir.isDirectory()) return;

    File entry;
    while ((entry = dir.openNextFile())) {
        if (!entry.isDirectory()) continue;
        String manifestPath = String("/plugins/") + entry.name() + "/manifest.json";
        InstalledPlugin info;
        if (parseManifest(manifestPath, info)) {
            installed_.push_back(info);
        }
    }
}
```

### Version Comparison

Semantic version comparison for update detection:

```cpp
int PluginLibrary::compareVersions(const String& a, const String& b) const {
    // Parse "major.minor.patch" and compare numerically
    int aMajor, aMinor, aPatch;
    int bMajor, bMinor, bPatch;
    sscanf(a.c_str(), "%d.%d.%d", &aMajor, &aMinor, &aPatch);
    sscanf(b.c_str(), "%d.%d.%d", &bMajor, &bMinor, &bPatch);

    if (aMajor != bMajor) return aMajor - bMajor;
    if (aMinor != bMinor) return aMinor - bMinor;
    return aPatch - bPatch;
}
```

### Download Flow

```
User taps "Install" on Plugin Library screen
    │
    ▼
connectWifi() using stored credentials
    │ (fail → show error, offer WiFi settings)
    ▼
downloadFile(binary_url → /plugins/{id}/plugin.bin)
    │ (progress callback updates e-ink)
    │ (fail → cleanupPartialDownload → show error)
    ▼
downloadFile(manifest_url → /plugins/{id}/manifest.json)
    │ (fail → cleanupPartialDownload → show error)
    ▼
scanInstalled() — refresh plugin list
    │
    ▼
Return to Plugin Library screen (plugin now shows "Installed")
```

### FocusTimer Plugin Extraction

The existing `FocusTimer` class is refactored into a standalone plugin binary. The 11-state state machine, genre selection, orientation detection, and timer logic move into the plugin. The plugin accesses hardware through Device_Services only.

### Plugin Structure

```cpp
// plugins/focus-timer/src/main.cpp
#include "PluginSdk.h"
#include "FocusTimerCore.h"  // extracted state machine logic

static FocusTimerCore* timer = nullptr;
static PluginContext* ctx = nullptr;

extern "C" {

// Place header in .plugin_header section
__attribute__((section(".plugin_header")))
const PluginBinaryHeader pluginHeader = {
    .magic = PLUGIN_HEADER_MAGIC,
    .sdkVersion = PLUGIN_SDK_VERSION,
    .binarySize = 0,  // filled by build tool post-process
    .entryOffset = 0, // filled by build tool post-process
    .reserved = {0, 0, 0, 0},
};

__attribute__((section(".plugin_vtable")))
const PluginVTable pluginVTable = {
    .init = plugin_init,
    .destroy = plugin_destroy,
    .update = plugin_update,
    .handleButton = plugin_handle_button,
    .handleTouch = plugin_handle_touch,
    .draw = plugin_draw,
    .getInfo = plugin_get_info,
};

PluginResult plugin_init(PluginContext* context) {
    ctx = context;
    timer = new FocusTimerCore(ctx->imu, ctx->audio, ctx->display, ctx->orientation);
    if (!timer) return PLUGIN_ERROR_MEMORY;
    if (!timer->begin()) return PLUGIN_ERROR_INIT;
    return PLUGIN_OK;
}

void plugin_destroy() {
    delete timer;
    timer = nullptr;
    ctx = nullptr;
}

void plugin_update(uint32_t nowMs) {
    if (timer) timer->update(nowMs);
}

void plugin_handle_button(const PluginButtonEvent* event) {
    if (timer) timer->handleButton(event);
}

void plugin_handle_touch(const PluginTouchEvent* event) {
    if (timer) timer->handleTouch(event);
}

void plugin_draw() {
    if (timer) timer->draw();
}

PluginInfo plugin_get_info() {
    return {
        .name = "Klepsydra",
        .version = "1.0.0",
        .sdkVersion = PLUGIN_SDK_VERSION,
    };
}

} // extern "C"
```

### CI/Build System

### Repository Structure

```
czytnik01/
├── firmware/              <- Main firmware (Plugin Loader + Library)
│   ├── src/
│   └── platformio.ini
├── plugins/               <- Plugin source code
│   ├── focus-timer/
│   │   ├── src/
│   │   ├── manifest.json
│   │   └── platformio.ini
│   └── rss/
│       ├── src/
│       ├── manifest.json
│       └── platformio.ini
├── tools/
│   ├── pio_plugin_build.py    <- Post-build: fill header offsets
│   └── generate_registry.py  <- CI: generate plugins-registry.json
└── .github/workflows/
    ├── ci.yml                 <- Build firmware + plugins
    └── release.yml            <- Attach binaries + registry to release
```

### CI Workflow

```yaml
# .github/workflows/release.yml (plugin section)
- name: Build plugins
  run: |
    for dir in plugins/*/; do
      plugin_id=$(basename "$dir")
      cd "$dir"
      pio run
      cp .pio/build/*/firmware.bin "../../dist/${plugin_id}-plugin.bin"
      cp manifest.json "../../dist/${plugin_id}-manifest.json"
      cd ../..
    done

- name: Generate registry
  run: python tools/generate_registry.py --output dist/plugins-registry.json

- name: Upload release assets
  uses: softprops/action-gh-release@v1
  with:
    files: |
      dist/*-plugin.bin
      dist/*-manifest.json
      dist/plugins-registry.json
```

## Error Handling

| Scenario                  | Handler                           | User Feedback                            |
| ------------------------- | --------------------------------- | ---------------------------------------- |
| SD card missing/unmounted | StorageManager returns error      | "SD card not found" status screen        |
| Plugin binary not found   | PluginLoader returns FileNotFound | "Plugin file missing" error              |
| SDK version mismatch      | PluginLoader returns SdkMismatch  | "Plugin incompatible — update firmware"  |
| PSRAM exhausted           | PluginLoader returns OutOfMemory  | "Not enough memory" error                |
| plugin_init fails         | PluginLoader returns InitFailed   | "Plugin failed to start" error           |
| Plugin hangs (watchdog)   | PluginLoader terminates task      | "Plugin stopped responding" error        |
| Plugin crashes            | Exception handler recovers        | "Plugin crashed" error                   |
| WiFi connection failure   | PluginLibrary shows error         | "Cannot connect to WiFi" + settings link |
| Download interrupted      | PluginLibrary cleans up           | "Download failed" + partial file removed |
| Invalid registry JSON     | PluginLibrary parse error         | "Registry format error"                  |
| Manifest missing fields   | Validation rejects                | Plugin not shown in library              |

## Data Models

### NVS Keys (Preferences)

The old `pl_mask` NVS key is removed. No NVS storage needed for plugin state — the filesystem is authoritative.

### In-Memory State

```cpp
// PluginLoader state machine
enum State { Idle, Loading, Running, Error };

// PluginLibrary caches
std::vector<RegistryEntry> registry_;   // fetched from GitHub
std::vector<InstalledPlugin> installed_; // scanned from SD
```

## Testing Strategy

### Unit Tests

- **SDK header parsing**: Verify fixed-offset header reads correct fields from byte buffers
- **Manifest JSON parsing**: Verify all fields extracted from valid JSON, errors on invalid
- **Registry JSON parsing**: Verify schema validation of registry entries
- **Path construction**: Verify correct paths for various plugin IDs
- **Version comparison**: Verify ordering of semantic version strings
- **Sandbox path validation**: Verify path traversal attacks are rejected

### Property-Based Tests

- Binary header serialization round-trip
- Manifest and registry validation (valid iff all fields present)
- SDK version compatibility (match iff equal)
- PSRAM capacity gating (accept iff size <= available)
- Watchdog triggering (fire iff elapsed > timeout)
- Semantic version comparison correctness
- File sandbox enforcement
- FocusTimer state machine equivalence against original

### Integration Tests

- Plugin load/init/destroy lifecycle on real PSRAM
- FreeRTOS task creation and watchdog termination
- WiFi fetch of registry JSON from GitHub
- SD card file write/read during plugin install
- Download interruption and partial file cleanup

### Smoke Tests

- SDK header defines all lifecycle function pointers
- Default stack size and watchdog timeout constants
- Compiled plugin contains no absolute relocations
- platformio.ini has no variant environments after migration
- CI workflow attaches registry to releases

## Correctness Properties

_A property is a characteristic or behavior that should hold true across all valid executions of a system — essentially, a formal statement about what the system should do. Properties serve as the bridge between human-readable specifications and machine-verifiable correctness guarantees._

### Property 1: Binary Header Round-Trip

_For any_ valid `PluginBinaryHeader` struct with magic = 0x504C5547, valid sdkVersion, non-zero binarySize, and valid entryOffset, serializing the header to a byte buffer and parsing it back SHALL produce an identical struct.

**Validates: Requirements 2.2**

### Property 2: Plugin Path Construction

_For any_ valid plugin ID string (non-empty, containing only lowercase alphanumeric characters and hyphens), the path construction functions SHALL produce paths matching the pattern `/plugins/{id}/plugin.bin` for binaries and `/plugins/{id}/manifest.json` for manifests.

**Validates: Requirements 2.4, 2.5, 11.1, 11.2, 11.3, 11.4**

### Property 3: Manifest Validation

_For any_ JSON object, manifest validation SHALL return true if and only if the object contains all required fields (`id`, `name`, `version`, `author`, `sdk_version`, `description`, `permissions`) with correct types, and SHALL return false for any object missing one or more required fields.

**Validates: Requirements 2.6**

### Property 4: SDK Version Compatibility Check

_For any_ plugin binary header SDK version and firmware-supported SDK version, the validation function SHALL return compatible if and only if the binary SDK version equals the firmware SDK version.

**Validates: Requirements 3.2, 3.3**

### Property 5: PSRAM Capacity Gate

_For any_ plugin binary size and available PSRAM capacity, the loader SHALL accept the load request if and only if the binary size is less than or equal to the available capacity.

**Validates: Requirements 3.6**

### Property 6: Watchdog Timeout Detection

_For any_ watchdog timeout value T and elapsed time E since last feed, the watchdog check SHALL trigger termination if and only if E > T.

**Validates: Requirements 4.4**

### Property 7: Download Failure Cleanup

_For any_ plugin download that fails or is interrupted at any point during the write process, after cleanup completes there SHALL be no partial files remaining in the `/plugins/{plugin_id}/` directory.

**Validates: Requirements 5.7**

### Property 8: Semantic Version Comparison

_For any_ two semantic version strings in "major.minor.patch" format, the comparison function SHALL return that an update is available if and only if the remote version is strictly greater than the local version (comparing major, then minor, then patch numerically).

**Validates: Requirements 5.8**

### Property 9: Installed Plugins Persistence Round-Trip

_For any_ set of plugin directories on SD card each containing a valid `manifest.json`, scanning the `/plugins/` directory SHALL produce an installed plugins list containing exactly those plugin IDs, and this list SHALL be identical across power cycles (re-scan after reboot yields the same result).

**Validates: Requirements 6.6**

### Property 10: Registry Entry Validation

_For any_ JSON object representing a registry entry, validation SHALL pass if and only if the object contains all required fields (`id`, `name`, `description`, `version`, `author`, `sdk_version`, `binary_url`, `manifest_url`, `size_bytes`, `min_firmware_version`) with correct types, and the root registry object contains a `registry_version` field.

**Validates: Requirements 7.2, 7.3, 7.4**

### Property 11: FocusTimer State Machine Equivalence

_For any_ sequence of timer events (genre selection, orientation changes, time advances), applying that sequence to both the original built-in FocusTimer state machine and the extracted plugin FocusTimerCore SHALL produce identical state transitions, timer values, and completion flags.

**Validates: Requirements 8.2**

### Property 12: Plugin File Sandboxing

_For any_ file path that a plugin attempts to write via the PluginStorageService, the write SHALL succeed if and only if the resolved absolute path is strictly within the plugin's own `/plugins/{own_id}/` directory. Any path traversal attempt (e.g., `../other-plugin/file`) SHALL be rejected.

**Validates: Requirements 11.6**

### Property 13: Registry Generation from Manifests

_For any_ set of valid plugin manifest files in the `plugins/` directory, the registry generator SHALL produce a `plugins-registry.json` containing exactly one entry per manifest with matching `id`, `name`, `version`, `author`, and `sdk_version` fields.

**Validates: Requirements 10.4**
