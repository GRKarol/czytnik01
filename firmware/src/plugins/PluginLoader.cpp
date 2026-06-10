// firmware/src/plugins/PluginLoader.cpp
#include "plugins/PluginLoader.h"

#include <SD.h>
#include <esp_heap_caps.h>

#include "plugins/DeviceServicesBridge.h"
#include <esp_system.h>

static const char* TAG = "PluginLoader";

// Global pointer for the ESP-IDF shutdown/panic handler to access the loader.
// Only one PluginLoader instance exists in firmware (owned by App).
static PluginLoader* s_pluginLoaderInstance = nullptr;

/// ESP-IDF shutdown handler — called on panic/crash.
/// If the crash originated in the plugin task, marks it as crashed
/// so firmware can recover gracefully on the next main loop iteration.
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

    // Store global instance for the panic handler
    s_pluginLoaderInstance = this;

    // Register ESP-IDF shutdown handler to detect plugin task crashes
    esp_err_t err = esp_register_shutdown_handler(pluginPanicHandler);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to register shutdown handler: %d", err);
        // Non-fatal — loader still works, just without crash isolation
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

    // 1. Construct path: /plugins/{pluginId}/plugin.bin
    String path = String("/plugins/") + pluginId + "/plugin.bin";

    // 2. Open file and check existence
    File file = SD.open(path, FILE_READ);
    if (!file) {
        state_ = State::Error;
        lastError_ = ErrorCode::FileNotFound;
        lastErrorMessage_ = "Plugin binary not found on SD card";
        return {false, lastError_, lastErrorMessage_};
    }

    size_t fileSize = file.size();

    // 3. Sanity check: file must be at least header size
    if (fileSize < sizeof(PluginBinaryHeader)) {
        file.close();
        state_ = State::Error;
        lastError_ = ErrorCode::InvalidHeader;
        lastErrorMessage_ = "Binary too small for header";
        return {false, lastError_, lastErrorMessage_};
    }

    // 4. Check PSRAM capacity
    size_t available = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    if (fileSize > available) {
        file.close();
        state_ = State::Error;
        lastError_ = ErrorCode::OutOfMemory;
        lastErrorMessage_ = "Insufficient PSRAM for plugin binary";
        return {false, lastError_, lastErrorMessage_};
    }

    // 5. Allocate PSRAM buffer (executable)
    binaryBuffer_ = (uint8_t*)heap_caps_malloc(fileSize,
                                                MALLOC_CAP_SPIRAM | MALLOC_CAP_EXEC);
    if (!binaryBuffer_) {
        file.close();
        state_ = State::Error;
        lastError_ = ErrorCode::OutOfMemory;
        lastErrorMessage_ = "PSRAM allocation failed";
        return {false, lastError_, lastErrorMessage_};
    }
    binarySize_ = fileSize;
    psramUsed_ = fileSize;

    // 6. Read binary into PSRAM
    size_t bytesRead = file.read(binaryBuffer_, fileSize);
    file.close();

    if (bytesRead != fileSize) {
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::FileNotFound;
        lastErrorMessage_ = "Failed to read complete binary";
        return {false, lastError_, lastErrorMessage_};
    }

    // 7. Validate header
    PluginBinaryHeader* header = (PluginBinaryHeader*)binaryBuffer_;
    if (!validateHeader(header, fileSize)) {
        freePluginMemory();
        state_ = State::Error;
        return {false, lastError_, lastErrorMessage_};
    }

    // 8. Resolve VTable at entryOffset
    if (!resolveVTable(binaryBuffer_, header->entryOffset)) {
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::InvalidHeader;
        lastErrorMessage_ = "VTable resolution failed";
        return {false, lastError_, lastErrorMessage_};
    }

    // 9. Setup device services bridge
    setupDeviceServices(pluginId);

    // 10. Call plugin_init
    if (!vtable_->init) {
        teardownDeviceServices();
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::InitFailed;
        lastErrorMessage_ = "Plugin init function is null";
        return {false, lastError_, lastErrorMessage_};
    }

    PluginResult initResult = vtable_->init(&context_);
    if (initResult != PLUGIN_OK) {
        teardownDeviceServices();
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::InitFailed;
        lastErrorMessage_ = "plugin_init returned error";
        return {false, lastError_, lastErrorMessage_};
    }

    // 11. Create FreeRTOS task pinned to Core 1
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
        // Cleanup on task creation failure
        if (vtable_->destroy) {
            vtable_->destroy();
        }
        teardownDeviceServices();
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::OutOfMemory;
        lastErrorMessage_ = "FreeRTOS task creation failed";
        return {false, lastError_, lastErrorMessage_};
    }

    state_ = State::Running;
    ESP_LOGI(TAG, "Plugin '%s' loaded and running (%u bytes PSRAM)", pluginId, fileSize);
    return {true, ErrorCode::None, ""};
}

