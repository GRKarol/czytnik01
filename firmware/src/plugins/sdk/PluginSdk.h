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
