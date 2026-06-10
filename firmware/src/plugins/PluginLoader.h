// firmware/src/plugins/PluginLoader.h
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include "plugins/sdk/PluginSdk.h"
#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginAudioService.h"
#include "plugins/sdk/PluginImuService.h"
#include "plugins/sdk/PluginStorageService.h"
#include "plugins/sdk/PluginOrientationService.h"

class DisplayManager;
class AudioManager;

/**
 * PluginLoader — loads position-independent plugin binaries from SD card
 * into PSRAM and executes them in an isolated FreeRTOS task.
 *
 * State machine:
 *   Idle → Loading → Running → Idle (normal)
 *   Idle → Loading → Error (validation/init failure)
 *   Running → Error (watchdog/crash)
 *   Error → Idle (after unload)
 */
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
    static constexpr uint32_t kTaskLoopDelayMs = 33;  // ~30fps for e-ink

    /// Initialize the loader (create mutex, etc.)
    bool begin();

    /// Set references to firmware managers (must be called before load)
    void setManagers(DisplayManager* display, AudioManager* audio);

    /// Load and start a plugin from SD card
    LoadResult load(const char* pluginId);

    /// Unload the currently running plugin (graceful shutdown)
    void unload();

    /// Feed the plugin watchdog (called by plugin task loop)
    void feedWatchdog();

    /// Check watchdog — call from main loop periodically
    void watchdogCheck(uint32_t nowMs);

    // Accessors
    State state() const { return state_; }
    ErrorCode lastError() const { return lastError_; }
    const char* lastErrorMessage() const { return lastErrorMessage_; }
    size_t psramUsed() const { return psramUsed_; }
    size_t psramAvailable() const;
    bool isRunning() const { return state_ == State::Running; }

    /// Get the plugin task handle (used by panic handler for crash detection)
    TaskHandle_t pluginTaskHandle() const { return pluginTask_; }

    /// Mark plugin as crashed (called from panic handler)
    void markCrashed();

 private:
    static void pluginTaskEntry(void* param);
    void pluginTaskLoop();
    bool validateHeader(const PluginBinaryHeader* header, size_t fileSize);
    bool resolveVTable(uint8_t* base, uint32_t entryOffset);
    void setupDeviceServices(const char* pluginId);
    void teardownDeviceServices();
    void terminatePluginTask();
    void freePluginMemory();

    // State
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

    // Device service implementations (bridge to firmware managers)
    PluginDisplayService displayService_ = {};
    PluginAudioService audioService_ = {};
    PluginImuService imuService_ = {};
    PluginStorageService storageService_ = {};
    PluginOrientationService orientationService_ = {};

    // FreeRTOS task
    TaskHandle_t pluginTask_ = nullptr;
    volatile uint32_t watchdogLastFeedMs_ = 0;
    uint32_t watchdogTimeoutMs_ = kDefaultWatchdogTimeoutMs;
    uint32_t stackSize_ = kDefaultStackSize;
    volatile bool exitRequested_ = false;
    SemaphoreHandle_t pluginMutex_ = nullptr;

    // Sandboxed storage root path for current plugin
    String pluginStorageRoot_;

    // Firmware manager pointers (set via setManagers)
    DisplayManager* display_ = nullptr;
    AudioManager* audio_ = nullptr;
};
