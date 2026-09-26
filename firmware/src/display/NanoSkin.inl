// Nano skin — DisplayManager's rendering of rsvpnano's "regular" UI (the
// presentation their firmware uses on this same Waveshare 3.49" panel).
//
// Included at the bottom of DisplayManager.cpp rather than compiled on its
// own: it shares that file's anonymous-namespace helpers (panelColor(), the
// tiny 5x7 glyph table, the font-picker typeface override) and frame
// buffer constants. Shapes follow Arduino_GFX's algorithms (the library
// rsvpnano draws with) so corners, circles and lines land on the same
// pixels; widget geometry follows their src/ui/Ui.cpp and Controls.cpp.

#include "display/NanoUiFont.h"

namespace {

constexpr int kNanoScreenW = kDisplayWidth;
constexpr int kNanoScreenH = kDisplayHeight;

constexpr uint16_t nanoRgb(uint8_t r, uint8_t g, uint8_t b) {
  return static_cast<uint16_t>(((r & 0xF8U) << 8) | ((g & 0xFCU) << 3) | (b >> 3));
}

int nanoGlyphInkLeft(uint8_t code) {
  int left = kNanoUiGlyphWidth;
  for (int row = 0; row < kNanoUiGlyphHeight; ++row) {
    for (int col = 0; col < kNanoUiGlyphWidth; ++col) {
      if (kNanoUiFont[code][row] & (1 << (kNanoUiGlyphWidth - 1 - col))) {
        left = std::min(left, col);
        break;
      }
    }
  }
  return left;
}

int nanoGlyphInkRight(uint8_t code) {
  int right = -1;
  for (int row = 0; row < kNanoUiGlyphHeight; ++row) {
    for (int col = kNanoUiGlyphWidth - 1; col >= 0; --col) {
      if (kNanoUiFont[code][row] & (1 << (kNanoUiGlyphWidth - 1 - col))) {
        right = std::max(right, col);
        break;
      }
    }
  }
  return right;
}

// Horizontal ink extent of `text` at 1x, in cells*6 units relative to the
// first glyph's cell origin: [left, right] inclusive, or {0,-1} when blank.
void nanoInkBounds(const String &text, int &left, int &right) {
  left = 0;
  right = -1;
  bool found = false;
  for (size_t i = 0; i < text.length(); ++i) {
    const uint8_t code = static_cast<uint8_t>(text[i]);
    const int glyphRight = nanoGlyphInkRight(code);
    if (glyphRight < 0) {
      continue;
    }
    const int origin = static_cast<int>(i) * kNanoUiGlyphWidth;
    if (!found) {
      left = origin + nanoGlyphInkLeft(code);
      found = true;
    }
    right = origin + glyphRight;
  }
}

String nanoPercentLabel(uint8_t percent) { return String(static_cast<unsigned>(percent)) + "%"; }

}  // namespace

// ─── State ──────────────────────────────────────────────────────────────────

void DisplayManager::setBatteryState(bool present, uint8_t percent, bool charging) {
  batteryPresent_ = present;
  batteryPercent_ = std::min<uint8_t>(percent, 100);
  batteryCharging_ = charging;
}

// Fixed palettes, colors in NanoRole order (Background, Foreground, Muted,
// Subtle, Accent, OnAccent, SurfaceMuted, SurfaceActive, Outline,
// ProgressTrack). Where an rsvpnano theme's accentBar differs from its
// accent (Nord, Gruvbox, Tokyo Night, Solarized), the bar color is used as
// the accent: here the accent fills whole tiles and sliders, which the
// softer bar color carries better than their red focus-letter color.
namespace {

struct NanoPaletteDef {
  const char *name;
  uint16_t colors[10];
};

constexpr NanoPaletteDef kNanoPalettes[] = {
    {"Mocha", {0x18E5, 0xCEBE, 0xA579, 0x7C33, 0xF455, 0x1083, 0x3188, 0x422B, 0x5ACE, 0x3188}},  // rsvpnano catppuccin-mocha
    {"Macchiato", {0x2127, 0xCE9E, 0xA579, 0x8434, 0xEC32, 0x18C4, 0x31C9, 0x4A6C, 0x5B0F, 0x31C9}},  // rsvpnano catppuccin-macchiato
    {"Frappe", {0x31A8, 0xC69E, 0xA579, 0x8454, 0xE410, 0x2126, 0x422B, 0x52AD, 0x6350, 0x422B}},  // rsvpnano catppuccin-frappe
    {"Latte", {0xEF9E, 0x4A6D, 0x6B70, 0x8C74, 0xD067, 0xEF9E, 0xCE9B, 0xBE19, 0x9D16, 0xCE9B}},  // rsvpnano catppuccin-latte
    {"Dracula", {0x2946, 0xFFDE, 0xBDF7, 0x6394, 0xFBD8, 0x2946, 0x422B, 0x6394, 0x6394, 0x422B}},  // rsvpnano dracula
    {"Nord", {0x29A8, 0xEF7E, 0xDEFD, 0x8518, 0x8E1A, 0x29A8, 0x426B, 0x4AAD, 0x8518, 0x426B}},  // rsvpnano nord
    {"Gruvbox", {0x2945, 0xEED6, 0xACD0, 0x940E, 0xFDE5, 0x2945, 0x39C6, 0x5248, 0x62EA, 0x39C6}},  // rsvpnano gruvbox-dark
    {"Tokyo", {0x18C4, 0xC65E, 0x9D39, 0x52F1, 0x7D1E, 0x18C4, 0x2968, 0x3A0C, 0x52F1, 0x2968}},  // rsvpnano tokyo-night
    {"Solarized", {0x0146, 0xFFBC, 0x9514, 0x84B2, 0x245A, 0x0146, 0x09E9, 0x124A, 0x5B6E, 0x01A8}},  // rsvpnano solarized-dark
    {"Cream", {0xFFDB, 0x18C2, 0x6B4A, 0x83EC, 0x0373, 0xFFFF, 0xEEF5, 0xDE73, 0x9CAD, 0xDE73}},  // rsvpnano dyslexic
    {"Sepia", {0xF77B, 0x3965, 0x7B4B, 0x8BCC, 0xB2A5, 0xFFFF, 0xEEF8, 0xDE54, 0xACAF, 0xDE54}},  // ours: warm paper
    {"Graphite", {0x0000, 0xE73C, 0x8C51, 0x5AEB, 0x4D1F, 0x0000, 0x10A2, 0x2124, 0x39C7, 0x2124}},  // ours: OLED black, grey tiles
    {"Forest", {0x1924, 0xDF3B, 0x9D93, 0x6C2E, 0x7E2F, 0x1102, 0x2185, 0x3227, 0x4B0A, 0x2185}},  // ours: dark green
};
constexpr uint8_t kNanoFixedPaletteCount = sizeof(kNanoPalettes) / sizeof(kNanoPalettes[0]);

}  // namespace

