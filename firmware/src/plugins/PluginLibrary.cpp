// firmware/src/plugins/PluginLibrary.cpp
#include "plugins/PluginLibrary.h"
#include "plugins/BuiltinPlugins.h"

namespace {

constexpr const char* kPrefsNamespace = "plugins";
constexpr const char* kEnabledKey = "enabled";

bool containsId(const String& csv, const char* id) {
    if (csv.isEmpty()) return false;
    const String needle = String(",") + id + ",";
    const String haystack = String(",") + csv + ",";
    return haystack.indexOf(needle) >= 0;
}

String withIdRemoved(const String& csv, const char* id) {
    std::vector<String> kept;
    int start = 0;
    while (start <= static_cast<int>(csv.length())) {
        int comma = csv.indexOf(',', start);
        if (comma < 0) comma = csv.length();
        String token = csv.substring(start, comma);
        if (!token.isEmpty() && token != id) {
            kept.push_back(token);
        }
        start = comma + 1;
    }
    String result;
    for (size_t i = 0; i < kept.size(); ++i) {
        if (i > 0) result += ",";
        result += kept[i];
    }
    return result;
}

}  // namespace

bool PluginLibrary::begin() {
    prefs_.begin(kPrefsNamespace, false);
    refresh();
    return true;
}

void PluginLibrary::refresh() {
    entries_.clear();
    const String enabledCsv = prefs_.getString(kEnabledKey, "");

    const BuiltinPlugin* plugins = BuiltinPlugins::all();
    const size_t count = BuiltinPlugins::count();
    entries_.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        Entry e;
        e.id = plugins[i].id;
        e.name = plugins[i].name;
        e.description = plugins[i].description;
        e.enabled = containsId(enabledCsv, plugins[i].id);
        entries_.push_back(e);
    }
}

std::vector<PluginLibrary::Entry> PluginLibrary::enabledEntries() const {
    std::vector<Entry> out;
    for (const auto& e : entries_) {
        if (e.enabled) out.push_back(e);
    }
    return out;
}

bool PluginLibrary::isEnabled(const char* id) const {
    for (const auto& e : entries_) {
        if (e.id == id) return e.enabled;
    }
    return false;
}

void PluginLibrary::setEnabled(const char* id, bool enabled) {
    String csv = prefs_.getString(kEnabledKey, "");
    csv = withIdRemoved(csv, id);
    if (enabled) {
        csv = csv.isEmpty() ? String(id) : (csv + "," + id);
    }
    prefs_.putString(kEnabledKey, csv);
    refresh();
}
