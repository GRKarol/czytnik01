// Settings > Reading in scroll mode (2026-10-11): the scroll page got its own
// delays, built the way App builds the list, with the Polish tables.
#include <vector>

#include "app/Translations.h"
#include "app/generated/HelpData.h"
#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct V12Sink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
constexpr uint8_t kPl = 5;
const char *t(TrKey key) { return TranslationsData::trKeyLookup(static_cast<uint8_t>(key), kPl); }
const char *t4(TrKey4 key) { return TranslationsData::trKey4Lookup(static_cast<uint8_t>(key), kPl); }
const char *ts(TrStatus key) { return TranslationsData::trStatusLookup(static_cast<uint8_t>(key), kPl); }
const char *uit(UiText key) { return TranslationsData::uiTextLookup(static_cast<uint8_t>(key), kPl); }
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
nano::ListItem button(int id, const String &label) {
  nano::ListItem it;
  it.kind = nano::ListItem::Kind::Button;
  it.id = id;
  it.label = label;
  it.helpId = id + 100;
  return it;
}
}  // namespace

void runV0412Screens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  V12Sink sink;
  d.setNanoUiFont(4);
  d.setDarkMode(true);
  d.setModernCardStyle(true);

  const char *names[] = {"v12_01_przewijanie_uklad", "v12_02_przewijanie_tempo",
                         "v12_03_przewijanie_podglad"};
  for (int page = 0; page < 3; ++page) {
    nano::ListView v;
    v.header.backId = 0;
    v.header.title = "Czytanie";
    v.fullScreen = true;
    v.columns = 2;
    v.rows = 3;
    if (page == 0) {
      v.items.push_back(setting(1, uit(UiText::ReadingMode), uit(UiText::ScrollMode)));
      v.items.push_back(slider(2, uit(UiText::FontSize), "4", 4, 0, 8));
      v.items.push_back(slider(3, uit(UiText::ScrollLineSpacing), ts(TrStatus::SpacingNormal), 1, 0, 2));
      v.items.push_back(slider(4, uit(UiText::ScrollMargins), ts(TrStatus::SpacingNormal), 1, 0, 2));
      v.items.push_back(setting(5, trimColon(t4(TrKey4::ScrollAlignColon)), t4(TrKey4::AlignJustify)));
      v.items.push_back(setting(6, trimColon(t(TrKey::PauseBehaviour)), t(TrKey::Sentence)));
    } else if (page == 1) {
      v.items.push_back(slider(7, trimColon(t(TrKey::BaseSpeed)), "300 WPM", 300, 50, 1000));
      v.items.push_back(slider(8, uit(UiText::LongWords), "100 ms", 100, 0, 600));
      v.items.push_back(slider(9, uit(UiText::Complexity), "100 ms", 100, 0, 600));
      v.items.push_back(slider(10, uit(UiText::Punctuation), "100 ms", 100, 0, 600));
      v.items.push_back(slider(11, t4(TrKey4::PacingSentences), "150 ms", 150, 0, 600));
      v.items.push_back(button(12, uit(UiText::ResetPacing)));
    } else {
      v.items.push_back(button(13, ts(TrStatus::Preview)));
    }
    d.nanoBeginFrame();
    nano::paintList(d, sink, v);
    dump(d, names[page]);
  }

  const HelpTopic topics[] = {HelpTopic::ReadingMode, HelpTopic::ResetPacing};
  const char *helpNames[] = {"v12_04_pomoc_tryb", "v12_05_pomoc_reset"};
  for (int i = 0; i < 2; ++i) {
    nano::HelpView h;
    h.header.backId = 0;
    h.header.title = HelpData::title(topics[i], kPl);
    h.lines = DisplayManager::nanoWrapText(HelpData::body(topics[i], kPl), nano::helpTextWidth(), 2);
    d.nanoBeginFrame();
    nano::paintHelp(d, sink, h);
    dump(d, helpNames[i]);
  }
}