uint8_t DisplayManager::nanoPaletteCount() { return kNanoFixedPaletteCount + 1; }

const char *DisplayManager::nanoPaletteName(uint8_t palette) {
  if (palette == kNanoPaletteClassic || palette > kNanoFixedPaletteCount) {
    return "Classic";
  }
  return kNanoPalettes[palette - 1].name;
}

void DisplayManager::setNanoPalette(uint8_t palette, bool ownAccent) {
  if (palette > kNanoFixedPaletteCount) {
    palette = kNanoPaletteClassic;
  }
  if (palette == nanoPalette_ && ownAccent == nanoOwnAccent_) {
    return;
  }
  nanoPalette_ = palette;
  nanoOwnAccent_ = ownAccent;
  lastRenderKey_ = "";
}

uint16_t DisplayManager::nanoPaletteColor(uint8_t palette, NanoRole role) const {
  if (palette != kNanoPaletteClassic && palette <= kNanoFixedPaletteCount) {
    return kNanoPalettes[palette - 1].colors[static_cast<uint8_t>(role)];
  }
  // Dark = rsvpnano's default theme, Light = their light.toml (background
  // kept at this firmware's softer 0xDEDA instead of pure white, so the
  // surfaces are shifted to stay distinguishable on it), Night = their
  // night.toml. Accent is always the user's highlight color.
  const bool night = nightMode_;
  const bool light = !night && !darkMode_;
  switch (role) {
    case NanoRole::Background:
      return backgroundColor();
    case NanoRole::Foreground:
      return wordColor();
    case NanoRole::Muted:
      return night ? nanoRgb(0x5F, 0x3A, 0x00) : light ? nanoRgb(0x6B, 0x6B, 0x6B) : 0x8410;
    case NanoRole::Subtle:
      return night ? nanoRgb(0x8A, 0x53, 0x00) : light ? nanoRgb(0x5A, 0x5A, 0x5A) : 0x528A;
    case NanoRole::Accent:
      return focusColor();
    case NanoRole::OnAccent:
      return night ? 0x0000 : 0xFFFF;
    case NanoRole::SurfaceMuted:
      return night ? nanoRgb(0x1A, 0x0F, 0x00) : light ? nanoRgb(0xEE, 0xEE, 0xEA) : 0x2104;
    case NanoRole::SurfaceActive:
      return night ? nanoRgb(0x33, 0x20, 0x00) : light ? nanoRgb(0xC4, 0xC4, 0xBE) : 0x4208;
    case NanoRole::Outline:
      return night ? nanoRgb(0x6B, 0x42, 0x00) : light ? nanoRgb(0x9A, 0x9A, 0x9A) : 0x8410;
    case NanoRole::ProgressTrack:
      return night ? nanoRgb(0x4F, 0x30, 0x00) : light ? nanoRgb(0xAE, 0xAE, 0xA8) : 0x8410;
  }
  return wordColor();
}

uint16_t DisplayManager::nanoColor(NanoRole role) const {
  if (nanoOwnAccent_ && nanoPalette_ != kNanoPaletteClassic) {
    if (role == NanoRole::Accent) {
      return focusColor();
    }
    if (role == NanoRole::OnAccent) {
      // The highlight colors range from yellow to deep blue: pick black or
      // white text by the accent's brightness instead of the palette's.
      const uint16_t accent = focusColor();
      const uint32_t luma = ((accent >> 11) & 0x1F) * 8 * 299 + ((accent >> 5) & 0x3F) * 4 * 587 +
                            (accent & 0x1F) * 8 * 114;
      return luma > 150000U ? 0x0000 : 0xFFFF;
    }
  }
  return nanoPaletteColor(nanoPalette_, role);
}

uint16_t DisplayManager::nanoBlend(NanoRole role, uint8_t alpha) const {
  const uint16_t fg = nanoColor(role);
  const uint16_t bg = nanoColor(NanoRole::Background);
  const uint32_t inv = 255U - alpha;
  const uint32_t r = (((fg >> 11) & 0x1F) * alpha + ((bg >> 11) & 0x1F) * inv) / 255U;
  const uint32_t g = (((fg >> 5) & 0x3F) * alpha + ((bg >> 5) & 0x3F) * inv) / 255U;
  const uint32_t b = ((fg & 0x1F) * alpha + (bg & 0x1F) * inv) / 255U;
  return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

// ─── Frame ──────────────────────────────────────────────────────────────────

void DisplayManager::nanoClearBackground(int width, int height) {
  if (virtualFrame_ == nullptr) {
    return;
  }
  const uint16_t background = panelColor(nanoColor(NanoRole::Background));
  for (int row = 0; row < height; ++row) {
    std::fill_n(virtualFrame_ + row * kVirtualBufferWidth, width, background);
  }
}

void DisplayManager::nanoBeginFrame() {
  nanoResetClip();
  nanoClearBackground(kNanoScreenW, kNanoScreenH);
}

void DisplayManager::nanoEndFrame() {
  if (!initialized_ || virtualFrame_ == nullptr) {
    return;
  }
  uint32_t hash = 2166136261U;
  for (int y = 0; y < kNanoScreenH; ++y) {
    const uint16_t *row = virtualFrame_ + y * kVirtualBufferWidth;
    for (int x = 0; x < kNanoScreenW; ++x) {
      hash = (hash ^ row[x]) * 16777619U;
    }
  }
  hash = (hash ^ static_cast<uint32_t>(uiOrientation_)) * 16777619U;
  if (hash == nanoFrameHash_ && panelWriteCount_ == nanoFramePanelWrites_ &&
      lastRenderKey_ == "nano") {
    return;
  }
  // Any other renderer comparing its own key must see a mismatch after this.
  lastRenderKey_ = "nano";
  flushScaledFrame(1, kNanoScreenW, kNanoScreenH);
  nanoFrameHash_ = hash;
  nanoFramePanelWrites_ = panelWriteCount_;
}

// ─── Primitives ─────────────────────────────────────────────────────────────

void DisplayManager::nanoSetClip(int x, int y, int w, int h) {
  nanoClipX0_ = std::max(0, x);
  nanoClipY0_ = std::max(0, y);
  nanoClipX1_ = std::min(kNanoScreenW, x + w);
  nanoClipY1_ = std::min(kNanoScreenH, y + h);
}

void DisplayManager::nanoResetClip() { nanoSetClip(0, 0, kNanoScreenW, kNanoScreenH); }

void DisplayManager::nanoSpan(int x, int y, int w, uint16_t color) {
  if (virtualFrame_ == nullptr || y < nanoClipY0_ || y >= nanoClipY1_ || w <= 0) {
    return;
  }
  const int x0 = std::max(x, nanoClipX0_);
  const int x1 = std::min(x + w, nanoClipX1_);
  if (x1 <= x0) {
    return;
  }
  std::fill_n(virtualFrame_ + y * kVirtualBufferWidth + x0, x1 - x0, panelColor(color));
}

void DisplayManager::nanoPixel(int x, int y, uint16_t color) {
  if (virtualFrame_ == nullptr || x < nanoClipX0_ || x >= nanoClipX1_ || y < nanoClipY0_ ||
      y >= nanoClipY1_) {
    return;
  }
  virtualFrame_[y * kVirtualBufferWidth + x] = panelColor(color);
}

void DisplayManager::nanoFillRect(int x, int y, int w, int h, uint16_t color) {
  for (int row = std::max(y, nanoClipY0_); row < std::min(y + h, nanoClipY1_); ++row) {
    nanoSpan(x, row, w, color);
  }
}

void DisplayManager::nanoDrawRect(int x, int y, int w, int h, uint16_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }
  nanoSpan(x, y, w, color);
  nanoSpan(x, y + h - 1, w, color);
  nanoFillRect(x, y, 1, h, color);
  nanoFillRect(x + w - 1, y, 1, h, color);
}

