// firmware/src/plugins/builtin/PlaceholderPlugins.cpp
#include "plugins/builtin/PlaceholderPlugins.h"
#include "plugins/sdk/PluginDisplayService.h"

namespace {

PluginDisplayService* s_display = nullptr;

PluginResult placeholderInit(PluginContext* ctx) {
    s_display = ctx->display;
    return PLUGIN_OK;
}

void placeholderDestroy() {
    s_display = nullptr;
}

void placeholderUpdate(uint32_t) {}
void placeholderHandleButton(const PluginButtonEvent*) {}
void placeholderHandleTouch(const PluginTouchEvent*) {}

}  // namespace

// One DEFINE per placeholder — mechanical vtable glue, kept as repetition
// on purpose (each entry is a distinct set of C function pointers, so it
// can't be collapsed into a single runtime-parameterized instance).
#define DEFINE_PLACEHOLDER_PLUGIN(fnName, displayName)                      \
    namespace {                                                             \
    void fnName##Draw() {                                                   \
        if (s_display && s_display->renderStatus) {                        \
            s_display->renderStatus(displayName, "Plugin demonstracyjny",  \
                                    "Wroc: przycisk zasilania");            \
        }                                                                   \
    }                                                                       \
    PluginInfo fnName##GetInfo() {                                          \
        return {displayName, "0.1.0", PLUGIN_SDK_VERSION};                  \
    }                                                                       \
    }                                                                       \
    PluginVTable PlaceholderPlugins::fnName() {                             \
        return {                                                            \
            placeholderInit,      placeholderDestroy,   placeholderUpdate,  \
            placeholderHandleButton, placeholderHandleTouch, fnName##Draw,  \
            fnName##GetInfo,                                                \
        };                                                                  \
    }

DEFINE_PLACEHOLDER_PLUGIN(nightReading, "Tryb nocnego czytania")
DEFINE_PLACEHOLDER_PLUGIN(pageCounter, "Licznik stron")
DEFINE_PLACEHOLDER_PLUGIN(quoteHighlight, "Cytaty")
DEFINE_PLACEHOLDER_PLUGIN(readingStats, "Statystyki czytania")
DEFINE_PLACEHOLDER_PLUGIN(notesSync, "Notatki")

#undef DEFINE_PLACEHOLDER_PLUGIN
