// firmware/src/plugins/DeviceServicesBridge.h
#pragma once

#include "plugins/sdk/PluginDisplayService.h"
#include "plugins/sdk/PluginAudioService.h"
#include "plugins/sdk/PluginImuService.h"
#include "plugins/sdk/PluginStorageService.h"
#include "plugins/sdk/PluginOrientationService.h"

class DisplayManager;
class AudioManager;
class AudioRecorder;

/**
 * DeviceServicesBridge — static bridge between C function-pointer-based
 * Plugin SDK services and the C++ firmware manager objects.
 *
 * Uses module-level static pointers so that plain C function pointers
 * (required by the plugin ABI) can forward calls to the real managers.
 *
 * Lifecycle:
 *   1. Call setup() with pointers to firmware managers and the plugin's storage root.
 *   2. Plugin runs — its service struct function pointers call the bridge statics.
 *   3. Call teardown() when the plugin is unloaded.
 */
namespace DeviceServicesBridge {

/// Wire up all service structs to forward to the given firmware managers.
/// @param pluginId        Current plugin identifier (for storage sandboxing)
/// @param storageRoot     Absolute SD path prefix, e.g. "/plugins/focus-timer/"
/// @param display         Pointer to the firmware's DisplayManager instance
/// @param audio           Pointer to the firmware's AudioManager instance
/// @param recorder        Pointer to the firmware's AudioRecorder instance
/// @param displayService  Struct to populate with display function pointers
/// @param audioService    Struct to populate with audio function pointers
/// @param imuService      Struct to populate with IMU function pointers
/// @param storageService  Struct to populate with storage function pointers
/// @param orientationService Struct to populate with orientation function pointers
void setup(const char* pluginId,
           const char* storageRoot,
           DisplayManager* display,
           AudioManager* audio,
           AudioRecorder* recorder,
           PluginDisplayService* displayService,
           PluginAudioService* audioService,
           PluginImuService* imuService,
           PluginStorageService* storageService,
           PluginOrientationService* orientationService);

/// Release bridge resources and null-out static pointers.
void teardown();

}  // namespace DeviceServicesBridge