void DisplayManager::nanoCircleHelper(int x0, int y0, int r, uint8_t corners, uint16_t color) {
  int f = 1 - r;
  int ddFx = 1;
  int ddFy = -2 * r;
  int x = 0;
  int y = r;
  while (x < y) {
    if (f >= 0) {
      --y;
      ddFy += 2;
      f += ddFy;
    }
    ++x;
    ddFx += 2;
    f += ddFx;
    if (corners & 0x4) {
      nanoPixel(x0 + x, y0 + y, color);
      nanoPixel(x0 + y, y0 + x, color);
    }
    if (corners & 0x2) {
      nanoPixel(x0 + x, y0 - y, color);
      nanoPixel(x0 + y, y0 - x, color);
    }
    if (corners & 0x8) {
      nanoPixel(x0 - y, y0 + x, color);
      nanoPixel(x0 - x, y0 + y, color);
    }
    if (corners & 0x1) {
      nanoPixel(x0 - y, y0 - x, color);
      nanoPixel(x0 - x, y0 - y, color);
    }
  }
}

void DisplayManager::nanoFillCircleHelper(int x0, int y0, int r, uint8_t corners, int delta,
                                          uint16_t color) {
  int f = 1 - r;
  int ddFx = 1;
  int ddFy = -r - r;
  int x = 0;
  int y = r;
  int px = x;
  int py = y;
  ++delta;
  while (x < y) {
    if (f >= 0) {
      --y;
      ddFy += 2;
      f += ddFy;
    }
    ++x;
    ddFx += 2;
    f += ddFx;
    if (x < y + 1) {
      if (corners & 1) nanoFillRect(x0 + x, y0 - y, 1, 2 * y + delta, color);
      if (corners & 2) nanoFillRect(x0 - x, y0 - y, 1, 2 * y + delta, color);
    }
    if (y != py) {
      if (corners & 1) nanoFillRect(x0 + py, y0 - px, 1, 2 * px + delta, color);
      if (corners & 2) nanoFillRect(x0 - py, y0 - px, 1, 2 * px + delta, color);
      py = y;
    }
    px = x;
  }
}

void DisplayManager::nanoFillRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }
  r = std::max(0, std::min(r, std::min(w, h) / 2));
  nanoFillRect(x + r, y, w - 2 * r, h, color);
  nanoFillCircleHelper(x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
  nanoFillCircleHelper(x + r, y + r, r, 2, h - 2 * r - 1, color);
}

void DisplayManager::nanoDrawRoundRect(int x, int y, int w, int h, int r, uint16_t color) {
  if (w <= 0 || h <= 0) {
    return;
  }
  r = std::max(0, std::min(r, std::min(w, h) / 2));
  nanoSpan(x + r, y, w - 2 * r, color);
  nanoSpan(x + r, y + h - 1, w - 2 * r, color);
  nanoFillRect(x, y + r, 1, h - 2 * r, color);
  nanoFillRect(x + w - 1, y + r, 1, h - 2 * r, color);
  nanoCircleHelper(x + r, y + r, r, 1, color);
  nanoCircleHelper(x + w - r - 1, y + r, r, 2, color);
  nanoCircleHelper(x + w - r - 1, y + h - r - 1, r, 4, color);
  nanoCircleHelper(x + r, y + h - r - 1, r, 8, color);
}

void DisplayManager::nanoDrawLine(int x0, int y0, int x1, int y1, uint16_t color) {
  const bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
  if (steep) {
    std::swap(x0, y0);
    std::swap(x1, y1);
  }
  if (x0 > x1) {
    std::swap(x0, x1);
    std::swap(y0, y1);
  }
  const int dx = x1 - x0;
  const int dy = std::abs(y1 - y0);
  int err = dx / 2;
  const int ystep = y0 < y1 ? 1 : -1;
  for (; x0 <= x1; ++x0) {
    if (steep) {
      nanoPixel(y0, x0, color);
    } else {
      nanoPixel(x0, y0, color);
    }
    err -= dy;
    if (err < 0) {
      y0 += ystep;
      err += dx;
    }
  }
}

void DisplayManager::nanoFillCircle(int cx, int cy, int r, uint16_t color) {
  nanoFillRect(cx, cy - r, 1, 2 * r + 1, color);
  nanoFillCircleHelper(cx, cy, r, 3, 0, color);
}

void DisplayManager::nanoDrawCircle(int cx, int cy, int r, uint16_t color) {
  nanoPixel(cx, cy + r, color);
  nanoPixel(cx, cy - r, color);
  nanoPixel(cx + r, cy, color);
  nanoPixel(cx - r, cy, color);
  nanoCircleHelper(cx, cy, r, 0xF, color);
}

void DisplayManager::nanoFillTriangle(int x0, int y0, int x1, int y1, int x2, int y2,
                                      uint16_t color) {
  if (y0 > y1) {
    std::swap(y0, y1);
    std::swap(x0, x1);
  }
  if (y1 > y2) {
    std::swap(y2, y1);
    std::swap(x2, x1);
  }
  if (y0 > y1) {
    std::swap(y0, y1);
    std::swap(x0, x1);
  }
  if (y0 == y2) {
    const int a = std::min(x0, std::min(x1, x2));
    const int b = std::max(x0, std::max(x1, x2));
    nanoSpan(a, y0, b - a + 1, color);
    return;
  }
  const int dx01 = x1 - x0, dy01 = y1 - y0, dx02 = x2 - x0, dy02 = y2 - y0;
  const int dx12 = x2 - x1, dy12 = y2 - y1;
  int32_t sa = 0;
  int32_t sb = 0;
  const int last = (y1 == y2) ? y1 : y1 - 1;
  int y = y0;
  for (; y <= last; ++y) {
    int a = x0 + sa / dy01;
    int b = x0 + sb / dy02;
    sa += dx01;
    sb += dx02;
    if (a > b) std::swap(a, b);
    nanoSpan(a, y, b - a + 1, color);
  }
  sa = static_cast<int32_t>(dx12) * (y - y1);
  sb = static_cast<int32_t>(dx02) * (y - y0);
  for (; y <= y2; ++y) {
    int a = x1 + (dy12 != 0 ? sa / dy12 : 0);
    int b = x0 + sb / dy02;
    sa += dx12;
    sb += dx02;
    if (a > b) std::swap(a, b);
    nanoSpan(a, y, b - a + 1, color);
  }
}

