// Screens changed in v0.4.11 (2026-10-11), built the way App builds them,
// with the real Polish translation tables.
#include <vector>

#include "app/Translations.h"
#include "app/generated/HelpData.h"
#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct V11Sink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
constexpr uint8_t kPl = 5;
// Menu look picked in the wizard for these shots ("Tokyo").
constexpr uint8_t kMenuPalette = 3;
const char *t(TrKey key) { return TranslationsData::trKeyLookup(static_cast<uint8_t>(key), kPl); }
const char *t3(TrKey3 key) { return TranslationsData::trKey3Lookup(static_cast<uint8_t>(key), kPl); }
const char *t4(TrKey4 key) { return TranslationsData::trKey4Lookup(static_cast<uint8_t>(key), kPl); }
const char *ts(TrStatus key) { return TranslationsData::trStatusLookup(static_cast<uint8_t>(key), kPl); }
const char *uit(UiText key) { return TranslationsData::uiTextLookup(static_cast<uint8_t>(key), kPl); }
nano::WizardView base(size_t step) {
  nano::WizardView v;
  v.step = step;
  v.stepCount = 12;
  v.backId = 2;
  v.backLabel = uit(UiText::Back);
  v.nextId = 1;
  v.nextLabel = t3(TrKey3::NextLabel);
  return v;
}
std::vector<String> words(const char *sentence) {
  std::vector<String> out;
  String s = sentence;
  int start = 0;
  while (start < static_cast<int>(s.length())) {
    int space = s.indexOf(' ', start);
    if (space < 0) space = s.length();
    if (space > start) out.push_back(s.substring(start, space));
    start = space + 1;
  }
  return out;
}
String trimColon(const char *label) {
  String s = label;
  s.trim();
  if (s.endsWith(":")) s.remove(s.length() - 1);
  s.trim();
  return s;
}
nano::ListItem setting(int id, const String &label, const String &value) {
  nano::ListItem it;
  it.kind = nano::ListItem::Kind::Setting;
  it.id = id;
  it.label = label;
  it.value = value;
  it.helpId = id + 100;
  return it;
}
nano::ListItem slider(int id, const String &label, const String &value, int v, int lo, int hi) {
  nano::ListItem it;
  it.kind = nano::ListItem::Kind::Slider;
  it.id = id;
  it.label = label;
  it.value = value;
  it.sliderValue = v;
  it.sliderMin = lo;
  it.sliderMax = hi;
  it.helpId = id + 100;
  return it;
}
nano::ListItem toggle(int id, const String &label, bool on) {
  nano::ListItem it;
  it.kind = nano::ListItem::Kind::Toggle;
  it.id = id;
  it.label = label;
  it.on = on;
  it.helpId = id + 100;
  return it;
}
nano::ListItem button(int id, const String &label) {
  nano::ListItem it;
  it.kind = nano::ListItem::Kind::Button;
  it.id = id;
  it.label = label;
  return it;
}
}  // namespace

