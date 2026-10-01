// Wizard pages added 2026-10-01, drawn with the real translation tables:
// loading with tips, menu font, reading font, the six starter books.
#include <vector>

#include "app/Translations.h"
#include "app/generated/StarterTitles.h"
#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct Wiz3Sink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
const char *t3(TrKey3 key, uint8_t lang) { return TranslationsData::trKey3Lookup(static_cast<uint8_t>(key), lang); }
const char *t4(TrKey4 key, uint8_t lang) { return TranslationsData::trKey4Lookup(static_cast<uint8_t>(key), lang); }
nano::WizardView base(size_t step, uint8_t lang) {
  nano::WizardView v;
  v.step = step;
  v.stepCount = 12;
  v.backId = 2;
  v.backLabel = TranslationsData::uiTextLookup(static_cast<uint8_t>(UiText::Back), lang);
  v.nextId = 1;
  v.nextLabel = t3(TrKey3::NextLabel, lang);
  return v;
}
// UiLanguage: 0 en, 1 es, 2 fr, 3 de, 4 ro, 5 pl.
const char *kLangCode[] = {"en", "es", "fr", "de", "ro", "pl"};
}  // namespace

void runWizard3Screens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  Wiz3Sink sink;
  const TrKey4 tips[] = {TrKey4::WizTip1, TrKey4::WizTip2, TrKey4::WizTip3, TrKey4::WizTip4, TrKey4::WizTip5,
                         TrKey4::WizTip6, TrKey4::WizTip7, TrKey4::WizTip8, TrKey4::WizTip9, TrKey4::WizTip10};
  for (uint8_t lang = 0; lang < 6; ++lang) {
    // Menu font as on a new reader (Literata, follows Standard).
    d.setNanoUiFont(4);
    for (int i = 0; i < 10; ++i) {
      nano::WizardView v = base(4, lang);
      v.body = nano::WizardBody::Loading;
      v.backId = -1;
      v.nextId = -1;
      v.title = i % 2 ? t4(TrKey4::WizPhrase5, lang) : t3(TrKey3::WelcomeLoadingPhrase3, lang);
      v.subtitle = String(t4(TrKey4::WizTipLabel, lang)) + " " + t4(tips[i], lang);
      v.status = String(t4(TrKey4::WizFontsProgress, lang)) + "  7/17";
      v.percent = 41;
      nano::paintWizard(d, sink, v);
      dump(d, (String("w3_loading_") + kLangCode[lang] + "_tip" + String(i + 1)).c_str());
    }
    {
      nano::WizardView v = base(6, lang);
      v.title = t4(TrKey4::WizMenuFontTitle, lang);
      v.subtitle = t4(TrKey4::WizMenuFontSub, lang);
      v.chipRows = 2;
      v.chipColumns = 3;
      for (uint8_t f = 0; f < DisplayManager::nanoUiFontCount(); ++f) {
        nano::WizardChip c;
        c.id = 100 + f;
        c.art = nano::WizardChipArt::UiFont;
        c.family = f;
        c.label = DisplayManager::nanoUiFontName(f);
        c.selected = f == 4;
        v.chips.push_back(c);
      }
      nano::paintWizard(d, sink, v);
      dump(d, (String("w3_menufont_") + kLangCode[lang]).c_str());
    }
    {
      nano::WizardView v = base(7, lang);
      v.title = t4(TrKey4::WizReadFontTitle, lang);
      v.subtitle = t4(TrKey4::WizFontSub, lang);
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
        c.selected = i == 0;
        v.chips.push_back(c);
      }
      nano::paintWizard(d, sink, v);
      dump(d, (String("w3_readfont_") + kLangCode[lang]).c_str());
    }
    for (int variant = 0; variant < 2; ++variant) {
      nano::WizardView v = base(11, lang);
      v.title = t3(TrKey3::WelcomeBookPickerTitle, lang);
      v.nextLabel = TranslationsData::uiTextLookup(static_cast<uint8_t>(UiText::Read), lang);
      v.extraId = 3;
      v.extraLabel = TranslationsData::trKey2Lookup(static_cast<uint8_t>(TrKey2::SkipForNow), lang);
      v.tallChips = true;
      v.chipRows = 2;
      v.chipColumns = 3;
      v.footer = variant ? t4(TrKey4::WizBookFailed, lang) : t4(TrKey4::WizBooksStillLoading, lang);
      for (uint8_t slot = 0; slot < StarterTitles::kPerLanguage; ++slot) {
        nano::WizardChip c;
        c.id = 100 + slot;
        c.art = nano::WizardChipArt::Book;
        c.label = StarterTitles::kTitles[lang][slot].title;
        c.detail = StarterTitles::kTitles[lang][slot].author;
        c.selected = slot == 0;
        v.chips.push_back(c);
      }
      nano::paintWizard(d, sink, v);
      dump(d, (String("w3_library_") + kLangCode[lang] + (variant ? "_failed" : "")).c_str());
    }
    {
      nano::WizardView v = base(11, lang);
      v.body = nano::WizardBody::Loading;
      v.title = t4(TrKey4::WizBookDownloading, lang);
      v.subtitle = StarterTitles::kTitles[lang][0].title;
      v.nextId = -1;
      v.backId = -1;
      v.extraId = 3;
      v.extraLabel = TranslationsData::trKey2Lookup(static_cast<uint8_t>(TrKey2::SkipForNow), lang);
      v.phase = 20;
      nano::paintWizard(d, sink, v);
      dump(d, (String("w3_library_wait_") + kLangCode[lang]).c_str());
    }
  }
}