// ─── Text ───────────────────────────────────────────────────────────────────

int DisplayManager::nanoTextWidth(const String &text, uint8_t size) {
  return static_cast<int>(text.length()) * kNanoUiGlyphWidth * std::max<uint8_t>(1, size);
}

int DisplayManager::nanoLineHeight(uint8_t size) {
  return kNanoUiGlyphHeight * std::max<uint8_t>(1, size);
}

void DisplayManager::nanoDrawGlyph(int x, int y, uint8_t code, uint8_t size, uint16_t color) {
  for (int row = 0; row < kNanoUiGlyphHeight; ++row) {
    const uint8_t bits = kNanoUiFont[code][row];
    if (bits == 0) {
      continue;
    }
    for (int col = 0; col < kNanoUiGlyphWidth; ++col) {
      if (bits & (1 << (kNanoUiGlyphWidth - 1 - col))) {
        nanoFillRect(x + col * size, y + row * size, size, size, color);
      }
    }
  }
}

void DisplayManager::nanoText(const ui::Rect &rect, const String &text, uint8_t size,
                              uint16_t color, NanoAlign align, uint8_t maxLines) {
  if (rect.w == 0 || rect.h == 0 || text.isEmpty()) {
    return;
  }
  maxLines = std::max<uint8_t>(1, std::min<uint8_t>(maxLines, 2));
  const size_t glyphs = text.length();
  size = std::max<uint8_t>(1, size);
  // Same shrink rule as rsvpnano's Context::appendText(): drop a size step
  // until the text fits in maxLines lines of the rect.
  while (size > 1) {
    const size_t columns = rect.w / (kNanoUiGlyphWidth * size);
    const size_t lines = columns == 0 ? SIZE_MAX : (glyphs + columns - 1) / columns;
    if (lines <= maxLines && static_cast<size_t>(kNanoUiGlyphHeight) * size * lines <= rect.h) {
      break;
    }
    --size;
  }
  const size_t capacity = rect.w / (kNanoUiGlyphWidth * size);
  if (capacity == 0) {
    return;
  }

  String first = text;
  String second;
  if (maxLines > 1 && glyphs > capacity) {
    int split = static_cast<int>(capacity);
    const int space = text.lastIndexOf(' ', split);
    if (space > 0 && static_cast<size_t>(space) >= capacity / 2) {
      split = space;
    }
    first = text.substring(0, split);
    second = text.substring(split);
    second.trim();
  }

  const int lineHeight = kNanoUiGlyphHeight * size;
  const int lineCount = second.isEmpty() ? 1 : 2;
  const int firstY = rect.y + std::max(0, (static_cast<int>(rect.h) - lineHeight * lineCount) / 2);
  auto drawLine = [&](const String &line, int lineY) {
    String shown = line;
    if (shown.length() > capacity) {
      shown = capacity > 3 ? line.substring(0, capacity - 3) + "..." : String("...").substring(0, capacity);
    }
    int inkLeft = 0;
    int inkRight = -1;
    nanoInkBounds(shown, inkLeft, inkRight);
    if (inkRight < inkLeft) {
      return;
    }
    const int inkWidth = (inkRight - inkLeft + 1) * size;
    const int startLeft = rect.x - inkLeft * size;
    int originX = startLeft;
    if (align == NanoAlign::Center) {
      originX = std::max(startLeft, rect.x + (static_cast<int>(rect.w) - inkWidth) / 2 - inkLeft * size);
    } else if (align == NanoAlign::End) {
      originX = std::max(startLeft, rect.x + static_cast<int>(rect.w) - inkWidth - inkLeft * size);
    }
    // Arduino_GFX puts a u8g2 baseline at lineTop + 9*size - size; the 6x9
    // cell's top row sits 7 rows above it.
    const int glyphTop = lineY + size;
    for (size_t i = 0; i < shown.length(); ++i) {
      nanoDrawGlyph(originX + static_cast<int>(i) * kNanoUiGlyphWidth * size, glyphTop,
                    static_cast<uint8_t>(shown[i]), size, color);
    }
  };
  drawLine(first, firstY);
  if (!second.isEmpty()) {
    drawLine(second, firstY + lineHeight);
  }
}

void DisplayManager::nanoSmallGlyph(int x, int y, char c, uint16_t color) {
  const uint8_t *rows = tinyRowsFor(c);
  for (int row = 0; row < kTinyGlyphHeight; ++row) {
    for (int col = 0; col < kTinyGlyphWidth; ++col) {
      if (rows[row] & (1 << (kTinyGlyphWidth - 1 - col))) {
        nanoPixel(x + col, y + row, color);
      }
    }
  }
}

// ─── Icons (rsvpnano src/ui/Icons.cpp) ──────────────────────────────────────