void runV0411Screens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  V11Sink sink;
  d.setNanoUiFont(4);
  d.setBatteryState(true, 76, false);
  d.setDarkMode(false);
  d.setNightMode(false);
  d.setFocusColorIndex(1);
  d.setNanoPalette(kMenuPalette, false);

  // ── Wizard: menu look, then the reading-look steps in reading colors ──
  auto reading = [&](bool on) {
    d.overrideNanoPalette(on ? DisplayManager::kNanoPaletteClassic : kMenuPalette, false);
  };

  {  // First page: no line about the Reboot button any more.
    nano::WizardView v = base(0);
    v.title = t3(TrKey3::WelcomeLanguageTitle);
    v.subtitle = t4(TrKey4::WizLanguageSub);
    v.backId = nano::kNoTarget;
    const char *langs[] = {"English", "Espa\xF1ol", "Fran\xE7""ais", "Deutsch", "Rom\xE2n\x9F", "Polski"};
    for (int i = 0; i < 6; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.label = langs[i];
      c.selected = i == 5;
      c.art = nano::WizardChipArt::Label;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "v11_01_kreator_jezyk");
  }

  {  // Menu look: the page wears the chosen menu palette.
    nano::WizardView v = base(3);
    v.title = t4(TrKey4::WizMenuThemeTitle);
    v.subtitle = t4(TrKey4::WizMenuThemeSub);
    v.chipRows = 2;
    v.chipColumns = 4;
    v.pageNextId = 5;
    for (uint8_t i = 0; i < 8; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.art = nano::WizardChipArt::Palette;
      c.palette = i;
      c.label = i == 0 ? String("Klasyczny") : String(DisplayManager::nanoPaletteName(i));
      c.selected = i == kMenuPalette;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    dump(d, "v11_02_kreator_wyglad_menu");
  }

  // Reading theme: the page turns into the tapped reading theme.
  const char *themeNames[] = {"Jasny", "Ciemny", "Nocny"};
  const uint8_t themeArt[] = {1, 0, 2};
  for (int pick = 0; pick < 3; ++pick) {
    d.setDarkMode(pick >= 1);
    d.setNightMode(pick == 2);
    reading(true);
    nano::WizardView v = base(4);
    v.title = t3(TrKey3::WelcomeThemeTitle);
    v.subtitle = t4(TrKey4::WizThemeSub);
    for (int i = 0; i < 3; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.label = themeNames[i];
      c.selected = i == pick;
      c.art = nano::WizardChipArt::Theme;
      c.theme = themeArt[i];
      c.word = t4(TrKey4::TutWord);
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    reading(false);
    const char *names[] = {"v11_03_kreator_motyw_jasny", "v11_03_kreator_motyw_ciemny", "v11_03_kreator_motyw_nocny"};
    dump(d, names[pick]);
  }
  // From here on: dark reading theme picked.
  d.setDarkMode(true);
  d.setNightMode(false);

  {  // Menu font: back in the menu look.
    nano::WizardView v = base(5);
    v.title = t4(TrKey4::WizMenuFontTitle);
    v.subtitle = t4(TrKey4::WizMenuFontSub);
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
    dump(d, "v11_04_kreator_czcionka_menu");
  }

  {  // Reading font: reading colors again.
    reading(true);
    nano::WizardView v = base(6);
    v.title = t4(TrKey4::WizReadFontTitle);
    v.subtitle = t4(TrKey4::WizFontSub);
    v.extraId = 3;
    v.extraLabel = t4(TrKey4::WizPreview);
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
      c.selected = i == 2;
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    reading(false);
    dump(d, "v11_05_kreator_czcionka_czytania");
  }

  {  // Reading mode.
    reading(true);
    nano::WizardView v = base(7);
    v.title = t3(TrKey3::WelcomeReadingModeTitle);
    v.subtitle = t4(TrKey4::WizModeSub);
    v.extraId = 3;
    v.extraLabel = t4(TrKey4::WizPreview);
    const char *modes[] = {"RSVP", "Przewijanie strony"};
    for (int i = 0; i < 2; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.label = modes[i];
      c.selected = i == 0;
      c.art = i == 0 ? nano::WizardChipArt::Rsvp : nano::WizardChipArt::Scroll;
      c.word = t4(TrKey4::TutWord);
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    reading(false);
    dump(d, "v11_06_kreator_sposob_czytania");
  }

  {  // Letter color: the accent follows the tapped color.
    d.setFocusColorIndex(4);
    reading(true);
    nano::WizardView v = base(8);
    v.title = t3(TrKey3::WelcomeHighlightColorTitle);
    v.subtitle = t4(TrKey4::WizColorSub);
    const TrKey3 colors[] = {TrKey3::ColorRed, TrKey3::ColorBlue, TrKey3::ColorGreen,
                             TrKey3::ColorYellow, TrKey3::ColorOrange, TrKey3::ColorPurple};
    v.chipRows = 2;
    v.chipColumns = 3;
    for (int i = 0; i < 6; ++i) {
      nano::WizardChip c;
      c.id = 100 + i;
      c.label = t3(colors[i]);
      c.selected = i == 4;
      c.art = nano::WizardChipArt::Swatch;
      c.swatch = DisplayManager::presetFocusColor(static_cast<uint8_t>(i));
      v.chips.push_back(c);
    }
    nano::paintWizard(d, sink, v);
    reading(false);
    dump(d, "v11_07_kreator_kolor_litery");
  }

  {  // App download: menu look again.
    nano::WizardView v = base(9);
    v.body = nano::WizardBody::Qr;
    v.title = t3(TrKey3::WelcomeConnectTitle);
    v.subtitle = t3(TrKey3::WelcomeConnectLine1);
    v.qrLine = "flower.theworkpc.com/appdownload";
    v.qrHint = t4(TrKey4::WizConnectLook);
    v.nextId = nano::kNoTarget;
    nano::paintWizard(d, sink, v);
    dump(d, "v11_08_kreator_aplikacja");
  }

  // ── Reading previews: the reading screen itself, Medium size ──
  DisplayManager::ReaderChrome chrome;
  chrome.showBattery = false;
  chrome.showChapter = false;
  chrome.showProgress = false;
  chrome.showPreviousSentenceHint = false;
  chrome.showSavePointButton = false;
  {
    const std::vector<String> w = words(t4(TrKey4::WizRsvpSample));
    for (size_t pick : {size_t(2), size_t(6)}) {
      d.renderPhantomRsvpWord(w[pick - 1], w[pick], w[pick + 1], 1, "", 0, false, "", chrome);
      dump(d, pick == 2 ? "v11_09_podglad_rsvp_a" : "v11_09_podglad_rsvp_b");
    }
  }
  {
    std::vector<DisplayManager::ContextWord> page;
    const std::vector<String> w = words(t3(TrKey3::WelcomePreviewScrollBody));
    for (int paragraph = 0; paragraph < 4; ++paragraph) {
      for (size_t i = 0; i < w.size(); ++i) {
        DisplayManager::ContextWord cw;
        cw.text = w[i];
        cw.paragraphStart = i == 0;
        page.push_back(cw);
      }
    }
    const size_t current = 9;
    page[current].current = true;
    d.setScrollFontSize(4);
    d.setScrollLineSpacing(1);
    d.setScrollMargin(1);
    d.setScrollAlign(DisplayManager::kScrollAlignJustify);
    d.renderScrollView(page, 0x50524556UL, 0, current, 400, "", 0, "", "", chrome);
    dump(d, "v11_10_podglad_przewijania");
  }

  // ── Power-off animation ──
  for (int frame = 0; frame < 3; ++frame) {
    nano::PowerOffView v;
    v.title = t(TrKey::PoweringOff);
    v.detail = TranslationsData::trKey2Lookup(static_cast<uint8_t>(TrKey2::HoldPwrToStart), kPl);
    const uint16_t progress[] = {150, 550, 1000};
    v.progress = progress[frame];
    d.nanoBeginFrame();
    nano::paintPowerOff(d, v);
    const char *names[] = {"v11_11_wylaczanie_a", "v11_11_wylaczanie_b", "v11_11_wylaczanie_c"};
    dump(d, names[frame]);
  }

  {  // Screensaver question (the dimming is applied to the picture later).
    nano::ConfirmView v;
    v.question = t4(TrKey4::SaverAskTitle);
    v.detail = String(t4(TrKey4::SaverAskCountdown)) + " 7 s";
    nano::ConfirmView::Action no;
    no.id = 1;
    no.label = t4(TrKey4::SaverAskNo);
    v.actions.push_back(no);
    nano::ConfirmView::Action yes;
    yes.id = 2;
    yes.label = t4(TrKey4::SaverAskYes);
    v.actions.push_back(yes);
    d.setModernCardStyle(true);
    d.nanoBeginFrame();
    nano::paintConfirm(d, sink, v);
    dump(d, "v11_12_wygaszacz_pytanie");
  }

  {  // Settings > Screensaver.
    nano::ListView v;
    v.header.backId = 0;
    v.header.title = trimColon(t(TrKey::Screensaver));
    v.fullScreen = true;
    v.columns = 2;
    v.rows = 3;
    v.items.push_back(setting(1, trimColon(t(TrKey::ScreensaverStyle)), t(TrKey::SaverWords)));
    v.items.push_back(slider(2, trimColon(t(TrKey::ScreensaverTimeout)), "5 min", 3, 0, 8));
    v.items.push_back(slider(3, trimColon(t(TrKey::ScreensaverAutoOff)), "30 s", 1, 0, 7));
    v.items.push_back(slider(4, trimColon(t(TrKey::ScreensaverSleepGuard)), t(TrKey::No), 0, 0, 7));
    v.items.push_back(button(5, t(TrKey::ScreensaverPreview)));
    d.nanoBeginFrame();
    nano::paintList(d, sink, v);
    dump(d, "v11_13_wygaszacz_ustawienia");

    nano::HelpView h;
    h.header.backId = 0;
    h.header.title = HelpData::title(HelpTopic::SaverAutoOff, kPl);
    h.lines = DisplayManager::nanoWrapText(HelpData::body(HelpTopic::SaverAutoOff, kPl), nano::helpTextWidth(), 2);
    d.nanoBeginFrame();
    nano::paintHelp(d, sink, h);
    dump(d, "v11_14_wylacz_po_pomoc");
  }

  {  // Settings > Reading (RSVP): the new delays, two pages.
    for (int page = 0; page < 2; ++page) {
      nano::ListView v;
      v.header.backId = 0;
      v.header.title = "Czytanie";
      v.fullScreen = true;
      v.columns = 2;
      v.rows = 3;
      if (page == 0) {
        v.items.push_back(setting(1, uit(UiText::ReadingMode), "RSVP"));
        v.items.push_back(setting(2, trimColon(t(TrKey::PauseBehaviour)), "Koniec zdania"));
        v.items.push_back(slider(3, trimColon(t(TrKey::BaseSpeed)), "300 WPM", 300, 50, 1000));
        v.items.push_back(slider(4, uit(UiText::LongWords), "100 ms", 100, 0, 600));
        v.items.push_back(slider(5, uit(UiText::Complexity), "100 ms", 100, 0, 600));
        v.items.push_back(slider(6, uit(UiText::Punctuation), "100 ms", 100, 0, 600));
      } else {
        v.items.push_back(slider(7, t4(TrKey4::PacingSentences), "150 ms", 150, 0, 600));
        v.items.push_back(button(8, uit(UiText::ResetPacing)));
      }
      d.nanoBeginFrame();
      nano::paintList(d, sink, v);
      dump(d, page == 0 ? "v11_15_tempo_a" : "v11_15_tempo_b");
    }
  }

  // Wyglad czytania > Prowadnica in three menu fonts: nothing cut short.
  for (uint8_t font : {uint8_t(4), uint8_t(0), uint8_t(2)}) {
    d.setNanoUiFont(font);
    nano::TypographyView v;
    v.backId = 200;
    v.section = 2;
    const String segs[] = {"Kolory", "Tekst", "Prowadnica"};
    for (int i = 0; i < nano::TypographyView::kSections; ++i) {
      v.segmentIds[i] = 201 + i;
      v.segmentLabels[i] = segs[i];
    }
    const std::vector<String> w = words(t4(TrKey4::WizRsvpSample));
    v.before = w[1];
    v.word = w[2];
    v.after = w[3];
    v.fontSizeLevel = 1;
    nano::ListItem side;
    side.kind = nano::ListItem::Kind::Setting;
    side.id = 234;
    side.label = t4(TrKey4::GuideSideShort);
    side.value = t(TrKey::LeftHand);
    v.items.push_back(side);
    nano::ListItem pos = slider(230, t4(TrKey4::TypoPosition), "35%", 35, 30, 40);
    pos.helpId = nano::kNoTarget;
    v.items.push_back(pos);
    nano::ListItem len = slider(231, t4(TrKey4::TypoLineLength), "20 px", 20, 12, 30);
    len.helpId = nano::kNoTarget;
    v.items.push_back(len);
    nano::ListItem gap = slider(232, t4(TrKey4::TypoLineGap), "0 px", 0, 0, 8);
    gap.helpId = nano::kNoTarget;
    v.items.push_back(gap);
    nano::ListItem reset = button(233, t4(TrKey4::TypoDefaults));
    reset.icon = nano::Icon::Restart;
    v.items.push_back(reset);
    d.setDarkMode(true);
    d.nanoBeginFrame();
    nano::paintTypography(d, sink, v);
    const String name = String("v11_16_prowadnica_font") + String(static_cast<int>(font));
    dump(d, name.c_str());
  }
  d.setNanoUiFont(4);

  {  // Settings > Display: no guide row; the battery look with its badge.
    for (int frame = 0; frame < 3; ++frame) {
      d.setBatteryStyle(frame == 0 ? 0 : 1);
      nano::ListView v;
      v.header.backId = 0;
      v.header.title = "Ekran";
      v.fullScreen = true;
      v.columns = 2;
      v.rows = 3;
      v.items.push_back(slider(1, uit(UiText::Brightness), "70%", 70, 10, 100));
      v.items.push_back(setting(3, trimColon(t4(TrKey4::BatteryStyleColon)),
                                frame == 0 ? t4(TrKey4::BatteryStyleIconPercent) : t4(TrKey4::BatteryStyleNumberInIcon)));
      v.items.push_back(setting(4, trimColon(t(TrKey::FooterLabel)), "Czas do ko\x9D""ca"));
      v.items.push_back(setting(5, trimColon(t(TrKey::BatteryLabel)), "Procent"));
      v.items.push_back(setting(6, trimColon(t(TrKey::Screensaver)), t(TrKey::SaverWords)));
      v.items.push_back(toggle(7, "Bateria w czytaniu", false));
      d.nanoBeginFrame();
      nano::paintList(d, sink, v);
      const uint8_t alpha[] = {0, 130, 255};
      nano::paintBatteryPreview(d, alpha[frame]);
      const char *names[] = {"v11_17_ekran_ustawienia", "v11_18_bateria_podglad_wejscie", "v11_18_bateria_podglad_pelny"};
      dump(d, names[frame]);
    }
    d.setBatteryStyle(0);
  }

  {  // A new, unformatted card.
    nano::WizardView v = base(0);
    v.stepCount = 0;
    v.body = nano::WizardBody::Message;
    v.backId = nano::kNoTarget;
    v.title = t4(TrKey4::WizSdFormatTitle);
    v.subtitle = t4(TrKey4::WizSdFormatSub);
    v.nextLabel = t4(TrKey4::WizSdFormat);
    v.extraId = 3;
    v.extraLabel = t4(TrKey4::WizSdSkip);
    nano::paintWizard(d, sink, v);
    dump(d, "v11_19_karta_niesformatowana");
    v.title = t4(TrKey4::SdSetupAssetsTitle);
    v.subtitle = t4(TrKey4::SdSetupAssetsSub);
    v.nextLabel = t4(TrKey4::SdSetupDownload);
    v.extraLabel = t4(TrKey4::SdSetupDecline);
    nano::paintWizard(d, sink, v);
    dump(d, "v11_20_karta_pobieranie");
  }
}
