// plugins/focus-timer/src/main.cpp
// FocusTimer (Klepsydra) plugin entry point
#include "PluginSdk.h"
#include "FocusTimerCore.h"
#include "plugin_new.h"

static PluginContext* ctx = nullptr;

// Static storage for the core instance (no heap allocation needed)
static unsigned char timerStorage[sizeof(FocusTimerCore)] __attribute__((aligned(4)));
static FocusTimerCore* timer = nullptr;

extern "C" {

// Forward declarations of lifecycle functions
PluginResult plugin_init(PluginContext* context);
void plugin_destroy(void);
void plugin_update(uint32_t nowMs);
void plugin_handle_button(const PluginButtonEvent* event);
void plugin_handle_touch(const PluginTouchEvent* event);
void plugin_draw(void);
PluginInfo plugin_get_info(void);

// Place header in .plugin_header section — binarySize and entryOffset filled by post-build script
__attribute__((used, section(".plugin_header")))
const PluginBinaryHeader pluginHeader = {
    .magic = PLUGIN_HEADER_MAGIC,
    .sdkVersion = PLUGIN_SDK_VERSION,
    .binarySize = 0,   // filled by build tool post-process
    .entryOffset = 0,  // filled by build tool post-process
    .reserved = {0, 0, 0, 0},
};

// Place vtable in .plugin_vtable section — resolved by loader at entryOffset
__attribute__((used, section(".plugin_vtable")))
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
    // Placement new into static buffer — no heap allocation
    timer = new (timerStorage) FocusTimerCore(ctx->imu, ctx->audio, ctx->display, ctx->orientation);
    if (!timer->begin()) {
        timer->~FocusTimerCore();
        timer = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    return PLUGIN_OK;
}

void plugin_destroy(void) {
    if (timer) {
        timer->~FocusTimerCore();
        timer = nullptr;
    }
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

void plugin_draw(void) {
    if (timer) timer->draw();
}

PluginInfo plugin_get_info(void) {
    PluginInfo info;
    info.name = "Klepsydra";
    info.version = "1.0.0";
    info.sdkVersion = PLUGIN_SDK_VERSION;
    return info;
}

} // extern "C"
