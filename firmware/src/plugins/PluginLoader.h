// firmware/src/plugins/PluginLoader.h
#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

#include "plugins/sdk/PluginSdk.h"
#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginAudioService.h"
#include "plugins/sdk/PluginImuService.h"
#include "plugins/sdk/PluginStorageService.h"
#include "plugins/sdk/PluginOrientationService.h"

class DisplayManager;
class AudioManager;
class AudioRecorder;

/**
 * PluginLoader — manages lifecycle of built-in plugins.
 *
 * Plugin code is compiled into the firmware binary. The loader looks up
 * the plugin by ID in the BuiltinPlugins registry and calls the built-in
 * code directly — no PSRAM binary loading.
 *
 * State machine:
 *   Idle → Loading → Running → Idle (normal)
 *   Idle → Loading → Error (plugin not found / init failure)
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
    static constexpr uint32_t kDefaultWatchdogTimeoutMs = 8000;
    static constexpr uint32_t kPluginTaskPriority = 2;
    static constexpr uint32_t kTaskLoopDelayMs = 33;  // ~30fps for e-ink
    static constexpr uint8_t kEventQueueSize = 8;

    /// Initialize the loader (create mutex, etc.)
    bool begin();

    /// Set references to firmware managers (must be called before load)
    void setManagers(DisplayManager* display, AudioManager* audio, AudioRecorder* recorder);

    /// Load and start a built-in plugin by its ID
    LoadResult load(const char* pluginId);

    /// Unload the currently running plugin (graceful shutdown)
    void unload();

    /// Feed the plugin watchdog (called by plugin task loop)
    void feedWatchdog();

    /// Check watchdog — call from main loop periodically
    void watchdogCheck(uint32_t nowMs);

    /// Forward button event to running plugin
    void forwardButton(const PluginButtonEvent& event);

    /// Forward touch event to running plugin
    void forwardTouch(const PluginTouchEvent& event);

    // Accessors
    State state() const { return state_; }
    ErrorCode lastError() const { return lastError_; }
    const char* lastErrorMessage() const { return lastErrorMessage_; }
    bool isRunning() const { return state_ == State::Running; }

    /// Get the plugin task handle (used by panic handler for crash detection)
    TaskHandle_t pluginTaskHandle() const { return pluginTask_; }

    /// Mark plugin as crashed (called from panic handler)
    void markCrashed();

 private:
    static void pluginTaskEntry(void* param);
    void pluginTaskLoop();
    void processEventQueue();
    void setupDeviceServices(const char* pluginId);
    void teardownDeviceServices();
    void terminatePluginTask();

    // Event queue for thread-safe input forwarding
    enum class EventType : uint8_t { Button, Touch };
    struct PluginEvent {
        EventType type;
        union {
            PluginButtonEvent button;
            PluginTouchEvent touch;
        };
    };

    // State
    State state_ = State::Idle;
    ErrorCode lastError_ = ErrorCode::None;
    const char* lastErrorMessage_ = "";

    // Plugin interface (points into the built-in registry — not owned)
    PluginVTable vtable_ = {};
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
    QueueHandle_t eventQueue_ = nullptr;

    // Sandboxed storage root path for current plugin
    String pluginStorageRoot_;

    // Firmware manager pointers (set via setManagers)
    DisplayManager* display_ = nullptr;
    AudioManager* audio_ = nullptr;
    AudioRecorder* recorder_ = nullptr;
};
