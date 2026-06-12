// firmware/src/plugins/BuiltinPlugins.cpp
#include "plugins/BuiltinPlugins.h"
#include "plugins/builtin/FocusTimerPlugin.h"
#include "plugins/builtin/RssPlugin.h"
#include "plugins/builtin/DictaphonePlugin.h"

#include <string.h>

static const BuiltinPlugin kBuiltinPlugins[] = {
    {"focus-timer",  "Focus Timer",  FocusTimerPlugin::vtable()},
    {"rss",          "RSS Reader",   RssPlugin::vtable()},
    {"dictaphone",   "Dictaphone",   DictaphonePlugin::vtable()},
};

static constexpr size_t kBuiltinPluginCount =
    sizeof(kBuiltinPlugins) / sizeof(kBuiltinPlugins[0]);

const BuiltinPlugin* BuiltinPlugins::find(const char* pluginId) {
    if (!pluginId) return nullptr;
    for (size_t i = 0; i < kBuiltinPluginCount; ++i) {
        if (strcmp(kBuiltinPlugins[i].id, pluginId) == 0) {
            return &kBuiltinPlugins[i];
        }
    }
    return nullptr;
}

const BuiltinPlugin* BuiltinPlugins::all() {
    return kBuiltinPlugins;
}

size_t BuiltinPlugins::count() {
    return kBuiltinPluginCount;
}
