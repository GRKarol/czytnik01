// Wizard pages added 2026-09-30: menu look, typefaces, reading preview,
// loading with progress, starter library, Wi-Fi status.
#include <vector>

#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct Wiz2Sink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
String p2(const char *s) {
  String out;
  for (const char *p = s; *p; ++p) {
    if (*p == '~' && p[1]) {
      ++p;
      switch (*p) {
        case 'a': out += "\x97"; break;
        case 'c': out += "\x9B"; break;
        case 'e': out += "\x99"; break;
        case 'l': out += "\x83"; break;
        case 'n': out += "\x9D"; break;
        case 'o': out += "\xF3"; break;
        case 's': out += "\x9F"; break;
        case 'z': out += "\xB5"; break;
        case 'x': out += "\xB3"; break;
        default: out += *p;
      }
    } else {
      out += *p;
    }
  }
  return out;
}
nano::WizardView base(size_t step) {
  nano::WizardView v;
  v.step = step;
  v.stepCount = 11;
  v.backId = 2;
  v.backLabel = p2("Wr~o~c");
  v.nextId = 1;
  v.nextLabel = "Dalej";
  return v;
}
}  // namespace

void runWizard2Screens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  Wiz2Sink sink;
  {
    nano::WizardView v = base(5);
    v.title = p2("Wybierz wygl~ad menu");
    v.subtitle = p2("Kolory menu i ustawie~n. Ekran czytania zostaje taki, jak wybra~le~s.");
    v.chipRows = 2;
    v.chipColumns = 4;
    v.pageNextId = 5;
    for (uint8_t i = 0; i < 8; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.art = nano::WizardChipArt::Palette;
      c.palette = i;
      c.label = i == 0 ? String("Klasyczny") : String(DisplayManager::nanoPaletteName(i));
      c.selected = i == 0;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_6_menutheme");
  }
  {
    nano::WizardView v = base(6);
    v.title = p2("Wybierz czcionk~e!");
    v.subtitle = p2("Tak b~ed~a wygl~ada~c s~lowa w ksi~a~zce.");
    v.footer = p2("Reszta czcionek jeszcze si~e pobiera");
    v.chipRows = 2;
    v.chipColumns = 3;
    v.pageNextId = 5;
    const char *names[] = {"Standard", "OpenDyslexic", "Atkinson", "Literata", "Merriweather", "Lora"};
    for (uint8_t i = 0; i < 6; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.art = nano::WizardChipArt::Typeface;
      c.typeface = static_cast<DisplayManager::ReaderTypeface>(i);
      c.label = names[i];
      c.selected = i == 3;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_6_font");
  }
  {
    nano::WizardView v = base(7);
    v.body = nano::WizardBody::Preview;
    v.nextLabel = "Wybieram";
    v.previewMode = 0;
    v.previewBefore = "the";
    v.previewWord = "reading";
    v.previewAfter = "flows";
    v.previewSizeLevel = 1;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_7_preview_rsvp");
  }
  {
    nano::WizardView v = base(7);
    v.body = nano::WizardBody::Preview;
    v.nextLabel = "Wybieram";
    v.previewMode = 1;
    static std::vector<DisplayManager::ContextWord> words;
    const char *text[] = {"Tekst", "p~lynie", "sam,", "a", "ty", "czytasz", "w", "swoim", "tempie.", "Linie",
                          "przesuwaj~a", "si~e", "powoli", "do", "g~ory,", "bez", "przewracania", "stron."};
    for (int r = 0; r < 3; ++r)
      for (size_t i = 0; i < sizeof(text) / sizeof(text[0]); ++i) {
        DisplayManager::ContextWord w;
        w.text = p2(text[i]);
        w.paragraphStart = i == 0;
        words.push_back(w);
      }
    v.scrollWords = &words;
    v.scrollCurrent = 12;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_7_preview_scroll");
  }
  {
    nano::WizardView v = base(4);
    v.body = nano::WizardBody::Loading;
    v.backId = -1;
    v.nextId = -1;
    v.title = "Odzyskaj stan Flow";
    v.subtitle = p2("Pobieram potrzebne zasoby");
    v.status = p2("Pobieram czcionki  7/17");
    v.percent = 41;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_5_loading_fonts");
    v.status = p2("Pobieram now~a wersj~e  63%");
    v.percent = 63;
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_5_loading_update");
  }
  {
    nano::WizardView v = base(10);
    v.title = p2("Prawie gotowe! Co dzi~s czytamy?");
    v.subtitle = p2("Wybierz tytu~l na start. Kolejne dodasz z aplikacji Flower.");
    v.nextLabel = "Czytaj";
    v.extraId = 3;
    v.extraLabel = p2("Pomi~n");
    v.chipColumns = 3;
    v.pageNextId = 5;
    const char *titles[][2] = {{"W pustyni i w puszczy", "Henryk Sienkiewicz"},
                               {"Pan Tadeusz", "Adam Mickiewicz"},
                               {"Na srebrnym globie", p2("Jerzy ~Zu~lawski").c_str()}};
    for (int i = 0; i < 3; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.art = nano::WizardChipArt::Book;
      c.label = p2(titles[i][0]);
      c.detail = i == 2 ? p2("Jerzy ~Zu~lawski") : p2(titles[i][1]);
      c.selected = i == 0;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_11_library");
  }
  {
    nano::WizardView v = base(3);
    v.body = nano::WizardBody::Message;
    v.backId = -1;
    v.nextId = -1;
    v.title = p2("Szukam sieci Wi-Fi");
    v.subtitle = p2("To potrwa kilka sekund");
    nano::paintWizard(d, sink, v);
    dump(d, "wizard_4_wifi_scan");
  }
}
