// firmware/src/plugins/BuiltinPlugins.cpp
#include "plugins/BuiltinPlugins.h"
#include "plugins/builtin/PlaceholderPlugins.h"
#include "plugins/builtin/DictaphonePlugin.h"
#include "plugins/builtin/FocusTimerPlugin.h"

#include <string.h>

// Dictaphone and Focus Timer are real plugins rebuilt on top of the
// Aktywne/Biblioteka redesign. The rest are still placeholder entries so
// the screens can be reviewed with real enable/disable state before more
// real plugins exist.
static const BuiltinPlugin kBuiltinPlugins[] = {
    {"dictaphone", "Dyktafon",
     "Nagrywa dzwiek z mikrofonu na karte SD. Odtwarzaj i zarzadzaj "
     "nagraniami w bibliotece.",
     DictaphonePlugin::vtable()},
    {"focus-timer", "Klepsydra",
     "Sesje skupienia sterowane odwracaniem urzadzenia — postaw na "
     "krotszym boku, by zaczac, poloz plasko, by zapauzowac.",
     FocusTimerPlugin::vtable()},
    {"night-reading", "Tryb nocnego czytania",
     "Automatycznie przyciemnia ekran i wlacza cieplejszy odcien po zachodzie slonca.",
     PlaceholderPlugins::nightReading()},
    {"page-counter", "Licznik stron",
     "Pokazuje szacowana liczbe przeczytanych stron w biezacej sesji.",
     PlaceholderPlugins::pageCounter()},
    {"quote-highlight", "Cytaty",
     "Zaznacz fragment tekstu i zapisz go jako cytat do pozniejszego przejrzenia.",
     PlaceholderPlugins::quoteHighlight()},
    {"reading-stats", "Statystyki czytania",
     "Wykresy tempa czytania i czasu spedzonego w ksiazkach w ostatnim tygodniu.",
     PlaceholderPlugins::readingStats()},
    {"notes-sync", "Notatki",
     "Synchronizuje notatki z aplikacja towarzyszaca na telefonie.",
     PlaceholderPlugins::notesSync()},
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
