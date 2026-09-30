// First-run wizard pages (Polish rows of tools/translations.csv).
#include <vector>

#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct WizSink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
bool gQr[29 * 29];
void fakeQr() {
  for (int y = 0; y < 29; ++y)
    for (int x = 0; x < 29; ++x) {
      const bool finder = (x < 7 && y < 7) || (x > 21 && y < 7) || (x < 7 && y > 21);
      const int fx = x > 21 ? x - 22 : x, fy = y > 21 ? y - 22 : y;
      const bool ring = finder && (fx == 0 || fx == 6 || fy == 0 || fy == 6 || (fx > 1 && fx < 5 && fy > 1 && fy < 5));
      gQr[y * 29 + x] = finder ? ring : (((x * 7 + y * 13) ^ (x * y)) % 3 == 0);
    }
}
}  // namespace

void runWizardScreens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  WizSink sink;
  fakeQr();
  {
    // Karta SD step: missing, unreadable, confirm, formatting, failed, plus
    // the longest German/French lines.
    struct SdPage {
      const char *name, *title, *sub, *next;
      bool loading;
    };
    const SdPage pages[] = {
        {"wizard_sd_1_missing", "W\x83""\xF3""\xB5"" kart\x99"" microSD",
         "Na karcie s\x97"" ksi\x97""\xB5""ki i czcionki. W\x83""\xF3""\xB5"" j\x97"" i dotknij Sprawd\xB3"".", "Sprawd\xB3""", false},
        {"wizard_sd_2_unreadable", "Karta wymaga formatowania",
         "Czytnik nie mo\xB5""e jej odczyta\x9B"". Formatowanie usunie wszystko, co na niej jest.", "Formatuj", false},
        {"wizard_sd_3_confirm", "Karta wymaga formatowania", "Na pewno? Tego nie da si\x99"" cofn\x97""\x9B"".",
         "Tak, formatuj", false},
        {"wizard_sd_4_formatting", "Formatuj\x99"" kart\x99""", "Nie wyjmuj karty", "", true},
        {"wizard_sd_5_failed", "Nie uda\x83""o si\x99"" sformatowa\x9B""",
         "Sformatuj kart\x99"" w komputerze jako FAT32 albo u\xB5""yj innej.", "Formatuj", false},
        {"wizard_sd_6_de", "Die Karte muss formatiert werden",
         "Der Reader kann sie nicht lesen. Formatieren l\xF6""scht alles darauf.", "Formatieren", false},
        {"wizard_sd_7_fr", "\xC9""chec du formatage",
         "Formatez la carte en FAT32 sur un ordinateur ou utilisez-en une autre.", "Formater", false},
    };
    for (const SdPage &pg : pages) {
      nano::WizardView v;
      v.step = 1;
      v.stepCount = 10;
      v.body = pg.loading ? nano::WizardBody::Loading : nano::WizardBody::Message;
      v.title = pg.title;
      v.subtitle = pg.sub;
      if (!pg.loading) {
        v.backId = 2;
        v.backLabel = "Wr\xF3""\x9B""";
        v.nextId = 1;
        v.nextLabel = pg.next;
        v.extraId = 3;
        v.extraLabel = "Pomi\x9D""";
      }
      v.phase = 25;
      nano::paintWizard(d, sink, v);
      dump(d, pg.name);
    }
  }
  {
    nano::WizardView v;
    v.step = 0;
    v.stepCount = 10;
    v.body = nano::WizardBody::Chips;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Wybierz j\x99""zyk";
    v.subtitle = "Menu b\x99""dzie w tym j\x99""zyku. Ksi\x97""\xB5""ki zostaj\x97"" w swoim.";
    v.backId = -1;
    v.footer = "Przycisk zasilania cofa o krok";
    {
      nano::WizardChip c;
      c.id = 100;
      c.label = "English";
      c.selected = false;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 101;
      c.label = "Polski";
      c.selected = true;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 102;
      c.label = "Deutsch";
      c.selected = false;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 103;
      c.label = "Espanol";
      c.selected = false;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 104;
      c.label = "Francais";
      c.selected = false;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 105;
      c.label = "Romana";
      c.selected = false;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_1_language");
  }
  {
    nano::WizardView v;
    v.step = 1;
    v.stepCount = 10;
    v.body = nano::WizardBody::Chips;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Wybierz motyw";
    v.subtitle = "Kolory ekranu czytania. Zmienisz je p\xF3""\xB3""niej w Motywach.";
    {
      nano::WizardChip c;
      c.id = 100;
      c.label = "Jasny";
      c.selected = false;
      c.art = nano::WizardChipArt::Theme;
      c.theme = 1;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 101;
      c.label = "Ciemny";
      c.selected = true;
      c.art = nano::WizardChipArt::Theme;
      c.theme = 0;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 102;
      c.label = "Nocny";
      c.selected = false;
      c.art = nano::WizardChipArt::Theme;
      c.theme = 2;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_2_theme");
  }
  {
    nano::WizardView v;
    v.step = 2;
    v.stepCount = 10;
    v.body = nano::WizardBody::Chips;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Wybierz kolor pod\x9F""wietlenia";
    v.subtitle = "Kolor litery, na kt\xF3""r\x97"" patrzysz przy czytaniu.";
    {
      nano::WizardChip c;
      c.id = 100;
      c.label = "Czerwony";
      c.selected = false;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(0);
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 101;
      c.label = "Niebieski";
      c.selected = true;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(1);
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 102;
      c.label = "Zielony";
      c.selected = false;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(2);
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 103;
      c.label = "\xB4""\xF3""\x83""ty";
      c.selected = false;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(3);
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 104;
      c.label = "Pomara\x9D""czowy";
      c.selected = false;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(4);
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 105;
      c.label = "Fioletowy";
      c.selected = false;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(5);
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_3_color");
  }
  {
    nano::WizardView v;
    v.step = 4;
    v.stepCount = 10;
    v.body = nano::WizardBody::Loading;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Odzyskaj stan Flow";
    v.subtitle = "Pobieranie potrzebnych zasob\xF3""w";
    v.phase = 17;
    v.backId = -1;
    v.nextId = -1;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_5_loading");
  }
  {
    nano::WizardView v;
    v.step = 4;
    v.stepCount = 10;
    v.body = nano::WizardBody::Message;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Super!";
    v.subtitle = "Podstawy gotowe";
    v.autoPercent = 60;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_5_super");
  }
  {
    nano::WizardView v;
    v.step = 6;
    v.stepCount = 10;
    v.body = nano::WizardBody::Chips;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Wybierz spos\xF3""b czytania!";
    v.subtitle = "RSVP: jedno s\x83""owo naraz. Przewijanie: zwyk\x83""y tekst, kt\xF3""ry sam p\x83""ynie.";
    v.extraId = 3;
    v.extraLabel = "Podgl\x97""d";
    {
      nano::WizardChip c;
      c.id = 100;
      c.label = "RSVP";
      c.selected = false;
      c.art = nano::WizardChipArt::Rsvp;
      v.chips.push_back(c);
    }
    {
      nano::WizardChip c;
      c.id = 101;
      c.label = "Przewijanie strony";
      c.selected = true;
      c.art = nano::WizardChipArt::Scroll;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_7_mode");
  }
  {
    nano::WizardView v;
    v.step = 7;
    v.stepCount = 10;
    v.body = nano::WizardBody::Qr;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Po\x83""\x97""cz czytnik z telefonem!";
    v.subtitle = "Zeskanuj i pobierz aplikacj\x99"" Flower";
    v.qr = gQr;
    v.qrSize = 29;
    v.qrLine = "flower.theworkpc.com/appdownload";
    v.qrHint = "Zeskanuj kod aparatem telefonu";
    v.nextId = -1;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_8_connect");
  }
  {
    nano::WizardView v;
    v.step = 8;
    v.stepCount = 10;
    v.body = nano::WizardBody::Qr;
    v.backId = 2;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 1;
    v.nextLabel = "Dalej";
    v.title = "Po\x83""\x97""czenie z aplikacj\x97""";
    v.subtitle = "Otw\xF3""rz aplikacj\x99"" Flower i zeskanuj ten kod";
    v.qr = gQr;
    v.qrSize = 29;
    v.qrLine = "flower-4F2A";
    v.qrHint = "Oczekiwanie na po\x83""\x97""czenie... dotknij, aby pomin\x97""\x9B""";
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_9_pairing");
  }
  for (int page = 0; page < 2; ++page) {
    nano::SyncView v;
    v.page = page;
    v.pageLabels[0] = "Po\x83""\x97""czenie";
    v.pageLabels[1] = "Pobierz aplikacj\x99""";
    v.pageIds[0] = 1;
    v.pageIds[1] = 2;
    v.stopId = 3;
    v.stopLabel = "Zako\x9D""cz";
    v.title = page == 0 ? "Po\x83""\x97""czenie z aplikacj\x97""" : "Aplikacja Flower";
    v.line = page == 0 ? "Flower-4F2A" : "flower.theworkpc.com/appdownload";
    v.hint = page == 0 ? "Zeskanuj kod w aplikacji Flower albo po\x83""\x97""cz telefon z t\x97"" sieci\x97"""
                       : "Zeskanuj aparatem telefonu i zainstaluj aplikacj\x99"". Potem wr\xF3""\x9B"" na stron\x99"" Po\x83""\x97""czenie.";
    v.qr = gQr;
    v.qrSize = 29;
    nano::paintSync(d, sink, v);
    dump(d, page == 0 ? "sync_pairing" : "sync_app");
  }
}
