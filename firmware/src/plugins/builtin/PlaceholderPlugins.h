// firmware/src/plugins/builtin/PlaceholderPlugins.h
#pragma once

#include "plugins/sdk/PluginSdk.h"

/**
 * PlaceholderPlugins — throwaway stub plugins used only to preview the
 * Aktywne/Biblioteka plugin screens with real enable/disable state before
 * any real plugin exists again. Each one just shows a static "not a real
 * plugin yet" screen; exiting is handled by App at the power-button level,
 * same as every other plugin.
 */
namespace PlaceholderPlugins {
    PluginVTable nightReading();
    PluginVTable pageCounter();
    PluginVTable quoteHighlight();
    PluginVTable readingStats();
    PluginVTable notesSync();
}  // namespace PlaceholderPlugins
