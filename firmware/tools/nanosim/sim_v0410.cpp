// Screens changed in v0.4.10 (list 6, 2026-10-10), built the way App builds
// them, with the real Polish translation tables.
#include <vector>

#include "app/Translations.h"
#include "app/generated/HelpData.h"
#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct V10Sink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
constexpr uint8_t kPl = 5;
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
}  // namespace

void runV0410Screens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  V10Sink sink;
  d.setNanoUiFont(4);

  {  // Wi-Fi password: visible as typed, "->" at the right end of the field.
    constexpr int kW = BoardConfig::DISPLAY_WIDTH, kMargin = 8, kTop = 48, kGap = 4, kRowH = 27, kConfirmW = 72;
    const char *rows[] = {"qwertyuiop", "asdfghjkl", "zxcvbnm"};
    std::vector<DisplayManager::Button> buttons;
    const int pitch = kRowH + kGap;
    for (int r = 0; r < 3; ++r) {
      const int n = strlen(rows[r]);
      const int keyW = std::max(28, (kW - 2 * kMargin - (n - 1) * kGap) / n);
      int x = std::max(0, (kW - (keyW * n + (n - 1) * kGap)) / 2);
      for (int i = 0; i < n; ++i) {
        DisplayManager::Button b;
        b.label = String(rows[r][i]);
        b.x = x;
        b.y = kTop + r * pitch;
        b.width = keyW;
        b.height = kRowH;
        buttons.push_back(b);
        x += keyW + kGap;
      }
    }
    struct Ctl {
      String label;
      int units;
      bool active;
    };
    const Ctl ctls[] = {{"abc", 11, true}, {"ABC", 11, false}, {"123", 11, false}, {ts(TrStatus::KeySpace), 24, false},
                        {ts(TrStatus::KeyBack), 13, false}, {ts(TrStatus::KeyHide), 13, false},
                        {ts(TrStatus::KeyCancel), 14, false}};
    int units = 0;
    for (const Ctl &c : ctls) units += c.units;
    const int count = sizeof(ctls) / sizeof(ctls[0]);
    const int avail = kW - 2 * kMargin - (count - 1) * kGap;
    int remaining = avail, x = kMargin;
    for (int i = 0; i < count; ++i) {
      int w = i + 1 < count ? avail * ctls[i].units / units : remaining;
      if (i + 1 < count) remaining -= w;
      DisplayManager::Button b;
      b.label = ctls[i].label;
      b.x = x;
      b.y = kTop + 3 * pitch;
      b.width = std::max(28, w);
      b.height = kRowH;
      b.active = ctls[i].active;
      buttons.push_back(b);
      x += b.width + kGap;
    }
    DisplayManager::Button ok;
    ok.label = "->";
    ok.width = kConfirmW;
    ok.height = kTop - 8;
    ok.x = kW - kMargin - kConfirmW;
    ok.y = 3;
    ok.accent = true;
    buttons.push_back(ok);
    d.renderTextEntry("Dom_Karola", ts(TrStatus::Password), "kwiatek2024", "", buttons);
    dump(d, "v10_01_wifi_haslo");
  }

  {  // One page instead of two after the downloads.
    nano::WizardView v = base(2);
    v.body = nano::WizardBody::Message;
    v.title = t3(TrKey3::WelcomeSuperTitle);
    v.subtitle = t3(TrKey3::WelcomeConfigureTitle);
    v.backId = nano::kNoTarget;
    v.autoPercent = 60;
    nano::paintWizard(d, sink, v);
    dump(d, "v10_02_super");
  }

  {  // A card the flasher didn't prepare.
    nano::WizardView v = base(0);
    v.body = nano::WizardBody::Message;
    v.title = t4(TrKey4::WizSdOwnTitle);
    v.subtitle = t4(TrKey4::WizSdOwnSub);
    v.nextLabel = t4(TrKey4::WizSdFormat);
    v.extraId = 3;
    v.extraLabel = t4(TrKey4::WizSdKeepFiles);
    nano::paintWizard(d, sink, v);
    dump(d, "v10_03_wlasna_karta");
  }

  {  // Font download on an own card: the 10 minute note.
    nano::WizardView v = base(2);
    v.body = nano::WizardBody::Loading;
    v.backId = -1;
    v.nextId = -1;
    v.title = t3(TrKey3::WelcomeLoadingPhrase3);
    v.subtitle = String(t4(TrKey4::WizTipLabel)) + " " + t4(TrKey4::WizTip2);
    v.status = String(t4(TrKey4::WizFontsProgress)) + "  7/34";
    v.footer = t4(TrKey4::WizFontsLongNote);
    v.percent = 21;
    nano::paintWizard(d, sink, v);
    dump(d, "v10_04_pobieranie_czcionek");
  }

  {  // RSVP preview in the menu language.
    const std::vector<String> w = words(t4(TrKey4::WizRsvpSample));
    for (size_t pick : {size_t(2), size_t(9)}) {
      nano::WizardView v = base(7);
      v.body = nano::WizardBody::Preview;
      v.nextLabel = "Wybieram";
      v.previewMode = 0;
      v.previewBefore = w[pick - 1];
      v.previewWord = w[pick];
      v.previewAfter = w[pick + 1];
      v.previewSizeLevel = 1;
      nano::paintWizard(d, sink, v);
      dump(d, pick == 2 ? "v10_05_podglad_rsvp_a" : "v10_05_podglad_rsvp_b");
    }
  }

  {  // Screensaver question, 10 s countdown.
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
    nano::paintConfirm(d, sink, v);
    dump(d, "v10_06_wygaszacz_pytanie");
  }

  {  // Settings > Display with the guide row.
    nano::ListView v;
    v.header.backId = 0;
    v.header.title = "Ekran";
    v.fullScreen = true;
    v.columns = 2;
    v.rows = 3;
    nano::ListItem bright;
    bright.kind = nano::ListItem::Kind::Slider;
    bright.id = 1;
    bright.label = uit(UiText::Brightness);
    bright.value = "70%";
    bright.sliderValue = 70;
    bright.sliderMin = 20;
    bright.sliderMax = 100;
    v.items.push_back(bright);
    auto setting = [&](int id, const String &label, const String &value) {
      nano::ListItem it;
      it.kind = nano::ListItem::Kind::Setting;
      it.id = id;
      it.label = label;
      it.value = value;
      it.helpId = id + 100;
      v.items.push_back(it);
    };
    setting(2, trimColon(t4(TrKey4::GuideSideColon)), t(TrKey::LeftHand));
    setting(3, trimColon(t4(TrKey4::BatteryStyleColon)), "Ikona");
    setting(4, trimColon(t(TrKey::FooterLabel)), "Czas do ko\x9D""ca");
    setting(5, trimColon(t(TrKey::BatteryLabel)), "Procent");
    setting(6, trimColon(t(TrKey::Screensaver)), "Ekran ga\x9F""nie >");
    nano::paintList(d, sink, v);
    dump(d, "v10_07_ustawienia_ekran");

    nano::HelpView h;
    h.header.backId = 0;
    h.header.title = HelpData::title(HelpTopic::Handedness, kPl);
    h.lines = DisplayManager::nanoWrapText(HelpData::body(HelpTopic::Handedness, kPl), nano::helpTextWidth(), 2);
    nano::paintHelp(d, sink, h);
    dump(d, "v10_08_prowadnica_pomoc");
  }

  {  // Settings > Reading (scroll) with alignment.
    nano::ListView v;
    v.header.backId = 0;
    v.header.title = "Czytanie";
    v.fullScreen = true;
    v.columns = 2;
    v.rows = 3;
    auto setting = [&](int id, const String &label, const String &value) {
      nano::ListItem it;
      it.kind = nano::ListItem::Kind::Setting;
      it.id = id;
      it.label = label;
      it.value = value;
      v.items.push_back(it);
    };
    setting(1, uit(UiText::ReadingMode), "Przewijanie");
    nano::ListItem size;
    size.kind = nano::ListItem::Kind::Slider;
    size.id = 2;
    size.label = uit(UiText::FontSize);
    size.value = "3";
    size.sliderValue = 3;
    size.sliderMin = 0;
    size.sliderMax = 8;
    v.items.push_back(size);
    setting(3, uit(UiText::ScrollLineSpacing), ts(TrStatus::SpacingNormal));
    setting(4, uit(UiText::ScrollMargins), "1");
    setting(5, trimColon(t4(TrKey4::ScrollAlignColon)), t4(TrKey4::AlignJustify));
    nano::ListItem prev;
    prev.kind = nano::ListItem::Kind::Button;
    prev.id = 6;
    prev.label = ts(TrStatus::Preview);
    v.items.push_back(prev);
    nano::paintList(d, sink, v);
    dump(d, "v10_09_przewijanie_ustawienia");
  }

  {  // The scroll page in each alignment.
    static const char *kText[] = {
        "Ka~zdy", "dzie~n", "zaczyna~l", "si~e", "tak", "samo.", "Wstawa~l", "przed", "~switem,", "parzy~l",
        "kaw~e", "i", "siada~l", "przy", "oknie", "z", "ksi~a~zk~a,", "kt~orej", "nie", "zd~a~zy~l",
        "sko~nczy~c", "poprzedniego", "wieczoru.", "Dopiero", "gdy", "s~lo~nce", "wsta~lo", "nad", "dachami,",
        "odk~lada~l", "j~a", "na", "parapet", "i", "szed~l", "do", "pracy."};
    auto encode = [](const char *s) {
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
            default: out += *p;
          }
        } else {
          out += *p;
        }
      }
      return out;
    };
    std::vector<DisplayManager::ContextWord> page;
    for (size_t i = 0; i < sizeof(kText) / sizeof(kText[0]); ++i) {
      DisplayManager::ContextWord w;
      w.text = encode(kText[i]);
      w.paragraphStart = i == 0 || i == 23;
      w.current = i == 12;
      page.push_back(w);
    }
    DisplayManager::ReaderChrome chrome;
    chrome.showBattery = false;
    chrome.showChapter = false;
    chrome.showProgress = false;
    chrome.showPreviousSentenceHint = false;
    chrome.showSavePointButton = false;
    d.setScrollFontSize(1);
    d.setScrollLineSpacing(1);
    d.setScrollMargin(1);
    const char *names[] = {"v10_10_strona_do_lewej", "v10_11_strona_wyjustowana", "v10_12_strona_do_srodka"};
    for (uint8_t align = 0; align < 3; ++align) {
      d.setScrollAlign(align);
      d.renderScrollView(page, 91000 + align, 0, 12, 0, "", 0, "", "", chrome);
      dump(d, names[align]);
    }
    d.setScrollAlign(0);
  }

  // What the reader shows while the flasher writes the card.
  d.renderStatus(t4(TrKey4::ProvTitle), String(t4(TrKey4::ProvWritingFonts)) + "  12", "literata-v2.fnt");
  dump(d, "v10_13_flasher_wgrywa");
}
