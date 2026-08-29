// firmware/src/plugins/PluginLibrary.h
#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <vector>

/**
 * PluginLibrary — local registry of built-in plugins with enable/disable
 * state. Plugin code is always compiled into the firmware; there is no
 * network install/download step anymore. Enabling a plugin just flips a
 * persisted flag so it shows up on the Aktywne (active) screen — disabling
 * it removes it from Aktywne without touching RAM or flash.
 */
class PluginLibrary {
 public:
    struct Entry {
        String id;
        String name;
        String description;
        bool enabled;
    };

    bool begin();

    /// All built-in plugins, each annotated with its current enabled state.
    const std::vector<Entry>& all() const { return entries_; }

    /// Subset of all() where enabled == true, in registry order.
    std::vector<Entry> enabledEntries() const;

    bool isEnabled(const char* id) const;
    void setEnabled(const char* id, bool enabled);

 private:
    void refresh();

    Preferences prefs_;
    std::vector<Entry> entries_;
};