void DisplayManager::nanoIcon(const ui::Rect &rect, NanoIcon icon, uint16_t ink, uint16_t surface) {
  const int cx = rect.x + rect.w / 2;
  const int cy = rect.y + rect.h / 2;
  switch (icon) {
    case NanoIcon::Bookmark: {
      const int width = std::max(1, std::min(13, static_cast<int>(rect.w) - 6));
      const int height = std::max(1, std::min(30, static_cast<int>(rect.h) - 4));
      const int x = cx - width / 2;
      const int y = rect.y + 1;
      nanoFillRect(x, y, width, height, ink);
      for (int row = 0; row <= std::min(6, height - 1); ++row) {
        const int half = std::min(row, width / 2);
        nanoSpan(cx - half, y + height - 7 + row, half * 2 + 1, surface);
      }
      break;
    }
    case NanoIcon::Books: {
      const int x = cx - 9;
      const int y = cy - 9;
      nanoDrawRect(x, y, 5, 18, ink);
      nanoDrawRect(x + 6, y + 2, 5, 16, ink);
      nanoDrawRect(x + 12, y - 1, 6, 19, ink);
      break;
    }
    case NanoIcon::Edit: {
      const int x = cx - 9;
      const int y = cy - 9;
      nanoDrawRect(x, y, 14, 18, ink);
      nanoDrawLine(x + 5, y + 13, x + 18, y, ink);
      nanoDrawLine(x + 6, y + 16, x + 19, y + 3, ink);
      break;
    }
    case NanoIcon::Device: {
      const int x = cx - 8;
      const int y = cy - 10;
      nanoDrawRoundRect(x, y, 16, 20, 3, ink);
      nanoFillRect(cx - 2, y + 16, 4, 1, ink);
      break;
    }
    case NanoIcon::Language: {
      const int left = cx - 10;
      const int top = cy - 8;
      nanoDrawRoundRect(left, top, 13, 13, 2, ink);
      nanoDrawRoundRect(left + 7, top + 5, 13, 13, 2, ink);
      nanoDrawLine(left + 3, top + 10, left + 6, top + 3, ink);
      nanoDrawLine(left + 6, top + 3, left + 9, top + 10, ink);
      nanoSpan(left + 4, top + 7, 5, ink);
      nanoSpan(left + 10, top + 10, 7, ink);
      nanoFillRect(left + 13, top + 8, 1, 7, ink);
      break;
    }
    case NanoIcon::Hourglass: {
      const int x = cx - 8;
      const int y = cy - 9;
      nanoDrawLine(x, y, x + 16, y, ink);
      nanoDrawLine(x, y + 18, x + 16, y + 18, ink);
      nanoDrawLine(x + 2, y + 1, x + 14, y + 17, ink);
      nanoDrawLine(x + 14, y + 1, x + 2, y + 17, ink);
      nanoFillRect(cx - 3, cy + 5, 6, 2, ink);
      break;
    }
    case NanoIcon::Power: {
      nanoDrawCircle(cx, cy + 1, 8, ink);
      nanoFillRect(cx - 3, cy - 9, 7, 10, surface);
      nanoFillRect(cx, cy - 9, 1, 9, ink);
      break;
    }
    case NanoIcon::Apps: {
      // Not in rsvpnano's set (they have no plugins) — four outlined tiles
      // drawn in the same 1px, ~18px-box style as Books/Device.
      const int x = cx - 9;
      const int y = cy - 9;
      nanoDrawRoundRect(x, y, 8, 8, 2, ink);
      nanoDrawRoundRect(x + 10, y, 8, 8, 2, ink);
      nanoDrawRoundRect(x, y + 10, 8, 8, 2, ink);
      nanoDrawRoundRect(x + 10, y + 10, 8, 8, 2, ink);
      break;
    }
    case NanoIcon::Palette: {
      // Painter's palette: a round board with a thumb hole in the lower
      // right and three paint dots, same 1px, ~18px-box style.
      nanoDrawCircle(cx, cy, 9, ink);
      nanoFillCircle(cx + 5, cy + 5, 3, surface);
      nanoDrawCircle(cx + 5, cy + 5, 2, ink);
      nanoFillCircle(cx - 4, cy - 3, 2, ink);
      nanoFillCircle(cx + 1, cy - 5, 2, ink);
      nanoFillCircle(cx - 4, cy + 3, 2, ink);
      break;
    }
    case NanoIcon::None:
    default:
      break;
  }
}

void DisplayManager::nanoBatteryIcon(int x, int y, int w, int h, uint8_t percent, bool charging,
                                     uint16_t ink, uint16_t surface) {
  constexpr uint16_t kBatteryGood = nanoRgb(126, 176, 92);
  constexpr uint16_t kBatteryMedium = nanoRgb(214, 163, 58);
  constexpr uint16_t kBatteryLow = nanoRgb(200, 82, 82);
  constexpr int kCapWidth = 3;
  constexpr int kCapHeight = 5;
  if (w <= kCapWidth || h <= 4) {
    return;
  }
  percent = std::min<uint8_t>(percent, 100);
  const int bodyWidth = w - kCapWidth;
  const uint16_t fillColor = charging || percent > 35 ? kBatteryGood
                             : percent <= 18          ? kBatteryLow
                                                      : kBatteryMedium;
  nanoDrawRect(x, y, bodyWidth, h, ink);
  nanoFillRect(x + bodyWidth, y + (h - kCapHeight) / 2, kCapWidth, kCapHeight, ink);
  const int innerWidth = std::max(0, bodyWidth - 4);
  const int fill = charging ? innerWidth : innerWidth * percent / 100;
  if (fill > 0) {
    nanoFillRect(x + 2, y + 2, fill, h - 4, fillColor);
  }
  if (charging) {
    nanoDrawLine(x + 15, y + 2, x + 11, y + 7, surface);
    nanoDrawLine(x + 11, y + 7, x + 16, y + 7, surface);
    nanoDrawLine(x + 16, y + 7, x + 12, y + 12, surface);
  }
}

// ─── Widgets (rsvpnano src/ui/Ui.cpp) ───────────────────────────────────────

void DisplayManager::nanoLabel(const ui::Rect &rect, const String &text, uint8_t size, NanoRole role,
                               NanoAlign align, uint8_t maxLines) {
  nanoText(rect, text, size, nanoColor(role), align, maxLines);
}

void DisplayManager::nanoSeparator(const ui::Rect &rect, const String &text) {
  const int labelWidth = std::min(static_cast<int>(rect.w), nanoTextWidth(text, 1));
  nanoText({rect.x, rect.y, static_cast<uint16_t>(labelWidth), rect.h}, text, 1,
           nanoColor(NanoRole::Muted));
  const int lineX = rect.x + labelWidth + 6;
  if (lineX < rect.x + rect.w) {
    nanoSpan(lineX, rect.y + rect.h / 2, rect.x + rect.w - lineX, nanoBlend(NanoRole::Muted, 96));
  }
}

