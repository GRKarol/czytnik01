// plugins/rss/src/main.cpp
#include "PluginSdk.h"
#include "RssPluginCore.h"
#include "plugin_new.h"

static PluginContext* ctx = nullptr;

// Static storage for the core instance (no heap allocation needed)
static unsigned char coreStorage[sizeof(RssPluginCore)] __attribute__((aligned(4)));
static RssPluginCore* core = nullptr;

extern "C" {

// Forward declarations
PluginResult plugin_init(PluginContext* context);
void plugin_destroy(void);
void plugin_update(uint32_t nowMs);
void plugin_handle_button(const PluginButtonEvent* event);
void plugin_handle_touch(const PluginTouchEvent* event);
void plugin_draw(void);
PluginInfo plugin_get_info(void);

// Place header in .plugin_header section
__attribute__((used, section(".plugin_header")))
const PluginBinaryHeader pluginHeader = {
    .magic = PLUGIN_HEADER_MAGIC,
    .sdkVersion = PLUGIN_SDK_VERSION,
    .binarySize = 0,  // filled by build tool post-process
    .entryOffset = 0, // filled by build tool post-process
    .reserved = {0, 0, 0, 0},
};

// Place vtable in .plugin_vtable section
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
    core = new (coreStorage) RssPluginCore(ctx->display, ctx->storage);
    if (!core->begin()) {
        core->~RssPluginCore();
        core = nullptr;
        return PLUGIN_ERROR_INIT;
    }
    return PLUGIN_OK;
}

void plugin_destroy(void) {
    if (core) {
        core->~RssPluginCore();
        core = nullptr;
    }
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

void plugin_draw(void) {
    if (core) core->draw();
}

PluginInfo plugin_get_info(void) {
    PluginInfo info;
    info.name = "RSS Feeds";
    info.version = "1.0.0";
    info.sdkVersion = PLUGIN_SDK_VERSION;
    return info;
}

} // extern "C"
