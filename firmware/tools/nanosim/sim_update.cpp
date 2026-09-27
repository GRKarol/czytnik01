// Screens of the 2026-09-27 update (go to, bookmark name choice, battery
// styles, scroll-mode reader panel, ...).
#include <string>
#include <vector>

#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

using namespace nano;

namespace {

struct SimSink : Sink {
  std::vector<Rect> rects;
  int pressedId = -999;
  void target(const Rect &rect, int id) override {
    (void)id;
    rects.push_back(rect);
  }
  bool pressed(int id) const override { return id == pressedId; }
};

String pl(const char *s) {
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

}  // namespace

void runUpdateScreens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  SimSink sink;
  auto frame = [&](const std::string &name, auto paint) {
    sink.rects.clear();
    d.nanoBeginFrame();
    paint();
    d.nanoEndFrame();
    dump(d, name.c_str());
    for (const Rect &r : sink.rects) d.nanoDrawRect(r.x, r.y, r.w, r.h, 0xF81F);
    dump(d, (name + "_targets").c_str());
  };
  auto panel = [&](bool scroll) {
    ReaderPanelView v;
    v.chapter = pl("Ksi~ega pierwsza: Gospodarstwo");
    v.progressLabel = "42%";
    v.timeLeft = "3 h 12 min";
    v.progressPercent = 42;
    v.before = pl("kt~ory");
    v.word = pl("przeczyta~l");
    v.after = pl("ksi~a~zk~e");
    v.scrollMode = scroll;
    const char *text[] = {"Litwo!", "Ojczyzno", "moja!", "ty", "jeste~s", "jak", "zdrowie.", "Ile", "ci~e",
                          "trzeba", "ceni~c,", "ten", "tylko", "si~e", "dowie,", "kto", "ci~e", "straci~l.",
                          "Dzi~s", "pi~eknos~c", "tw~a", "w", "ca~lej", "ozdobie", "widz~e", "i", "opisuj~e,",
                          "bo", "t~eskni~e", "po", "tobie.", "Panno", "~swi~eta,", "co", "Jasnej", "bronisz",
                          "Cz~estochowy", "i", "w", "Ostrej", "~swiecisz", "Bramie!"};
    for (size_t i = 0; i < sizeof(text) / sizeof(text[0]); ++i) {
      DisplayManager::ContextWord w;
      w.text = pl(text[i]);
      w.paragraphStart = i == 0;
      v.words.push_back(w);
    }
    v.currentLocal = 17;
    v.menuId = 180;
    v.chaptersId = 181;
    v.gotoId = 187;
    v.statusId = 187;
    v.rewindId = 188;
    v.bookmarkId = 182;
    v.minusId = 183;
    v.wpmId = 184;
    v.plusId = 185;
    v.startId = 186;
    v.menuLabel = "Menu";
    v.wpmLabel = "350 WPM";
    v.startLabel = "Czytaj";
    v.hint = scroll ? String() : pl("Przytrzymaj, by czyta~c  -  przesu~n w bok, by przewin~a~c");
    paintReaderPanel(d, sink, v);
  };
  frame("u_panel_rsvp", [&] { panel(false); });
  frame("u_panel_scroll", [&] { panel(true); });
  d.setDarkMode(false);
  frame("u_panel_scroll_light", [&] { panel(true); });
  d.setDarkMode(true);

  for (int segment = 0; segment < 3; ++segment) {
    frame("u_goto" + std::to_string(segment), [&] {
      GoToView v;
      v.header.backId = 0;
      v.header.title = pl("Przejd~x do");
      v.segmentIds[0] = 1;
      v.segmentIds[1] = 2;
      v.segmentIds[2] = 3;
      v.segmentLabels[0] = "Procent";
      v.segmentLabels[1] = "Strona";
      v.segmentLabels[2] = pl("Rozdzia~l");
      v.segment = segment;
      v.detail = pl("Ksi~ega pi~ata: Rada");
      if (segment == 0) {
        v.value = "42%";
        v.hint = "Strona 120 z 286";
        v.sliderMax = 100;
        v.sliderValue = 42;
      } else if (segment == 1) {
        v.value = "120 / 286";
        v.hint = pl("1 strona = 250 s~l~ow  -  42%");
        v.sliderMin = 1;
        v.sliderMax = 286;
        v.sliderValue = 120;
      } else {
        v.value = "5 / 12";
        v.hint = "Strona 120 z 286  -  42%";
        v.sliderMax = 11;
        v.sliderValue = 4;
      }
      v.minusId = 4;
      v.plusId = 5;
      v.readId = 6;
      v.readLabel = pl("Czytaj st~ad");
      paintGoTo(d, sink, v);
    });
  }

  frame("u_bookmark_choice", [&] {
    ChoiceView v;
    v.header.backId = 0;
    v.header.title = pl("Nazwij zak~ladk~e");
    v.question = pl("Jak nazwa~c zak~ladk~e?");
    ChoiceView::Option a;
    a.id = 1;
    a.label = pl("Domy~slna nazwa");
    a.detail = pl("42.3% Ksi~ega pierwsza");
    a.icon = Icon::Bookmark;
    a.accent = true;
    ChoiceView::Option b;
    b.id = 2;
    b.label = pl("W~lasna nazwa");
    b.detail = "Wpisz na klawiaturze";
    b.icon = Icon::Edit;
    v.options = {a, b};
    paintChoice(d, sink, v);
  });

  // Battery indicator styles, charging and not, in the rail footer size.
  frame("u_battery", [&] {
    for (int style = 0; style < 4; ++style) {
      d.setBatteryStyle(static_cast<uint8_t>(style));
      d.setBatteryState(true, 76, false);
      d.nanoBatteryInline(Rect(20, 10 + style * 38, 140, 22));
      d.setBatteryState(true, 58, true);
      d.nanoBatteryInline(Rect(200, 10 + style * 38, 140, 22));
      d.setBatteryState(true, 12, false);
      d.nanoBatteryInline(Rect(380, 10 + style * 38, 140, 22));
      d.nanoBatteryInline(Rect(540, 10 + style * 38, 60, 22), true);
    }
    d.setBatteryStyle(0);
    d.setBatteryState(true, 76, false);
  });
}