void DisplayManager::nanoButton(const ui::Rect &rect, const String &text, bool enabled,
                                NanoIcon icon, uint8_t textLines, const String &detailLeft,
                                const String &detailRight, bool pressed, bool armed,
                                ReaderTypeface previewTypeface) {
  const int x = rect.x;
  const int y = rect.y;
  const int w = rect.w;
  const int h = rect.h;
  const uint16_t surface = armed     ? nanoColor(NanoRole::Accent)
                           : pressed ? nanoColor(NanoRole::SurfaceActive)
                                     : nanoColor(NanoRole::SurfaceMuted);
  nanoFillRoundRect(x, y, w, h, 5, surface);
  nanoDrawRoundRect(x, y, w, h, 5,
                    nanoColor(enabled ? NanoRole::Outline : NanoRole::ProgressTrack));
  if (enabled && !armed && w > 16 && h >= 28) {
    nanoFillRect(x + 8, y + h - 3, w - 16, 2, nanoColor(NanoRole::Accent));
  }

  const int iconWidth = icon == NanoIcon::None ? 0 : std::min(34, w / 3);
  const bool hasDetail = !detailLeft.isEmpty() || !detailRight.isEmpty();
  const int textHeight = hasDetail ? h - 18 : h;
  const ui::Rect textRect{static_cast<uint16_t>(x + 6), static_cast<uint16_t>(y),
                          static_cast<uint16_t>(std::max(0, w - iconWidth - 12)),
                          static_cast<uint16_t>(std::max(0, textHeight))};
  const uint16_t ink = armed     ? nanoColor(NanoRole::OnAccent)
                       : enabled ? nanoColor(NanoRole::Foreground)
                                 : nanoColor(NanoRole::Muted);

  if (previewTypeface != ReaderTypeface::Count) {
    // Font picker: the label is the font's own name set in that font.
    gButtonLabelPreviewTypeface = previewTypeface;
    uint8_t scalePercent = 26;
    if (isExtraTypeface(previewTypeface)) {
      const int sourceGlyphHeight = std::max(1, baseGlyphHeightForTypeface(previewTypeface));
      const int targetHeightPx = (kEmbeddedSerifHeight * scalePercent + 50) / 100;
      scalePercent = static_cast<uint8_t>(
          std::min(100, std::max(10, (targetHeightPx * 100 + sourceGlyphHeight / 2) / sourceGlyphHeight)));
    }
    const String label = fitSerifTextScaled(text, textRect.w, scalePercent);
    const int labelW = measureSerifTextWidthScaled(label, scalePercent);
    const int labelH = scaledPercentDimension(baseGlyphHeightForTypeface(previewTypeface), scalePercent);
    drawSerifTextScaledAt(label, textRect.x + std::max(0, (static_cast<int>(textRect.w) - labelW) / 2),
                          y + std::max(1, (h - labelH) / 2), ink, scalePercent);
    gButtonLabelPreviewTypeface = ReaderTypeface::Count;
  } else {
    nanoText(textRect, text, 2, ink, NanoAlign::Center, textLines);
  }

  if (hasDetail) {
    const int detailY = y + h - 20;
    const uint16_t detailInk = armed ? ink : nanoColor(NanoRole::Muted);
    if (detailLeft.isEmpty() || detailRight.isEmpty()) {
      nanoText({textRect.x, static_cast<uint16_t>(detailY), textRect.w, 16},
               detailLeft.isEmpty() ? detailRight : detailLeft, 2, detailInk,
               detailLeft.isEmpty() ? NanoAlign::End : NanoAlign::Start);
    } else {
      const uint16_t detailWidth = static_cast<uint16_t>((textRect.w - 8) / 2);
      nanoText({textRect.x, static_cast<uint16_t>(detailY), detailWidth, 16}, detailLeft, 2,
               detailInk, NanoAlign::Start);
      nanoText({static_cast<uint16_t>(textRect.x + textRect.w - detailWidth), static_cast<uint16_t>(detailY),
                detailWidth, 16},
               detailRight, 2, detailInk, NanoAlign::End);
    }
  }
  if (icon != NanoIcon::None) {
    nanoIcon({static_cast<uint16_t>(x + w - iconWidth), static_cast<uint16_t>(y),
              static_cast<uint16_t>(iconWidth), static_cast<uint16_t>(h)},
             icon, armed ? ink : nanoColor(enabled ? NanoRole::Accent : NanoRole::Muted), surface);
  }
}

void DisplayManager::nanoIconButton(const ui::Rect &rect, NanoIcon icon, bool pressed) {
  const uint16_t surface =
      pressed ? nanoColor(NanoRole::SurfaceActive) : nanoColor(NanoRole::SurfaceMuted);
  nanoFillRoundRect(rect.x, rect.y, rect.w, rect.h, 7, surface);
  nanoDrawRoundRect(rect.x, rect.y, rect.w, rect.h, 7, nanoColor(NanoRole::Outline));
  nanoIcon(rect, icon, nanoColor(NanoRole::Muted), surface);
}

void DisplayManager::nanoTab(const ui::Rect &rect, const String &text, bool active, NanoIcon icon,
                             bool pressed, bool badge, bool markerRight) {
  const uint16_t surface = pressed  ? nanoColor(NanoRole::SurfaceActive)
                           : active ? nanoColor(NanoRole::Background)
                                    : nanoColor(NanoRole::SurfaceMuted);
  nanoFillRect(rect.x, rect.y, rect.w, rect.h, surface);
  nanoDrawRect(rect.x, rect.y, rect.w, rect.h, nanoColor(NanoRole::Outline));
  if (active) {
    nanoFillRect(markerRight ? rect.x + rect.w - 3 : rect.x, rect.y + 5, 3, rect.h - 10,
                 nanoColor(NanoRole::Accent));
  }
  const uint16_t ink = nanoColor(active ? NanoRole::Foreground : NanoRole::Muted);
  if (text.isEmpty()) {
    nanoIcon(rect, icon, active ? nanoColor(NanoRole::Accent) : ink, surface);
    if (badge) {
      nanoFillCircle(rect.x + rect.w - 9, rect.y + 8, 3, nanoColor(NanoRole::Accent));
    }
    return;
  }
  const int iconWidth = icon == NanoIcon::None ? 0 : std::min(26, rect.w / 3);
  if (icon != NanoIcon::None) {
    nanoIcon({static_cast<uint16_t>(rect.x + 7), rect.y, static_cast<uint16_t>(iconWidth), rect.h}, icon,
             ink, surface);
  }
  nanoText({static_cast<uint16_t>(rect.x + iconWidth + 8), rect.y,
            static_cast<uint16_t>(std::max(0, static_cast<int>(rect.w) - iconWidth - 12)), rect.h},
           text, 2, ink, NanoAlign::Center);
  if (badge) {
    nanoFillCircle(rect.x + rect.w - 9, rect.y + 8, 3, nanoColor(NanoRole::Accent));
  }
}

void DisplayManager::nanoSetting(const ui::Rect &rect, const String &label, const String &value,
                                 bool inlineLayout, bool pressed) {
  const uint16_t surface =
      pressed ? nanoColor(NanoRole::SurfaceActive) : nanoColor(NanoRole::SurfaceMuted);
  nanoFillRoundRect(rect.x, rect.y, rect.w, rect.h, 5, surface);
  nanoDrawRoundRect(rect.x, rect.y, rect.w, rect.h, 5, nanoColor(NanoRole::Outline));
  const int textWidth = std::max(0, static_cast<int>(rect.w) - 14);
  if (inlineLayout) {
    // Label left in the foreground color, value right in the accent — when
    // both don't fit at 2x the value drops to 1x first, then the label gets
    // half the row (rsvpnano's Context::setting() Inline branch).
    const int labelRequired = nanoTextWidth(label, 2);
    uint8_t valueSize = 2;
    int valueRequired = nanoTextWidth(value, 2);
    if (labelRequired + valueRequired + 8 > textWidth) {
      valueSize = 1;
      valueRequired = nanoTextWidth(value, 1);
    }
    const int labelWidth = labelRequired + valueRequired + 8 <= textWidth
                               ? labelRequired
                               : std::max(std::min(labelRequired, textWidth / 2), textWidth - valueRequired - 8);
    const int valueWidth = std::max(0, textWidth - labelWidth - 8);
    nanoText({static_cast<uint16_t>(rect.x + 7), rect.y, static_cast<uint16_t>(std::max(0, labelWidth)), rect.h},
             label, 2, nanoColor(NanoRole::Foreground), NanoAlign::Start, rect.h >= 32 ? 2 : 1);
    nanoText({static_cast<uint16_t>(rect.x + rect.w - valueWidth - 7), rect.y,
              static_cast<uint16_t>(valueWidth), rect.h},
             value, valueSize, nanoColor(NanoRole::Accent), NanoAlign::End);
  } else {
    const bool largeValue = nanoTextWidth(value, 2) <= textWidth;
    nanoText({static_cast<uint16_t>(rect.x + 7), static_cast<uint16_t>(rect.y + 3),
              static_cast<uint16_t>(textWidth), 8},
             label, 1, nanoColor(NanoRole::Muted));
    nanoText({static_cast<uint16_t>(rect.x + 7), static_cast<uint16_t>(rect.y + 11),
              static_cast<uint16_t>(textWidth), static_cast<uint16_t>(std::max(0, rect.h - 13))},
             value, largeValue ? 2 : 1, nanoColor(NanoRole::Accent), NanoAlign::Start,
             !largeValue && rect.h >= 32 ? 2 : 1);
  }
}

