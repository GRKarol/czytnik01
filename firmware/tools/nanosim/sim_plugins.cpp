// Plugin screens (dictaphone, focus timer) exactly as the plugin bridge
// builds them (src/plugins/DeviceServicesBridge.cpp), in the Nano skin.
#include <vector>

#include "display/DisplayManager.h"

using Button = DisplayManager::Button;

void runPluginScreens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  const int W = BoardConfig::DISPLAY_WIDTH;
  const int H = BoardConfig::DISPLAY_HEIGHT;
  // Button pair = DeviceServicesBridge's bridgeRenderButtonPair: header row
  // with the plugin name and a back button that leaves the plugin.
  auto pair = [&](const char *title, const char *left, ui::IconId leftIcon, bool leftActive, const char *right,
                  ui::IconId rightIcon) {
    const int top = 36 + 4;
    std::vector<Button> b(3);
    b[0].icon = ui::IconId::Back; b[0].x = 4; b[0].y = 2; b[0].width = 48; b[0].height = 32;
    b[1].label = left; b[1].x = 0; b[1].y = top; b[1].width = W / 2; b[1].height = H - top; b[1].icon = leftIcon;
    b[1].active = leftActive;
    b[2].label = right; b[2].x = W / 2; b[2].y = top; b[2].width = W - W / 2; b[2].height = H - top;
    b[2].icon = rightIcon;
    d.renderButtonGrid(title, b, 0, 1);
  };
  pair("Dyktafon", "Nagraj", ui::IconId::Record, false, "Biblioteka (3)", ui::IconId::Book);
  dump(d, "plugin_dict_home");
  pair("Dyktafon", "Stop 00:12", ui::IconId::Stop, true, "Biblioteka (3)", ui::IconId::Book);
  dump(d, "plugin_dict_recording");
  pair("Klepsydra", "Pomodoro 25/5 x4", ui::IconId::None, false, "Start", ui::IconId::Play);
  dump(d, "plugin_focus_home");
  {
    const char *items[] = {"Nagranie 2026-09-27 10:14", "Nagranie 2026-09-26 21:03", "Notatka"};
    const int n = 3, rowH = H / n, iconZone = 120, backZone = 64;
    std::vector<Button> b;
    for (int i = 0; i < n; ++i) {
      Button row; row.label = items[i]; row.y = i * rowH; row.height = rowH; row.active = i == 0;
      row.x = i == 0 ? backZone : 0; row.width = i == 0 ? W - backZone - iconZone : W - iconZone;
      b.push_back(row);
      Button del; del.x = W - iconZone; del.y = row.y; del.width = iconZone; del.height = rowH;
      del.icon = ui::IconId::Delete; del.iconMaxSize = 22; b.push_back(del);
    }
    Button back; back.icon = ui::IconId::Back; back.x = 0; back.y = 2; back.width = 44; back.height = 26;
    b.push_back(back);
    d.renderButtonGrid("", b, 0, 2, "", true, true);
    dump(d, "plugin_dict_library");
  }
  {
    std::vector<Button> b;
    const char *labels[] = {"Biblioteka", "G\x83 -", "Pauza", "G\x83 +"};
    ui::IconId icons[] = {ui::IconId::Back, ui::IconId::None, ui::IconId::None, ui::IconId::None};
    for (int c = 0; c < 4; ++c) {
      Button x; x.label = labels[c]; x.icon = icons[c]; x.x = 4 + c * 149; x.y = 20; x.width = 145; x.height = 46;
      b.push_back(x);
    }
    Button s; s.kind = Button::ButtonKind::Slider; s.label = "Pozycja  G\x83:80%"; s.x = 4; s.y = 68; s.width = 632;
    s.height = 104; s.sliderMin = 0; s.sliderMax = 95; s.sliderValue = 31; s.sliderUnit = "s";
    b.push_back(s);
    d.renderButtonGrid("Nagranie 2026-09-27 10:14", b, 0, 1);
    dump(d, "plugin_dict_playing");
  }
  d.renderFocusTimerScreen("Gotowy", "", "25:00",
                           "Runda 1/4. Postaw na kr\xF3" "tszym boku albo dotknij, by zacz\x97" "\x9B", "", -1, false);
  dump(d, "plugin_focus_ready");
  d.renderFocusTimerScreen("Skupienie", "", "24:13", "Runda 1/4. Dotknij, by zapauzowa\x9B", "", 18, false);
  dump(d, "plugin_focus");
}
