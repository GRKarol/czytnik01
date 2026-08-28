#pragma once

#include <cstddef>

// Help texts for each setting. Shown when user presses boot button in settings.
// Two description lines to fit on the small display.
// ASCII-only (no diacritics - font doesn't support them).

struct HelpEntry {
  const char* titleEn;
  const char* titlePl;
  const char* line1En;  // max ~35 chars per line
  const char* line1Pl;
  const char* line2En;  // optional second line
  const char* line2Pl;
};

namespace HelpTexts {

inline const HelpEntry* getDisplayHelp(size_t itemIndex) {
  static const HelpEntry entries[] = {
    {"Theme", "Motyw",
     "Light/Dark/Night color scheme.", "Jasny/Ciemny/Nocny schemat.",
     "Also changes brightness range.", "Zmienia tez zakres jasnosci."},
    {"Brightness", "Jasnosc",
     "Screen brightness level.", "Poziom jasnosci ekranu.",
     "Night mode caps at 55%.", "Tryb nocny max 55%."},
    {"Reader hand", "Reka",
     "Which hand holds the device.", "Ktora reka trzyma czytnik.",
     "Moves controls to that side.", "Przesuwa sterowanie na te strone."},
    {"Save button", "Zapis",
     "Save point button in reader.", "Przycisk zapisu w czytniku.",
     "Tap it to bookmark position.", "Dotknij by zapisac pozycje."},
    {"Footer", "Stopka",
     "Info at bottom while reading.", "Info na dole przy czytaniu.",
     "%, chapter time, or book time.", "%, czas rozdzialu, lub ksiazki."},
    {"Battery label", "Bateria",
     "Battery display format.", "Format wyswietlania baterii.",
     "Percent, time left, or voltage.", "Procent, czas, lub napiecie."},
    {"Screensaver", "Wygaszacz",
     "Animation when device is idle.", "Animacja przy bezczynnosci.",
     "Life, Maze, Voronoi, or off.", "Life, Maze, Voronoi, lub wyl."},
    {"Reading battery", "Bat. w czytaniu",
     "Battery icon while reading.", "Ikona baterii przy czytaniu.",
     "Hide to gain more text space.", "Ukryj by miec wiecej miejsca."},
    {"Reading chapter", "Rozdzial",
     "Chapter name while reading.", "Nazwa rozdzialu przy czytaniu.",
     "Shows current chapter at top.", "Pokazuje rozdzial u gory."},
    {"Reading percent", "Procent",
     "Progress % while reading.", "Procent postepu przy czytaniu.",
     "Shows how far in the book.", "Pokazuje ile przeczytano."},
    {"Language", "Jezyk",
     "Interface language.", "Jezyk interfejsu.",
     "For all menus and labels.", "Dla menu i etykiet."},
    {"Focus color", "Kolor fokusa",
     "Color of the ORP letter.", "Kolor litery ORP.",
     "The highlighted center letter.", "Podswietlona srodkowa litera."},
    {"Help (?)", "Pomoc (?)",
     "Show ? on selected item.", "Pokazuj ? przy wybranej opcji.",
     "Press side btn to see help.", "Nacisnij bok. przycisk = pomoc."},
  };
  constexpr size_t count = sizeof(entries) / sizeof(entries[0]);
  if (itemIndex < count) return &entries[itemIndex];
  return nullptr;
}

inline const HelpEntry* getPacingHelp(size_t itemIndex) {
  static const HelpEntry entries[] = {
    {"Reading mode", "Tryb czytania",
     "RSVP = one word at a time.", "RSVP = jedno slowo na raz.",
     "Scroll = pages of text.", "Scroll = strony tekstu."},
    {"Pause mode", "Tryb pauzy",
     "Sentence: pauses at period.", "Zdanie: pauzuje na kropce.",
     "Instant: pauses immediately.", "Natychm.: pauzuje od razu."},
    {"Base speed", "Predkosc",
     "Words per minute (WPM).", "Slowa na minute (WPM).",
     "Higher value = faster reading.", "Wyzsza wartosc = szybciej."},
    {"Long words", "Dlugie slowa",
     "Extra ms for long words.", "Dodatkowe ms dla dl. slow.",
     "0 = no extra delay.", "0 = bez dodatkowego czasu."},
    {"Complexity", "Zlozonosc",
     "Extra ms for complex words.", "Dodatkowe ms dla zlozonych.",
     "Hyphens, numbers, symbols.", "Myslniki, liczby, symbole."},
    {"Punctuation", "Interpunkcja",
     "Pause after . , ! ? marks.", "Pauza po . , ! ? znakach.",
     "Simulates speech pauses.", "Symuluje przerwy w mowie."},
  };
  constexpr size_t count = sizeof(entries) / sizeof(entries[0]);
  if (itemIndex < count) return &entries[itemIndex];
  return nullptr;
}

inline const HelpEntry* getPacingScrollHelp(size_t itemIndex) {
  static const HelpEntry entries[] = {
    {"Reading mode", "Tryb czytania",
     "RSVP = one word at a time.", "RSVP = jedno slowo na raz.",
     "Scroll = pages of text.", "Scroll = strony tekstu."},
    {"Font size", "Czcionka",
     "Text size in scroll mode.", "Rozmiar tekstu w scroll.",
     "Larger = easier to read.", "Wieksza = latwiej czytac."},
    {"Line spacing", "Interlinia",
     "Space between text lines.", "Odstep miedzy wierszami.",
     "Compact, Normal, or Relaxed.", "Compact, Normal, Relaxed."},
    {"Margins", "Marginesy",
     "Left/right margins.", "Lewy/prawy margines.",
     "Narrow, Normal, or Wide.", "Waski, Normalny, Szeroki."},
  };
  constexpr size_t count = sizeof(entries) / sizeof(entries[0]);
  if (itemIndex < count) return &entries[itemIndex];
  return nullptr;
}

} // namespace HelpTexts