void DisplayManager::nanoToggle(const ui::Rect &rect, const String &label, bool on, bool pressed) {
  const uint16_t surface =
      pressed ? nanoColor(NanoRole::SurfaceActive) : nanoColor(NanoRole::SurfaceMuted);
  nanoFillRoundRect(rect.x, rect.y, rect.w, rect.h, 5, surface);
  nanoDrawRoundRect(rect.x, rect.y, rect.w, rect.h, 5, nanoColor(NanoRole::Outline));
  constexpr int kSwitchWidth = 34;
  const int switchX = rect.x + rect.w - kSwitchWidth - 7;
  const int switchY = rect.y + (static_cast<int>(rect.h) - 16) / 2;
  nanoFillRoundRect(switchX, switchY, kSwitchWidth, 16, 8,
                    nanoColor(on ? NanoRole::Accent : NanoRole::ProgressTrack));
  nanoFillCircle(switchX + (on ? kSwitchWidth - 8 : 8), switchY + 8, 6, nanoColor(NanoRole::Foreground));
  nanoText({static_cast<uint16_t>(rect.x + 7), rect.y,
            static_cast<uint16_t>(std::max(0, static_cast<int>(rect.w) - kSwitchWidth - 21)), rect.h},
           label, 2, nanoColor(NanoRole::Foreground), NanoAlign::Start, rect.h >= 32 ? 2 : 1);
}

void DisplayManager::nanoProgress(const ui::Rect &rect, int value, int minimum, int maximum) {
  value = std::max(minimum, std::min(value, maximum));
  nanoFillRect(rect.x, rect.y, rect.w, rect.h, nanoColor(NanoRole::ProgressTrack));
  if (maximum > minimum && rect.w > 2 && rect.h > 2) {
    const int fill = (static_cast<int>(rect.w) - 2) * (value - minimum) / (maximum - minimum);
    nanoFillRect(rect.x + 1, rect.y + 1, fill, rect.h - 2, nanoColor(NanoRole::Accent));
  }
}

void DisplayManager::nanoSlider(const ui::Rect &rect, const String &label, const String &valueText,
                                int value, int minimum, int maximum, bool pressed, bool dragging) {
  const int x = rect.x;
  const int y = rect.y;
  const int w = rect.w;
  const int h = rect.h;
  if (w <= 4 || h <= 4) {
    return;
  }
  const int range = maximum - minimum;
  value = std::max(minimum, std::min(value, maximum));
  // Even the minimum keeps a sliver of fill, so the tile never reads as a
  // plain button.
  constexpr int kMinFill = 6;
  const int fill = range > 0 ? kMinFill + (w - kMinFill) * (value - minimum) / range : w;

  const uint16_t surface = nanoColor(pressed || dragging ? NanoRole::SurfaceActive : NanoRole::SurfaceMuted);
  const uint16_t accent = nanoColor(NanoRole::Accent);
  nanoFillRoundRect(x, y, w, h, 5, surface);
  nanoSetClip(x, y, fill, h);
  nanoFillRoundRect(x, y, w, h, 5, accent);
  nanoResetClip();
  nanoDrawRoundRect(x, y, w, h, 5, dragging ? accent : nanoColor(NanoRole::Outline));
  if (fill > 8 && fill < w - 4) {
    // Grip at the fill edge: the part a finger drags.
    nanoFillRect(x + fill - 3, y + h / 2 - 7, 2, 14, nanoColor(NanoRole::OnAccent));
  }

  // Label left, value right (nanoSetting()'s inline rule), drawn twice:
  // clipped to the filled and to the empty part, so each half keeps its
  // contrast.
  const int textWidth = std::max(0, w - 14);
  const int labelRequired = nanoTextWidth(label, 2);
  uint8_t valueSize = 2;
  int valueRequired = nanoTextWidth(valueText, 2);
  if (labelRequired + valueRequired + 8 > textWidth) {
    valueSize = 1;
    valueRequired = nanoTextWidth(valueText, 1);
  }
  const int labelWidth = labelRequired + valueRequired + 8 <= textWidth
                             ? labelRequired
                             : std::max(std::min(labelRequired, textWidth / 2), textWidth - valueRequired - 8);
  const int valueWidth = std::max(0, textWidth - labelWidth - 8);
  const ui::Rect labelRect(x + 7, y, std::max(0, labelWidth), h);
  const ui::Rect valueRect(x + w - valueWidth - 7, y, valueWidth, h);
  const uint8_t labelLines = h >= 32 ? 2 : 1;
  const uint16_t onAccent = nanoColor(NanoRole::OnAccent);
  const uint16_t foreground = nanoColor(NanoRole::Foreground);
  nanoSetClip(x + fill, y, w - fill, h);
  nanoText(labelRect, label, 2, foreground, NanoAlign::Start, labelLines);
  nanoText(valueRect, valueText, valueSize, foreground, NanoAlign::End);
  nanoSetClip(x, y, fill, h);
  nanoText(labelRect, label, 2, onAccent, NanoAlign::Start, labelLines);
  nanoText(valueRect, valueText, valueSize, onAccent, NanoAlign::End);
  nanoResetClip();
}

void DisplayManager::nanoPaletteChip(const ui::Rect &rect, uint8_t palette, const String &name,
                                     bool selected, bool pressed) {
  const int x = rect.x;
  const int y = rect.y;
  const int w = rect.w;
  const int h = rect.h;
  if (w <= 16 || h <= 16) {
    return;
  }
  const uint16_t background = nanoPaletteColor(palette, NanoRole::Background);
  const uint16_t surface =
      nanoPaletteColor(palette, pressed ? NanoRole::SurfaceActive : NanoRole::SurfaceMuted);
  nanoFillRoundRect(x, y, w, h, 5, background);
  // Mini tile in the palette's own surface with its accent underline: how
  // its buttons will look.
  const int barY = y + h - 11;
  nanoFillRoundRect(x + 6, barY, w - 12, 7, 3, surface);
  nanoFillRect(x + 6, barY + 5, (w - 12) * 2 / 5, 2, nanoPaletteColor(palette, NanoRole::Accent));
  nanoFillCircle(x + w - 12, barY + 3, 2, nanoPaletteColor(palette, NanoRole::Accent));
  nanoText(ui::Rect(x + 6, y + 2, w - 12, std::max(0, h - 15)), name, 2,
           nanoPaletteColor(palette, NanoRole::Foreground), NanoAlign::Start);
  if (selected) {
    const uint16_t ring = nanoColor(NanoRole::Accent);
    nanoDrawRoundRect(x, y, w, h, 5, ring);
    nanoDrawRoundRect(x + 1, y + 1, w - 2, h - 2, 4, ring);
  } else {
    nanoDrawRoundRect(x, y, w, h, 5, nanoColor(NanoRole::Outline));
  }
}

