// plugins/rss/src/main.cpp
#include "PluginSdk.h"
#include "RssPluginCore.h"

static RssPluginCore* core = nullptr;
static PluginContext* ctx = nullptr;

// Forward declarations
extern "C" PluginResult plugin_init(PluginContext* context);
extern "C" void plugin_destroy();
extern "C" void plugin_update(uint32_t nowMs);
extern "C" void plugin_handle_button(const PluginButtonEvent* event);
extern "C" void plugin_handle_touch(const PluginTouchEvent* event);
extern "C" void plugin_draw();
extern "C" PluginInfo plugin_get_info();

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

// Place vtable in .plugin_vtable section
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
    core = new RssPluginCore(ctx->display, ctx->storage);
    if (!core) return PLUGIN_ERROR_MEMORY;
    if (!core->begin()) return PLUGIN_ERROR_INIT;
    return PLUGIN_OK;
}

void plugin_destroy() {
    delete core;
    core = nullptr;
    ctx = nullptr;
}

void plugin_update(uint32_t nowMs) {
    if (core) core->update(nowMs);
}

void plugin_handle_button(const PluginButtonEvent* event) {
    if (core) core->handleButton(event);
}

void plugin_handle_touch(const PluginTouchEvent* event) {
    if (core) core->handleTouch(event);
}

void plugin_draw() {
    if (core) core->draw();
}

PluginInfo plugin_get_info() {
    return {
        .name = "RSS Feeds",
        .version = "1.0.0",
        .sdkVersion = PLUGIN_SDK_VERSION,
    };
}

} // extern "C"