void PluginLoader::unload() {
    if (state_ != State::Running && state_ != State::Error) {
        return;
    }

    // Signal the plugin task to exit
    exitRequested_ = true;

    // Wait for task to finish (it will call plugin_destroy internally)
    if (pluginTask_ != nullptr) {
        // Give the task time to exit gracefully
        uint32_t waitStart = millis();
        const uint32_t kMaxWaitMs = 2000;

        while (pluginTask_ != nullptr && (millis() - waitStart) < kMaxWaitMs) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // If task didn't exit in time, force-terminate
        if (pluginTask_ != nullptr) {
            terminatePluginTask();
            // Call destroy since task didn't get to do it
            if (vtable_ && vtable_->destroy) {
                vtable_->destroy();
            }
        }
    }

    teardownDeviceServices();
    freePluginMemory();

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
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::WatchdogTimeout;
        lastErrorMessage_ = "Plugin stopped responding";
    }
}

size_t PluginLoader::psramAvailable() const {
    return heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

void PluginLoader::markCrashed() {
    if (state_ == State::Running) {
        pluginTask_ = nullptr;  // Task is already dead from crash
        freePluginMemory();
        state_ = State::Error;
        lastError_ = ErrorCode::Crashed;
        lastErrorMessage_ = "Plugin crashed";
    }
}

// ─── Private ────────────────────────────────────────────────────────────────

void PluginLoader::pluginTaskEntry(void* param) {
    PluginLoader* self = static_cast<PluginLoader*>(param);
    self->pluginTaskLoop();
    // Notify that task is done by nullifying handle before deletion
    self->pluginTask_ = nullptr;
    vTaskDelete(nullptr);
}

void PluginLoader::pluginTaskLoop() {
    while (!exitRequested_) {
        uint32_t now = millis();

        // Feed watchdog from within the task
        watchdogLastFeedMs_ = now;

        // Call plugin update
        if (vtable_->update) {
            vtable_->update(now);
        }

        // Call plugin draw
        if (vtable_->draw) {
            vtable_->draw();
        }

        // Yield to other tasks (~30fps for e-ink)
        vTaskDelay(pdMS_TO_TICKS(kTaskLoopDelayMs));
    }

    // Clean exit: call plugin_destroy
    if (vtable_ && vtable_->destroy) {
        vtable_->destroy();
    }
}

bool PluginLoader::validateHeader(const PluginBinaryHeader* header, size_t fileSize) {
    // Check magic number
    if (header->magic != PLUGIN_HEADER_MAGIC) {
        lastError_ = ErrorCode::InvalidHeader;
        lastErrorMessage_ = "Invalid plugin magic number";
        ESP_LOGE(TAG, "Bad magic: 0x%08X (expected 0x%08X)",
                 header->magic, PLUGIN_HEADER_MAGIC);
        return false;
    }

    // Check SDK version compatibility
    if (header->sdkVersion != PLUGIN_SDK_VERSION) {
        lastError_ = ErrorCode::SdkMismatch;
        lastErrorMessage_ = "Plugin SDK version incompatible — update firmware or plugin";
        ESP_LOGE(TAG, "SDK mismatch: plugin=%u, firmware=%u",
                 header->sdkVersion, PLUGIN_SDK_VERSION);
        return false;
    }

    // Check declared binary size matches actual file size
    if (header->binarySize != fileSize) {
        lastError_ = ErrorCode::InvalidHeader;
        lastErrorMessage_ = "Binary size mismatch in header";
        ESP_LOGE(TAG, "Size mismatch: header=%u, file=%u",
                 header->binarySize, (uint32_t)fileSize);
        return false;
    }

    // Check entry offset is within bounds
    if (header->entryOffset + sizeof(PluginVTable) > fileSize) {
        lastError_ = ErrorCode::InvalidHeader;
        lastErrorMessage_ = "Entry offset exceeds binary size";
        ESP_LOGE(TAG, "entryOffset=%u + vtable=%u > fileSize=%u",
                 header->entryOffset, (uint32_t)sizeof(PluginVTable),
                 (uint32_t)fileSize);
        return false;
    }

    return true;
}

bool PluginLoader::resolveVTable(uint8_t* base, uint32_t entryOffset) {
    if (!base || entryOffset == 0) {
        return false;
    }

    vtable_ = reinterpret_cast<PluginVTable*>(base + entryOffset);

    // Verify essential function pointers are non-null
    if (!vtable_->init) {
        ESP_LOGE(TAG, "VTable: init is null");
        return false;
    }

    // destroy, update, draw can be null (optional) but init is mandatory
    return true;
}

void PluginLoader::setupDeviceServices(const char* pluginId) {
    // Store the plugin storage root for sandboxed file access
    pluginStorageRoot_ = String("/plugins/") + pluginId + "/";

    // Wire up device service function pointers via the bridge module.
    // The bridge uses static pointers so C function pointers can reach C++ managers.
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
    context_.firmwareVersion = 1;  // Will come from build version
}

void PluginLoader::teardownDeviceServices() {
    // Release the bridge's static state
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

void PluginLoader::freePluginMemory() {
    if (binaryBuffer_ != nullptr) {
        heap_caps_free(binaryBuffer_);
        binaryBuffer_ = nullptr;
    }
    binarySize_ = 0;
    psramUsed_ = 0;
    vtable_ = nullptr;
}