void DisplayManager::nanoBatteryStack(const ui::Rect &rect) {
  if (!batteryPresent_ && batteryLabel_.isEmpty()) {
    return;
  }
  constexpr int kIconW = 29;
  constexpr int kIconH = 13;
  const int iconX = rect.x + (static_cast<int>(rect.w) - kIconW) / 2;
  const uint16_t ink = nanoColor(NanoRole::Muted);
  nanoBatteryIcon(iconX, rect.y, kIconW, kIconH, batteryPercent_, batteryCharging_, ink,
                  nanoColor(NanoRole::Background));
  const String label = batteryPresent_ ? nanoPercentLabel(batteryPercent_) : batteryLabel_;
  nanoText({rect.x, static_cast<uint16_t>(rect.y + kIconH + 3), rect.w, 10}, label, 1, ink,
           NanoAlign::Center);
}

// ─── Generic Button painting for the list renderers (Nano skin) ─────────────

void DisplayManager::drawNanoButton(const Button &button) {
  const ui::Rect rect{button.x, button.y, button.width, button.height};
  switch (button.kind) {
    case Button::ButtonKind::Separator:
      nanoSeparator(rect, button.label);
      return;
    case Button::ButtonKind::Label:
      nanoText({rect.x, rect.y, rect.w, static_cast<uint16_t>(button.sublabel.isEmpty() ? rect.h : rect.h / 2)},
               button.label, 2, nanoColor(NanoRole::Muted), NanoAlign::Center, 2);
      if (!button.sublabel.isEmpty()) {
        nanoText({rect.x, static_cast<uint16_t>(rect.y + rect.h / 2), rect.w, static_cast<uint16_t>(rect.h / 2)},
                 button.sublabel, 2, nanoColor(NanoRole::Muted), NanoAlign::Center, 2);
      }
      return;
    case Button::ButtonKind::Toggle:
      nanoToggle(rect, button.label, button.active, button.armed);
      return;
    case Button::ButtonKind::Slider: {
      // Full-screen drag editor. The track keeps sliderTrackRectFor()'s x
      // range (the touch mapping reads it back) but is drawn the rsvpnano
      // way: a 3px bar with tick marks and a round knob.
      const bool hasValueLabels =
          !button.sliderValueLabels.empty() && button.sliderValue < button.sliderValueLabels.size();
      const String valueText = hasValueLabels ? button.sliderValueLabels[button.sliderValue]
                                              : String(button.sliderValue) + button.sliderUnit;
      nanoText({static_cast<uint16_t>(rect.x + 64), static_cast<uint16_t>(rect.y + 4),
                static_cast<uint16_t>(std::max(0, static_cast<int>(rect.w) - 128)), 24},
               button.label, 2, nanoColor(NanoRole::Muted), NanoAlign::Center);
      nanoText({rect.x, static_cast<uint16_t>(rect.y + 36), rect.w, 40}, valueText, 4,
               nanoColor(NanoRole::Accent), NanoAlign::Center);
      // Same x range as sliderTrackRectFor() (the touch mapping reads it
      // back), drawn as a thick bar whose accent fill is the value.
      const ui::Rect track = sliderTrackRectFor(button);
      constexpr int kBarHeight = 28;
      const int barY = track.y + track.h / 2 - kBarHeight / 2;
      const int range = static_cast<int>(button.sliderMax) - static_cast<int>(button.sliderMin);
      const int value = std::max(static_cast<int>(button.sliderMin),
                                 std::min(static_cast<int>(button.sliderValue), static_cast<int>(button.sliderMax)));
      const int fill = range > 0 ? 8 + (static_cast<int>(track.w) - 8) * (value - button.sliderMin) / range
                                 : static_cast<int>(track.w);
      nanoFillRoundRect(track.x, barY, track.w, kBarHeight, 7, nanoColor(NanoRole::SurfaceMuted));
      if (range > 0) {
        for (int i = 1; i < 10; ++i) {
          nanoFillRect(track.x + (static_cast<int>(track.w) - 1) * i / 10, barY + kBarHeight - 7, 1, 4,
                       nanoColor(NanoRole::Outline));
        }
      }
      nanoSetClip(track.x, barY, fill, kBarHeight);
      nanoFillRoundRect(track.x, barY, track.w, kBarHeight, 7, nanoColor(NanoRole::Accent));
      nanoResetClip();
      nanoDrawRoundRect(track.x, barY, track.w, kBarHeight, 7, nanoColor(NanoRole::Outline));
      if (fill > 10 && fill < static_cast<int>(track.w) - 4) {
        nanoFillRect(track.x + fill - 4, barY + 6, 2, kBarHeight - 12, nanoColor(NanoRole::OnAccent));
      }
      const String minText = hasValueLabels && button.sliderMin < button.sliderValueLabels.size()
                                 ? button.sliderValueLabels[button.sliderMin]
                                 : String(button.sliderMin);
      const String maxText = hasValueLabels && button.sliderMax < button.sliderValueLabels.size()
                                 ? button.sliderValueLabels[button.sliderMax]
                                 : String(button.sliderMax);
      const uint16_t endY = static_cast<uint16_t>(barY + kBarHeight + 3);
      nanoText({track.x, endY, static_cast<uint16_t>(track.w / 2), 10}, minText, 1,
               nanoColor(NanoRole::Muted), NanoAlign::Start);
      nanoText({static_cast<uint16_t>(track.x + track.w / 2), endY, static_cast<uint16_t>(track.w / 2), 10},
               maxText, 1, nanoColor(NanoRole::Muted), NanoAlign::End);
      return;
    }
    default:
      break;
  }

  if (button.label.isEmpty() && button.icon == ui::IconId::Back) {
    nanoButton(rect, "<<", true, NanoIcon::None, 1, "", "", button.armed);
    return;
  }
  if (button.label.isEmpty() && button.icon == ui::IconId::Check) {
    nanoButton(rect, "OK", true, NanoIcon::None, 1, "", "", button.armed);
    return;
  }
  nanoButton(rect, button.label, true, NanoIcon::None, 2, button.sublabel, "", button.active && !button.armed,
             button.armed, button.previewTypeface);
}
