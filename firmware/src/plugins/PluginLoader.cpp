// firmware/src/plugins/PluginLoader.cpp
#include "plugins/PluginLoader.h"
#include "plugins/BuiltinPlugins.h"
#include "plugins/DeviceServicesBridge.h"

#include <esp_system.h>

static const char* TAG = "PluginLoader";

// Global pointer for the ESP-IDF shutdown/panic handler.
static PluginLoader* s_pluginLoaderInstance = nullptr;

/// ESP-IDF shutdown handler — called on panic/crash.
static void pluginPanicHandler() {
    if (!s_pluginLoaderInstance) return;

    TaskHandle_t crashed = xTaskGetCurrentTaskHandle();
    if (crashed == s_pluginLoaderInstance->pluginTaskHandle()) {
        s_pluginLoaderInstance->markCrashed();
    }
}

// ─── Public ─────────────────────────────────────────────────────────────────

bool PluginLoader::begin() {
    pluginMutex_ = xSemaphoreCreateMutex();
    if (!pluginMutex_) {
        ESP_LOGE(TAG, "Failed to create plugin mutex");
        return false;
    }

    s_pluginLoaderInstance = this;

    esp_err_t err = esp_register_shutdown_handler(pluginPanicHandler);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register shutdown handler: %d", err);
    }

    state_ = State::Idle;
    return true;
}

void PluginLoader::setManagers(DisplayManager* display, AudioManager* audio) {
    display_ = display;
    audio_ = audio;
}

PluginLoader::LoadResult PluginLoader::load(const char* pluginId) {
    if (state_ == State::Running) {
        return {false, ErrorCode::InitFailed, "Another plugin is already running"};
    }

    state_ = State::Loading;
    lastError_ = ErrorCode::None;
    lastErrorMessage_ = "";

    // 1. Look up in built-in plugin registry
    const BuiltinPlugin* plugin = BuiltinPlugins::find(pluginId);
    if (!plugin) {
        state_ = State::Error;
        lastError_ = ErrorCode::FileNotFound;
        lastErrorMessage_ = "Unknown plugin (not compiled into firmware)";
        return {false, lastError_, lastErrorMessage_};
    }

    // 2. Copy vtable from registry
    vtable_ = plugin->vtable;

    // 3. Setup device services bridge
    setupDeviceServices(pluginId);

    // 4. Call plugin init
    if (!vtable_.init) {
        teardownDeviceServices();
        state_ = State::Error;
        lastError_ = ErrorCode::InitFailed;
        lastErrorMessage_ = "Plugin init function is null";
        return {false, lastError_, lastErrorMessage_};
    }

    PluginResult initResult = vtable_.init(&context_);
    if (initResult != PLUGIN_OK) {
        teardownDeviceServices();
        state_ = State::Error;
        lastError_ = ErrorCode::InitFailed;
        lastErrorMessage_ = "plugin_init returned error";
        return {false, lastError_, lastErrorMessage_};
    }

    // 5. Create FreeRTOS task pinned to Core 1
    exitRequested_ = false;
    watchdogLastFeedMs_ = millis();

    BaseType_t taskCreated = xTaskCreatePinnedToCore(
        pluginTaskEntry,
        "plugin",
        stackSize_,
        this,
        kPluginTaskPriority,
        &pluginTask_,
        1  // Core 1
    );

    if (taskCreated != pdPASS) {
        if (vtable_.destroy) {
            vtable_.destroy();
        }
        teardownDeviceServices();
        state_ = State::Error;
        lastError_ = ErrorCode::OutOfMemory;
        lastErrorMessage_ = "FreeRTOS task creation failed";
        return {false, lastError_, lastErrorMessage_};
    }

    state_ = State::Running;
    ESP_LOGI(TAG, "Plugin '%s' launched (built-in)", pluginId);
    return {true, ErrorCode::None, ""};
}

void PluginLoader::unload() {
    if (state_ != State::Running && state_ != State::Error) {
        return;
    }

    exitRequested_ = true;

    if (pluginTask_ != nullptr) {
        uint32_t waitStart = millis();
        const uint32_t kMaxWaitMs = 2000;

        while (pluginTask_ != nullptr && (millis() - waitStart) < kMaxWaitMs) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (pluginTask_ != nullptr) {
            terminatePluginTask();
            if (vtable_.destroy) {
                vtable_.destroy();
            }
        }
    }

    teardownDeviceServices();

    state_ = State::Idle;
    lastError_ = ErrorCode::None;
    lastErrorMessage_ = "";

    ESP_LOGI(TAG, "Plugin unloaded");
}

void PluginLoader::feedWatchdog() {
    watchdogLastFeedMs_ = millis();
}

void PluginLoader::watchdogCheck(uint32_t nowMs) {
    if (state_ != State::Running || pluginTask_ == nullptr) {
        return;
    }

    uint32_t elapsed = nowMs - watchdogLastFeedMs_;
    if (elapsed > watchdogTimeoutMs_) {
        ESP_LOGW(TAG, "Plugin watchdog timeout (%u ms elapsed)", elapsed);
        terminatePluginTask();
        state_ = State::Error;
        lastError_ = ErrorCode::WatchdogTimeout;
        lastErrorMessage_ = "Plugin stopped responding";
    }
}

void PluginLoader::markCrashed() {
    if (state_ == State::Running) {
        pluginTask_ = nullptr;
        state_ = State::Error;
        lastError_ = ErrorCode::Crashed;
        lastErrorMessage_ = "Plugin crashed";
    }
}

// ─── Private ────────────────────────────────────────────────────────────────

void PluginLoader::pluginTaskEntry(void* param) {
    PluginLoader* self = static_cast<PluginLoader*>(param);
    self->pluginTaskLoop();
    self->pluginTask_ = nullptr;
    vTaskDelete(nullptr);
}

void PluginLoader::pluginTaskLoop() {
    while (!exitRequested_) {
        uint32_t now = millis();

        // Feed watchdog
        watchdogLastFeedMs_ = now;

        // Call plugin update
        if (vtable_.update) {
            vtable_.update(now);
        }

        // Call plugin draw
        if (vtable_.draw) {
            vtable_.draw();
        }

        // Yield (~30fps)
        vTaskDelay(pdMS_TO_TICKS(kTaskLoopDelayMs));
    }

    // Clean exit: call plugin_destroy
    if (vtable_.destroy) {
        vtable_.destroy();
    }
}

void PluginLoader::setupDeviceServices(const char* pluginId) {
    pluginStorageRoot_ = String("/plugins/") + pluginId + "/";

    DeviceServicesBridge::setup(
        pluginId,
        pluginStorageRoot_.c_str(),
        display_,
        audio_,
        &displayService_,
        &audioService_,
        &imuService_,
        &storageService_,
        &orientationService_);

    context_.display = &displayService_;
    context_.audio = &audioService_;
    context_.imu = &imuService_;
    context_.storage = &storageService_;
    context_.orientation = &orientationService_;
    context_.firmwareVersion = 1;
}

void PluginLoader::teardownDeviceServices() {
    DeviceServicesBridge::teardown();

    context_.display = nullptr;
    context_.audio = nullptr;
    context_.imu = nullptr;
    context_.storage = nullptr;
    context_.orientation = nullptr;
    pluginStorageRoot_ = "";
}

void PluginLoader::terminatePluginTask() {
    if (pluginTask_ != nullptr) {
        vTaskDelete(pluginTask_);
        pluginTask_ = nullptr;
        ESP_LOGW(TAG, "Plugin task force-terminated");
    }
}
