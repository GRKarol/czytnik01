// List 8 (2026-10-12): Polish letters in the flash fonts (Atkinson and
// OpenDyslexic now carry the whole 1-255 slot map), the scroll page on its
// new defaults (size 2, relaxed lines) and the Wi-Fi-with-screen-off row.
#include <vector>

#include "app/Translations.h"
#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct V13Sink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
constexpr uint8_t kPl = 5;
const char *t(TrKey key) { return TranslationsData::trKeyLookup(static_cast<uint8_t>(key), kPl); }
const char *t3(TrKey3 key) { return TranslationsData::trKey3Lookup(static_cast<uint8_t>(key), kPl); }
const char *t4(TrKey4 key) { return TranslationsData::trKey4Lookup(static_cast<uint8_t>(key), kPl); }
String trimColon(const char *label) {
  String s = label;
  s.trim();
  if (s.endsWith(":")) s.remove(s.length() - 1);
  s.trim();
  return s;
}
std::vector<String> splitWords(const char *sentence) {
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
nano::ListItem setting(int id, const String &label, const String &value) {
  nano::ListItem it;
  it.kind = nano::ListItem::Kind::Setting;
  it.id = id;
  it.label = label;
  it.value = value;
  return it;
}
}  // namespace

void runV0413Screens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  V13Sink sink;
  d.setDarkMode(true);
  d.setModernCardStyle(true);
  DisplayManager::ReaderChrome chrome;
  chrome.showBattery = false;
  chrome.showChapter = false;
  chrome.showProgress = false;
  chrome.showPreviousSentenceHint = false;
  chrome.showSavePointButton = false;

  // ── RSVP, Medium: Atkinson and OpenDyslexic with Polish letters ──
  const struct {
    DisplayManager::ReaderTypeface face;
    const char *name;
  } faces[] = {
      {DisplayManager::ReaderTypeface::AtkinsonHyperlegible, "v13_01_atkinson"},
      {DisplayManager::ReaderTypeface::OpenDyslexic, "v13_02_opendyslexic"},
  };
  for (const auto &face : faces) {
    DisplayManager::TypographyConfig config;
    config.typeface = face.face;
    d.setTypographyConfig(config);
    d.renderPhantomRsvpWord("o", "tysi\x97""cu", "twarzy", 1, "", 0, false, "", chrome);
    dump(d, (String(face.name) + "_rsvp").c_str());
    d.renderPhantomRsvpWord("\xB4""\xF3""\x83""\x9B", "g\x99""\x9F""l\x97", "ja\x9D""", 1, "", 0, false, "", chrome);
    dump(d, (String(face.name) + "_rsvp_znaki").c_str());
  }

  // ── Scroll page in Atkinson on the new defaults: size 2, relaxed ──
  {
    DisplayManager::TypographyConfig config;
    config.typeface = DisplayManager::ReaderTypeface::AtkinsonHyperlegible;
    d.setTypographyConfig(config);
    std::vector<DisplayManager::ContextWord> page;
    const std::vector<String> w = splitWords(t3(TrKey3::WelcomePreviewScrollBody));
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
    d.setScrollFontSize(2);
    d.setScrollLineSpacing(2);
    d.setScrollMargin(1);
    d.setScrollAlign(DisplayManager::kScrollAlignJustify);
    d.renderScrollView(page, 0x56313300UL, 0, current, 400, "", 0, "", "", chrome);
    dump(d, "v13_03_przewijanie_domyslne");
  }

  // ── Device > Connections in advanced mode: the new row last ──
  {
    d.setNanoUiFont(4);
    nano::ListView v;
    v.header.backId = 0;
    v.header.title = t(TrKey::Connectivity);
    v.fullScreen = true;
    v.columns = 2;
    v.rows = 3;
    v.items.push_back(setting(1, "Wi-Fi", "Dom"));
    v.items.push_back(setting(2, "Bluetooth", t(TrKey::Yes)));
    v.items.push_back(setting(3, trimColon(t(TrKey::PhoneSync)), t(TrKey::No)));
    v.items.push_back(setting(4, "Kopiuj przez USB", ""));
    v.items.push_back(setting(5, trimColon(t4(TrKey4::WifiSessionColon)), "Ca\x83""a sesja"));
    v.items.push_back(setting(6, trimColon(t4(TrKey4::WifiScreenOffColon)), t(TrKey::Yes)));
    d.nanoBeginFrame();
    nano::paintList(d, sink, v);
    d.nanoEndFrame();
    dump(d, "v13_04_polaczenia_wifi_zgaszony");
  }
}
