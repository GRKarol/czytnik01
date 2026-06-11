// firmware/src/plugins/BuiltinPlugins.h
#pragma once

#include "plugins/sdk/PluginSdk.h"

/**
 * BuiltinPlugins — registry of all plugins compiled into the firmware binary.
 *
 * Plugin code is always present in the firmware. A plugin is considered
 * "installed" (activated) only when its manifest exists on SD card at
 * /plugins/{id}/manifest.json. Launching a plugin calls the built-in code
 * directly — no PSRAM binary loading needed.
 */

struct BuiltinPlugin {
    const char* id;         // e.g. "focus-timer", "rss"
    const char* name;       // human-readable name
    PluginVTable vtable;    // function pointers to built-in code
};

namespace BuiltinPlugins {

/// Find a built-in plugin by its ID. Returns nullptr if not found.
const BuiltinPlugin* find(const char* pluginId);

/// Get the full list of built-in plugins and count.
const BuiltinPlugin* all();
size_t count();

}  // namespace BuiltinPlugins
