#include "ui/NanoScreens.h"

#include <algorithm>

namespace nano {

namespace {

Layout gLayout;

int right(const Rect &rect) { return rect.x + rect.w; }
int bottom(const Rect &rect) { return rect.y + rect.h; }

void addTarget(Sink &sink, const Rect &rect, int id) {
  if (id != kNoTarget) {
    sink.target(rect, id);
  }
}

void iconButton(DisplayManager &d, Sink &sink, const Rect &rect, int id, Icon icon, bool enabled = true) {
  d.nanoButton(rect, "", enabled, icon, 1, "", "", sink.pressed(id));
  if (enabled) {
    addTarget(sink, rect, id);
  }
}

constexpr int kHelpButtonW = 34;

// Cuts the "?" button off the right of `rect` (when the item has one) and
// paints it; returns what is left for the tile itself.
Rect withHelpButton(DisplayManager &d, Sink &sink, const Rect &rect, int helpId) {
  if (helpId == kNoTarget || rect.w < 120) {
    return rect;
  }
  const Rect help(rect.x + rect.w - kHelpButtonW + 4, rect.y, kHelpButtonW - 4, rect.h);
  const int r = std::min<int>(13, (std::min(help.w, help.h) - 2) / 2);
  const int cx = help.x + help.w / 2;
  const int cy = help.y + help.h / 2;
  d.nanoFillCircle(cx, cy, r, d.nanoColor(sink.pressed(helpId) ? Role::SurfaceActive : Role::SurfaceMuted));
  d.nanoText(Rect(cx - r, cy - r, r * 2 + 1, r * 2 + 1), "?", 2, d.nanoColor(Role::Muted), Align::Center);
  addTarget(sink, help, helpId);
  return Rect(rect.x, rect.y, rect.w - kHelpButtonW, rect.h);
}

}  // namespace

Layout &layout() { return gLayout; }

int railWidthFor(const std::vector<String> &labels) {
  int widest = 0;
  for (const String &label : labels) {
    widest = std::max(widest, DisplayManager::nanoTextWidth(label, 2));
  }
  // nanoTab(): 6px pill inset, 10px + 22px icon + 8px, text, 8px + 6px.
  return std::max(136, std::min(196, widest + 62));
}

Rect railRect() {
  const int width = gLayout.compact ? kCompactRailW : gLayout.railWidth;
  return Rect(gLayout.railRight ? kScreenW - width : 0, 0, width, kScreenH);
}

Rect tabContent() {
  const Rect rail = railRect();
  if (gLayout.railRight) {
    return Rect(kMargin, 8, rail.x - kMargin * 2, kScreenH - 16);
  }
  const int x = rail.w + kMargin;
  return Rect(x, 8, kScreenW - x - kMargin, kScreenH - 16);
}

Rect fullContent() { return Rect(kMargin, 8, kScreenW - kMargin * 2, kScreenH - 16); }

// ─── Rail ───────────────────────────────────────────────────────────────────

void paintRail(DisplayManager &d, Sink &sink, const std::vector<RailTab> &tabs) {
  const Rect rail = railRect();
  d.nanoRailBackground(rail);
  if (tabs.empty()) {
    return;
  }
  const int top = 4;
  const int available = kScreenH - kRailFooterH - top;
  const int tabHeight = available / static_cast<int>(tabs.size());
  int y = top;
  for (size_t i = 0; i < tabs.size(); ++i) {
    const Rect rect(rail.x, y, rail.w, tabHeight);
    d.nanoTab(rect, gLayout.compact ? String() : tabs[i].label, tabs[i].active, tabs[i].icon,
              sink.pressed(tabs[i].id), tabs[i].badge, gLayout.railRight);
    addTarget(sink, rect, tabs[i].id);
    y += tabHeight;
  }
  d.nanoBatteryInline(Rect(rail.x, kScreenH - kRailFooterH, rail.w, kRailFooterH - 2), gLayout.compact);
}

// ─── Header ─────────────────────────────────────────────────────────────────

int paintHeader(DisplayManager &d, Sink &sink, const Rect &area, const Header &header) {
  int x = area.x;
  int limit = right(area);
  const int y = area.y;
  if (header.backId != kNoTarget) {
    iconButton(d, sink, Rect(x, y, 48, kHeaderH), header.backId, Icon::ChevronLeft);
    x += 48 + 10;
  }
  if (header.pageCount > 1) {
    const Rect next(limit - 36, y, 36, kHeaderH);
    const Rect label(next.x - 48, y, 48, kHeaderH);
    const Rect prev(label.x - 36, y, 36, kHeaderH);
    iconButton(d, sink, prev, header.prevId, Icon::ChevronLeft, header.page > 0);
    iconButton(d, sink, next, header.nextId, Icon::ChevronRight, header.page + 1 < header.pageCount);
    d.nanoLabel(label,
                String(static_cast<unsigned>(header.page + 1)) + "/" + String(static_cast<unsigned>(header.pageCount)),
                1, Role::Muted, Align::Center);
    limit = prev.x - 10;
  }
  if (header.pillId != kNoTarget && !header.pillLabel.isEmpty()) {
    const int wanted = DisplayManager::nanoTextWidth(header.pillLabel, 1) + 28 + (header.pillIcon == Icon::None ? 0 : 24);
    const int width = std::min(wanted, std::max(60, (limit - x) / 2));
    const Rect pill(limit - width, y + 2, width, kHeaderH - 4);
    d.nanoPill(pill, header.pillLabel, header.pillIcon, sink.pressed(header.pillId), false);
    addTarget(sink, Rect(pill.x, y, pill.w, kHeaderH), header.pillId);
    limit = pill.x - 10;
  }
  if (!header.trailing.isEmpty()) {
    const int width = std::min(DisplayManager::nanoTextWidth(header.trailing, 1), (limit - x) / 3);
    d.nanoLabel(Rect(limit - width, y, width, kHeaderH), header.trailing, 1, Role::Muted, Align::End);
    limit -= width + 10;
  }
  d.nanoLabel(Rect(x, y, std::max(0, limit - x), kHeaderH), header.title, 2, Role::Foreground);
  return y + kHeaderH + 6;
}

// ─── Tile grid ──────────────────────────────────────────────────────────────

void paintTileGrid(DisplayManager &d, Sink &sink, const Rect &area, const std::vector<Tile> &tiles, int columns,
                   int rows) {
  columns = std::max(1, columns);
  rows = std::max(1, rows);
  const int cellW = (area.w - kGap * (columns - 1)) / columns;
  const int cellH = (area.h - kGap * (rows - 1)) / rows;
  const size_t count = std::min(tiles.size(), static_cast<size_t>(columns * rows));
  for (size_t i = 0; i < count; ++i) {
    const int column = static_cast<int>(i) % columns;
    const int row = static_cast<int>(i) / columns;
    const int x = area.x + column * (cellW + kGap);
    const int y = area.y + row * (cellH + kGap);
    const int w = column == columns - 1 ? right(area) - x : cellW;
    const int h = row == rows - 1 ? bottom(area) - y : cellH;
    const Tile &tile = tiles[i];
    const Rect rect(x, y, w, h);
    d.nanoTile(rect, tile.label, tile.icon, tile.detail, sink.pressed(tile.id), tile.accent || sink.armed(tile.id),
               tile.enabled);
    if (tile.enabled) {
      addTarget(sink, rect, tile.id);
    }
  }
}

// ─── Czytaj ─────────────────────────────────────────────────────────────────

void paintReadHome(DisplayManager &d, Sink &sink, const ReadHome &view) {
  const Rect content = tabContent();
  constexpr int kCardH = 78;
  constexpr int kFontsW = 60;
  const Rect card(content.x, content.y, content.w - kFontsW - kGap, kCardH);
  const bool cardPressed = sink.pressed(view.resumeId);
  const uint16_t surface = d.nanoColor(cardPressed ? Role::SurfaceActive : Role::SurfaceMuted);
  d.nanoFillRoundRect(card.x, card.y, card.w, card.h, 10, surface);
  addTarget(sink, card, view.resumeId);

  // Round play button on the right edge of the card.
  const int playR = 19;
  const int playCx = right(card) - 14 - playR;
  const int playCy = card.y + 30;
  if (view.hasBook) {
    d.nanoFillCircle(playCx, playCy, playR, d.nanoColor(Role::Accent));
    d.nanoIcon(Rect(playCx - playR + 2, playCy - playR, playR * 2, playR * 2), Icon::Play, d.nanoColor(Role::OnAccent),
               d.nanoColor(Role::Accent));
  }

  if (view.hasBook) {
    // Cover: a colored book with its initials.
    const Rect cover(card.x + 10, card.y + 10, 46, kCardH - 20);
    d.nanoFillRoundRect(cover.x, cover.y, cover.w, cover.h, 6, view.coverColor);
    d.nanoFillRect(cover.x + 6, cover.y, 2, cover.h, d.nanoBlend(Role::Background, 90));
    d.nanoText(Rect(cover.x + 8, cover.y, cover.w - 8, cover.h), view.coverInitials, 2, 0xFFFF, Align::Center);

    const int textX = right(cover) + 12;
    const int textRight = playCx - playR - 12;
    d.nanoLabel(Rect(textX, card.y + 8, textRight - textX, 24), view.title, 2, Role::Foreground);
    d.nanoLabel(Rect(textX, card.y + 32, textRight - textX, 18), view.author, 1, Role::Muted);
    // Progress row along the bottom of the card.
    const int barY = card.y + kCardH - 18;
    const int labelW = 48;
    d.nanoProgress(Rect(textX, barY + 5, right(card) - 14 - labelW - 8 - textX, 5), view.progressPercent, 0, 100);
    d.nanoLabel(Rect(right(card) - 14 - labelW, barY - 3, labelW, 20), view.progressLabel, 1, Role::Accent,
                Align::End);
  } else {
    d.nanoIcon(Rect(card.x + 12, card.y, 30, card.h), Icon::Books, d.nanoColor(Role::Accent), surface);
    const int textX = card.x + 54;
    d.nanoLabel(Rect(textX, card.y + 16, right(card) - 14 - textX, 26), view.title, 2, Role::Foreground);
    d.nanoLabel(Rect(textX, card.y + 42, right(card) - 14 - textX, 20), view.hint, 1, Role::Muted);
  }

  const Rect fonts(right(card) + kGap, content.y, kFontsW, kCardH);
  const uint16_t fontsSurface = d.nanoColor(sink.pressed(view.fontsId) ? Role::SurfaceActive : Role::SurfaceMuted);
  d.nanoFillRoundRect(fonts.x, fonts.y, fonts.w, fonts.h, 10, fontsSurface);
  // "Aa" set large in the UI font: the font picker's own face.
  d.nanoText(Rect(fonts.x, fonts.y, fonts.w, fonts.h), "Aa", 3, d.nanoColor(Role::Foreground), Align::Center);
  addTarget(sink, fonts, view.fontsId);

  const Rect row(content.x, content.y + kCardH + kGap, content.w, content.h - kCardH - kGap);
  paintTileGrid(d, sink, row, view.tiles, std::max<int>(1, static_cast<int>(view.tiles.size())), 1);
}

// ─── Sections ───────────────────────────────────────────────────────────────

void paintSections(DisplayManager &d, Sink &sink, const Rect &area, const std::vector<Section> &sections) {
  constexpr int kLabelH = 16;
  int labelCount = 0;
  int rowCount = 0;
  for (const Section &section : sections) {
    labelCount += section.title.isEmpty() ? 0 : 1;
    int column = 0;
    for (const SectionItem &item : section.items) {
      if (item.fullWidth || column == 2) {
        if (column != 0) ++rowCount;
        column = 0;
      }
      if (item.fullWidth) {
        ++rowCount;
        continue;
      }
      ++column;
    }
    if (column != 0) ++rowCount;
  }
  if (rowCount == 0) {
    return;
  }
  const int free = area.h - labelCount * kLabelH - (rowCount - 1) * 6;
  const int rowH = std::max(28, std::min(40, free / rowCount));
  const int half = (area.w - kGap) / 2;
  int y = area.y;
  for (const Section &section : sections) {
    if (!section.title.isEmpty()) {
      d.nanoSeparator(Rect(area.x, y, area.w, kLabelH - 2), section.title);
      y += kLabelH;
    }
    int column = 0;
    for (size_t i = 0; i < section.items.size(); ++i) {
      const SectionItem &item = section.items[i];
      if ((item.fullWidth || column == 2) && column != 0) {
        y += rowH + 6;
        column = 0;
      }
      const bool alone = item.fullWidth;
      // A switch row next to a plain button gets the larger share: its
      // label ("Tryb zaawansowany") is the long one.
      const SectionItem *pair = column == 0 && i + 1 < section.items.size() ? &section.items[i + 1] : nullptr;
      const SectionItem *prev = column == 1 && i > 0 ? &section.items[i - 1] : nullptr;
      int split = half;
      if (pair != nullptr && !pair->fullWidth && item.toggle && !pair->toggle) {
        split = (area.w - kGap) * 3 / 5;
      } else if (prev != nullptr && prev->toggle && !item.toggle) {
        split = (area.w - kGap) * 3 / 5;
      }
      const int x = column == 0 ? area.x : area.x + split + kGap;
      const int w = alone ? area.w : (column == 0 ? split : right(area) - x);
      const Rect rect = withHelpButton(d, sink, Rect(x, y, w, rowH), item.helpId);
      if (item.toggle) {
        d.nanoToggle(rect, item.label, item.on, sink.pressed(item.id));
      } else {
        d.nanoButton(rect, item.label, true, item.icon, 1, "", "", sink.pressed(item.id));
      }
      addTarget(sink, rect, item.id);
      if (alone) {
        y += rowH + 6;
        column = 0;
      } else {
        ++column;
      }
    }
    if (column != 0) {
      y += rowH + 6;
    }
  }
}

// ─── Motywy ─────────────────────────────────────────────────────────────────

void paintThemes(DisplayManager &d, Sink &sink, const ThemesView &view) {
  const Rect content = tabContent();
  // Segmented control.
  constexpr int count = ThemesView::kSections;
  const Rect segments(content.x, content.y, content.w, 30);
  d.nanoFillRoundRect(segments.x, segments.y, segments.w, segments.h, 15, d.nanoColor(Role::SurfaceMuted));
  const int segmentW = segments.w / count;
  for (int i = 0; i < count; ++i) {
    const int x = segments.x + i * segmentW;
    const Rect rect(x, segments.y, i == count - 1 ? right(segments) - x : segmentW, segments.h);
    const bool active = view.section == i;
    if (active) {
      d.nanoFillRoundRect(rect.x + 3, rect.y + 3, rect.w - 6, rect.h - 6, 12, d.nanoColor(Role::Accent));
    } else if (sink.pressed(view.segmentIds[i])) {
      d.nanoFillRoundRect(rect.x + 3, rect.y + 3, rect.w - 6, rect.h - 6, 12, d.nanoColor(Role::SurfaceActive));
    }
    const uint8_t size = DisplayManager::nanoTextWidth(view.segmentLabels[i], 2) <= static_cast<int>(rect.w) - 12 ? 2 : 1;
    d.nanoText(rect, view.segmentLabels[i], size, d.nanoColor(active ? Role::OnAccent : Role::Foreground),
               Align::Center);
    addTarget(sink, rect, view.segmentIds[i]);
  }

  const Rect body(content.x, content.y + 38, content.w, content.h - 38);
  if (view.section == 0) {
    constexpr int kColumns = 5;
    constexpr int kRows = 3;
    const int cellW = (body.w - 6 * (kColumns - 1)) / kColumns;
    const int cellH = (body.h - 6 * (kRows - 1)) / kRows;
    size_t cell = 0;
    auto cellRect = [&](size_t index) {
      const int column = static_cast<int>(index) % kColumns;
      const int row = static_cast<int>(index) / kColumns;
      const int x = body.x + column * (cellW + 6);
      return Rect(x, body.y + row * (cellH + 6), column == kColumns - 1 ? right(body) - x : cellW, cellH);
    };
    for (const auto &chip : view.palettes) {
      if (cell >= static_cast<size_t>(kColumns * kRows)) break;
      const Rect rect = cellRect(cell++);
      d.nanoPaletteChip(rect, chip.palette, chip.name, chip.selected, sink.pressed(chip.id));
      addTarget(sink, rect, chip.id);
    }
    if (view.ownAccentId != kNoTarget && cell < static_cast<size_t>(kColumns * kRows)) {
      const Rect rect = cellRect(cell);
      d.nanoPill(rect, view.ownAccentLabel, view.ownAccentOn ? Icon::Check : Icon::None, sink.pressed(view.ownAccentId),
                 view.ownAccentOn);
      addTarget(sink, rect, view.ownAccentId);
    }
  } else if (view.section == 1) {
    const int hintH = view.readingHint.isEmpty() ? 0 : 18;
    const Rect row(body.x, body.y, body.w, body.h - hintH);
    const int themes = static_cast<int>(view.readingThemes.size());
    const int colorW = 124;
    const int themesW = row.w - (view.letterColorId != kNoTarget ? colorW + kGap : 0);
    const int cellW = themes > 0 ? (themesW - kGap * (themes - 1)) / themes : themesW;
    for (int i = 0; i < themes; ++i) {
      const auto &chip = view.readingThemes[static_cast<size_t>(i)];
      const int x = row.x + i * (cellW + kGap);
      const Rect rect(x, row.y, i == themes - 1 ? row.x + themesW - x : cellW, row.h);
      d.nanoReadingThemeChip(rect, chip.theme, chip.name, chip.selected, sink.pressed(chip.id));
      addTarget(sink, rect, chip.id);
    }
    if (view.letterColorId != kNoTarget) {
      const Rect rect(right(row) - colorW, row.y, colorW, row.h);
      const bool pressed = sink.pressed(view.letterColorId);
      d.nanoFillRoundRect(rect.x, rect.y, rect.w, rect.h, 8,
                          d.nanoColor(pressed ? Role::SurfaceActive : Role::SurfaceMuted));
      const int cy = rect.y + rect.h / 2 - 14;
      d.nanoFillCircle(rect.x + rect.w / 2, cy, 13, view.letterColor);
      d.nanoText(Rect(rect.x + 6, cy + 17, rect.w - 12, 20), view.letterColorLabel, 2, d.nanoColor(Role::Foreground),
                 Align::Center);
      d.nanoText(Rect(rect.x + 6, cy + 35, rect.w - 12, 16), view.letterColorName, 1, d.nanoColor(Role::Muted),
                 Align::Center);
      addTarget(sink, rect, view.letterColorId);
    }
    if (hintH > 0) {
      d.nanoLabel(Rect(body.x, bottom(body) - hintH + 2, body.w, hintH - 2), view.readingHint, 1, Role::Muted,
                  Align::Center);
    }
  } else if (view.section == 2) {
    constexpr int kColumns = 4;
    constexpr int kRows = 2;
    const int cellW = (body.w - kGap * (kColumns - 1)) / kColumns;
    const int cellH = (body.h - kGap * (kRows - 1)) / kRows;
    for (size_t i = 0; i < view.fonts.size() && i < static_cast<size_t>(kColumns * kRows); ++i) {
      const int column = static_cast<int>(i) % kColumns;
      const int row = static_cast<int>(i) / kColumns;
      const int x = body.x + column * (cellW + kGap);
      const Rect rect(x, body.y + row * (cellH + kGap), column == kColumns - 1 ? right(body) - x : cellW, cellH);
      const auto &chip = view.fonts[i];
      d.nanoFontChip(rect, chip.family, chip.name, chip.sample, chip.selected, sink.pressed(chip.id));
      addTarget(sink, rect, chip.id);
    }
  } else {
    const int hintH = view.layoutHint.isEmpty() ? 0 : 18;
    const Rect row(body.x, body.y, body.w, body.h - hintH);
    const int n = static_cast<int>(view.layouts.size());
    const int cellW = n > 0 ? (row.w - kGap * (n - 1)) / n : row.w;
    for (int i = 0; i < n; ++i) {
      const int x = row.x + i * (cellW + kGap);
      const Rect rect(x, row.y, i == n - 1 ? right(row) - x : cellW, row.h);
      const auto &chip = view.layouts[static_cast<size_t>(i)];
      d.nanoLayoutChip(rect, chip.compact, chip.railRight, chip.name, chip.detail, chip.selected,
                       sink.pressed(chip.id));
      addTarget(sink, rect, chip.id);
    }
    if (hintH > 0) {
      d.nanoLabel(Rect(body.x, bottom(body) - hintH + 2, body.w, hintH - 2), view.layoutHint, 1, Role::Muted,
                  Align::Center);
    }
  }
}

// ─── Biblioteka ─────────────────────────────────────────────────────────────

namespace {
constexpr int kShelfDetailHeight = 38;
constexpr int kShelfGap = 5;
constexpr int kShelfSpineBaseWidth = 29;
constexpr int kShelfSpineWidthStep = 2;
constexpr int kShelfSpinePeriod = 4;
constexpr int kShelfCycleWidth = kShelfSpinePeriod * (kShelfSpineBaseWidth + kShelfGap) +
                                 kShelfSpineWidthStep * kShelfSpinePeriod * (kShelfSpinePeriod - 1) / 2;

uint16_t shelfSpineColor(size_t index) {
  constexpr uint16_t kColors[] = {0x99E3, 0x1AF5, 0x0B6A, 0x7B98, 0x4490, 0xB4CD, 0x9A49, 0x32FA};
  return kColors[index % 8];
}
}  // namespace

int shelfSpineWidth(size_t index) {
  return kShelfSpineBaseWidth + kShelfSpineWidthStep * static_cast<int>(index % kShelfSpinePeriod);
}

int32_t shelfSpineLeft(size_t index) {
  const int32_t within = static_cast<int32_t>(index % kShelfSpinePeriod);
  return static_cast<int32_t>(index / kShelfSpinePeriod) * kShelfCycleWidth +
         within * (kShelfSpineBaseWidth + kShelfGap) + kShelfSpineWidthStep * within * (within - 1) / 2;
}

size_t shelfSpineIndexAt(int32_t contentX, size_t count) {
  if (count == 0 || contentX <= 0) {
    return 0;
  }
  const size_t cycle = static_cast<size_t>(contentX / kShelfCycleWidth);
  const int32_t within = contentX % kShelfCycleWidth;
  const size_t offset = within >= shelfSpineLeft(3)   ? 3
                        : within >= shelfSpineLeft(2) ? 2
                        : within >= shelfSpineLeft(1) ? 1
                                                      : 0;
  return std::min(count - 1, cycle * kShelfSpinePeriod + offset);
}

ShelfGeometry shelfGeometry() {
  const Rect content = tabContent();
  ShelfGeometry g;
  g.header = Rect(content.x, content.y, content.w, kHeaderH);
  const int detailY = bottom(content) - kShelfDetailHeight;
  const int viewportY = bottom(g.header) + 6;
  g.viewport = Rect(content.x, viewportY, content.w, detailY - 6 - viewportY);
  g.detail = Rect(content.x, detailY, content.w, kShelfDetailHeight);
  g.marker = g.viewport.x + g.viewport.w / 2;
  return g;
}

int32_t shelfClampOffset(size_t count, int32_t offset, int viewportWidth) {
  if (count == 0) {
    return 0;
  }
  const size_t last = count - 1;
  const int32_t lastCenter = shelfSpineLeft(last) + shelfSpineWidth(last) / 2;
  const int32_t firstCenter = shelfSpineWidth(0) / 2;
  return std::max<int32_t>(viewportWidth / 2 - lastCenter, std::min<int32_t>(offset, viewportWidth / 2 - firstCenter));
}

int32_t shelfCenteredOffset(size_t count, size_t index, int viewportWidth) {
  if (count == 0) {
    return 0;
  }
  index = std::min(index, count - 1);
  return shelfClampOffset(count, viewportWidth / 2 - shelfSpineLeft(index) - shelfSpineWidth(index) / 2,
                          viewportWidth);
}

size_t shelfNearest(size_t count, int32_t offset, int markerX, int viewportX) {
  if (count == 0) {
    return 0;
  }
  const int32_t contentX = markerX - viewportX - offset;
  const size_t candidate = shelfSpineIndexAt(contentX, count);
  const size_t first = candidate == 0 ? 0 : candidate - 1;
  const size_t last = std::min(count - 1, candidate + 1);
  size_t best = first;
  int32_t bestDistance = INT32_MAX;
  for (size_t i = first; i <= last; ++i) {
    const int32_t center = viewportX + shelfSpineLeft(i) + shelfSpineWidth(i) / 2 + offset;
    const int32_t distance = std::abs(center - markerX);
    if (distance < bestDistance) {
      bestDistance = distance;
      best = i;
    }
  }
  return best;
}

void paintShelf(DisplayManager &d, Sink &sink, const ShelfView &view) {
  const ShelfGeometry g = shelfGeometry();
  paintHeader(d, sink, g.header, view.header);
  const size_t count = view.books.size();
  if (count == 0) {
    d.nanoLabel(Rect(g.viewport.x, g.viewport.y, g.viewport.w, bottom(g.detail) - g.viewport.y), view.emptyLabel, 2,
                Role::Muted, Align::Center, 3);
    return;
  }
  const size_t selected = std::min(view.selected, count - 1);
  const uint16_t accent = d.nanoColor(Role::Accent);
  const uint16_t floor = d.nanoColor(Role::ProgressTrack);
  d.nanoSetClip(g.viewport.x, g.viewport.y - 8, g.viewport.w, g.viewport.h + 11);
  const int32_t contentLeft = -view.offset;
  const size_t firstVisible = shelfSpineIndexAt(contentLeft, count);
  const size_t lastVisible = shelfSpineIndexAt(contentLeft + g.viewport.w, count);
  const int maxHeight = g.viewport.h;
  for (size_t i = firstVisible; i <= lastVisible && i < count; ++i) {
    const ShelfBook &book = view.books[i];
    const int width = shelfSpineWidth(i);
    const int variation = static_cast<int>((i * 7 + book.title.length() * 3) % 19);
    const int height = std::min(maxHeight - 8, maxHeight - 26 + variation);
    const int x = g.viewport.x + static_cast<int>(shelfSpineLeft(i) + view.offset);
    const bool active = i == selected;
    const int y = bottom(g.viewport) - height - (active ? 6 : 0);
    const uint16_t fill = shelfSpineColor(i);
    d.nanoFillRoundRect(x, y, width, height, 3, fill);
    d.nanoFillRect(x + 3, y + 5, width - 6, 1, d.nanoBlend(Role::Background, 60));
    d.nanoFillRect(x + 3, y + height - 6, width - 6, 1, d.nanoBlend(Role::Background, 60));
    if (active) {
      d.nanoFillRoundRect(x, y - 5, width, 3, 1, accent);
    }
    if (book.progress > 0) {
      // Bookmark ribbon hanging from the top, as long as the progress.
      const int ribbonX = x + width - 9;
      const int ribbonHeight = std::max(8, (height - 8) * book.progress / 100);
      d.nanoFillRect(ribbonX, y, 5, ribbonHeight, 0xDACA);
      d.nanoFillTriangle(ribbonX, y + ribbonHeight, ribbonX + 4, y + ribbonHeight, ribbonX + 2, y + ribbonHeight - 3,
                         fill);
    }
    // Spine lettering: up to 6 capitals, top to bottom.
    String title = book.title;
    String lower = title;
    lower.toLowerCase();
    if (lower.startsWith("the ")) {
      title = title.substring(4);
    }
    int letterY = y + 10;
    int written = 0;
    for (size_t c = 0; c < title.length() && written < 6 && letterY + 10 < y + height - 6; ++c) {
      char letter = title[c];
      if (letter >= 'a' && letter <= 'z') {
        letter = static_cast<char>(letter - 'a' + 'A');
      }
      const uint8_t value = static_cast<uint8_t>(letter);
      if ((letter >= 'A' && letter <= 'Z') || (letter >= '0' && letter <= '9') || value >= 0x80) {
        d.nanoText(Rect(x, letterY, width - (book.progress > 0 ? 6 : 0), 10), String(letter), 1, 0xFFFF,
                   Align::Center);
        letterY += 11;
        ++written;
      }
    }
  }
  d.nanoFillRoundRect(g.viewport.x, bottom(g.viewport), g.viewport.w, 3, 1, floor);
  d.nanoResetClip();

  // Detail strip: title + author on the left, progress on the right.
  const int percentW = 64;
  const int textW = g.detail.w - percentW - 12;
  d.nanoLabel(Rect(g.detail.x, g.detail.y, textW, 20), view.detailTitle, 2, Role::Foreground);
  String second = view.detailAuthor;
  if (!view.detailStatus.isEmpty()) {
    second += "  -  " + view.detailStatus;
  }
  d.nanoLabel(Rect(g.detail.x, g.detail.y + 20, textW, 17), second, 1, Role::Muted);
  d.nanoLabel(Rect(right(g.detail) - percentW, g.detail.y, percentW, g.detail.h), view.detailPercent, 3, Role::Accent,
              Align::End);
}

// ─── Rozdzialy ──────────────────────────────────────────────────────────────

Rect wheelViewport() {
  const Rect content = tabContent();
  return Rect(content.x, content.y + kHeaderH + 6, content.w, content.h - kHeaderH - 6);
}

int wheelRowCenter(const Rect &viewport, int row, int offset) {
  const int raw = row * kWheelRowStep + offset;
  const int magnitude = std::min(std::abs(raw), static_cast<int>(viewport.h));
  return viewport.y + viewport.h / 2 + raw * (2 * viewport.h - magnitude) / (2 * viewport.h);
}

int wheelRowHeight(bool centered) { return centered ? 30 : 20; }

bool wheelRowVisible(const Rect &viewport, int y, int height) {
  return y - height / 2 >= viewport.y && y + height / 2 <= viewport.y + viewport.h;
}

void paintWheel(DisplayManager &d, Sink &sink, const WheelView &view) {
  const Rect content = tabContent();
  paintHeader(d, sink, Rect(content.x, content.y, content.w, kHeaderH), view.header);
  const Rect viewport = wheelViewport();
  if (view.count == 0) {
    d.nanoButton(viewport, view.emptyLabel, true, Icon::Play, 1, "", "", sink.pressed(view.emptyId));
    addTarget(sink, viewport, view.emptyId);
    return;
  }
  const int centerY = viewport.y + viewport.h / 2;
  const int halfHeight = std::max(1, viewport.h / 2);
  const int maximumWidth = viewport.w;
  d.nanoSetClip(viewport.x, viewport.y, viewport.w, viewport.h);
  for (size_t local = 0; local < view.titles.size(); ++local) {
    const size_t i = view.firstIndex + local;
    const int y = wheelRowCenter(viewport, static_cast<int>(i) - static_cast<int>(view.centered), view.offset);
    const int curved = y - centerY;
    const bool centered = i == view.centered;
    const uint8_t alpha =
        centered ? 255 : static_cast<uint8_t>(std::max(40, 210 - std::abs(curved) * 170 / halfHeight));
    const int height = wheelRowHeight(centered);
    if (!wheelRowVisible(viewport, y, height)) {
      continue;
    }
    const int width = centered ? maximumWidth
                               : maximumWidth - std::min(std::abs(curved), halfHeight) * (maximumWidth / 4) / halfHeight;
    const int x = viewport.x + (viewport.w - width) / 2;
    const int top = y - height / 2;
    if (centered) {
      d.nanoFillRoundRect(x, top, width, height, 8, d.nanoColor(Role::SurfaceActive));
      d.nanoFillRoundRect(x + 5, top + 6, 3, height - 12, 1, d.nanoColor(Role::Accent));
    } else {
      d.nanoFillRoundRect(x, top, width, height, 6, d.nanoBlend(Role::SurfaceMuted, alpha));
    }
    if (i == view.readingIndex) {
      d.nanoFillCircle(x + width - 14, y, 4, centered ? d.nanoColor(Role::Accent) : d.nanoBlend(Role::Accent, alpha));
    }
    const String &title = view.titles[local];
    d.nanoText(Rect(x + 16, top, width - 40, height), title, centered ? 2 : 1, d.nanoBlend(Role::Foreground, alpha),
               centered ? Align::Start : Align::Center);
  }
  d.nanoResetClip();
}

// ─── Lists ──────────────────────────────────────────────────────────────────

void paintList(DisplayManager &d, Sink &sink, const ListView &view) {
  const Rect area = view.fullScreen ? fullContent() : tabContent();
  const int top = paintHeader(d, sink, Rect(area.x, area.y, area.w, kHeaderH), view.header);
  const Rect grid(area.x, top, area.w, bottom(area) - top);
  const int columns = std::max(1, view.columns);
  const int rows = std::max(1, view.rows);
  const int gap = 6;
  const int cellW = (grid.w - kGap * (columns - 1)) / columns;
  const int cellH = (grid.h - gap * (rows - 1)) / rows;
  int column = 0;
  int row = 0;
  for (const ListItem &item : view.items) {
    const bool wide = item.fullWidth || item.kind == ListItem::Kind::Label || item.kind == ListItem::Kind::Separator ||
                      item.kind == ListItem::Kind::Row || columns == 1;
    if (wide && column != 0) {
      column = 0;
      ++row;
    }
    if (row >= rows) {
      break;
    }
    const int span = item.kind == ListItem::Kind::Label ? std::min(2, rows - row) : 1;
    const int x = grid.x + column * (cellW + kGap);
    const int y = grid.y + row * (cellH + gap);
    const int w = wide ? grid.w : (column == columns - 1 ? right(grid) - x : cellW);
    const int h = span * cellH + (span - 1) * gap;
    const Rect rect = withHelpButton(d, sink, Rect(x, y, w, h), item.helpId);
    const bool pressed = sink.pressed(item.id);
    switch (item.kind) {
      case ListItem::Kind::Separator:
        d.nanoSeparator(Rect(rect.x, rect.y + rect.h / 2 - 7, rect.w, 14), item.label);
        break;
      case ListItem::Kind::Label:
        d.nanoLabel(rect, item.label, 2, Role::Muted, Align::Start, 3);
        break;
      case ListItem::Kind::Setting:
        d.nanoSetting(rect, item.label, item.value, true, pressed);
        addTarget(sink, rect, item.id);
        break;
      case ListItem::Kind::Toggle:
        d.nanoToggle(rect, item.label, item.on, pressed);
        addTarget(sink, rect, item.id);
        break;
      case ListItem::Kind::Slider:
        d.nanoSlider(rect, item.label, item.value, item.sliderValue, item.sliderMin, item.sliderMax, pressed,
                     item.dragging);
        sink.slider(rect, item.id);
        addTarget(sink, rect, item.id);
        break;
      case ListItem::Kind::Row: {
        const int trailingW = item.trailingId == kNoTarget ? 0 : std::min(120, rect.w / 4);
        const Rect name(rect.x, rect.y, rect.w - (trailingW > 0 ? trailingW + kGap : 0), rect.h);
        d.nanoButton(name, item.label, true, item.icon, 1, item.value, "", pressed, item.armed || sink.armed(item.id));
        addTarget(sink, name, item.id);
        if (trailingW > 0) {
          const Rect trailing(right(rect) - trailingW, rect.y, trailingW, rect.h);
          const bool armed = item.trailingArmed || sink.armed(item.trailingId);
          d.nanoButton(trailing, armed ? item.trailingLabel : String(), true, Icon::Trash, 1, "", "",
                       sink.pressed(item.trailingId), armed);
          addTarget(sink, trailing, item.trailingId);
        }
        break;
      }
      case ListItem::Kind::Button:
      default: {
        const bool armed = item.armed || sink.armed(item.id);
        d.nanoButton(rect, item.label, true, item.icon, rect.h >= 48 ? 2 : 1, item.value, "", pressed, armed,
                     item.typeface);
        if (item.marked && !armed) {
          d.nanoFillCircle(right(rect) - 16, rect.y + rect.h / 2, 8, d.nanoColor(Role::Accent));
          d.nanoIcon(Rect(right(rect) - 26, rect.y + rect.h / 2 - 10, 20, 20), Icon::Check, d.nanoColor(Role::OnAccent),
                     d.nanoColor(Role::Accent));
        }
        addTarget(sink, rect, item.id);
        break;
      }
    }
    if (wide) {
      column = 0;
      row += span;
    } else if (++column >= columns) {
      column = 0;
      ++row;
    }
  }
}

// ─── Book details ───────────────────────────────────────────────────────────

void paintBookDetails(DisplayManager &d, Sink &sink, const BookDetailsView &view) {
  const Rect area = fullContent();
  const int top = paintHeader(d, sink, Rect(area.x, area.y, area.w, kHeaderH), view.header);
  const int percentW = 60;
  d.nanoLabel(Rect(area.x, top, area.w - percentW - 10, 18), view.author, 1, Role::Muted);
  d.nanoLabel(Rect(right(area) - percentW, top, percentW, 18), view.percentLabel, 1, Role::Accent, Align::End);
  d.nanoProgress(Rect(area.x, top + 22, area.w, 5), view.percent, 0, 100);
  const int gridY = top + 36;
  const int columns = view.actions.size() > 4 ? 3 : 2;
  paintTileGrid(d, sink, Rect(area.x, gridY, area.w, bottom(area) - gridY), view.actions, columns, 2);
}

// ─── Confirm ────────────────────────────────────────────────────────────────

void paintConfirm(DisplayManager &d, Sink &sink, const ConfirmView &view) {
  const Rect content = fullContent();
  const int width = std::min(480, static_cast<int>(content.w));
  const Rect panel(content.x + (content.w - width) / 2, content.y + 4, width, content.h - 8);
  const int textH = view.detail.isEmpty() ? 64 : 48;
  d.nanoLabel(Rect(panel.x, panel.y, panel.w, textH), view.question, 2, Role::Foreground, Align::Center, 2);
  if (!view.detail.isEmpty()) {
    d.nanoLabel(Rect(panel.x, panel.y + textH, panel.w, 20), view.detail, 1, Role::Muted, Align::Center);
  }
  const int buttonsH = 44;
  const int buttonsY = bottom(panel) - buttonsH;
  int x = panel.x;
  const bool hasBack = view.backId != kNoTarget;
  const int backW = hasBack ? (view.backLabel.isEmpty() ? 56 : 140) : 0;
  const int actionCount = static_cast<int>(view.actions.size());
  const int actionsW = panel.w - (hasBack ? backW + kGap : 0);
  const int actionW = actionCount > 0 ? (actionsW - kGap * (actionCount - 1)) / actionCount : 0;
  if (hasBack) {
    const Rect back(x, buttonsY, backW, buttonsH);
    d.nanoButton(back, view.backLabel, true, Icon::ChevronLeft, 1, "", "", sink.pressed(view.backId));
    addTarget(sink, back, view.backId);
    x += backW + kGap;
  }
  for (int i = 0; i < actionCount; ++i) {
    const auto &action = view.actions[static_cast<size_t>(i)];
    const int w = i == actionCount - 1 ? right(panel) - x : actionW;
    const Rect rect(x, buttonsY, w, buttonsH);
    d.nanoButton(rect, action.label, true, action.danger ? Icon::Trash : Icon::None, 1, "", "", sink.pressed(action.id),
                 action.armed || sink.armed(action.id));
    addTarget(sink, rect, action.id);
    x += w + kGap;
  }
}

// ─── Reader panel ───────────────────────────────────────────────────────────

Rect readerPanelWordArea() { return Rect(0, 24, kScreenW, 96); }

Rect readerPanelBar() { return Rect(0, 128, kScreenW, kScreenH - 128); }

Rect readerPanelStatusArea() { return Rect(0, 0, kScreenW - 100, 22); }

void paintReaderPanel(DisplayManager &d, Sink &sink, const ReaderPanelView &view) {
  // Top line: chapter left, time left + percent right, battery.
  const int statusW = 150;
  d.nanoBatteryInline(Rect(kScreenW - 104, 2, 96, 20), false, Align::End);
  String status = view.progressLabel;
  if (!view.timeLeft.isEmpty()) {
    status = view.timeLeft + "  -  " + status;
  }
  const bool statusPressed = sink.pressed(view.statusId);
  if (statusPressed) {
    d.nanoFillRoundRect(4, 1, kScreenW - 112, 22, 6, d.nanoColor(Role::SurfaceMuted));
  }
  d.nanoLabel(Rect(kScreenW - 112 - statusW, 2, statusW, 20), status, 1, Role::Muted, Align::End);
  d.nanoLabel(Rect(12, 2, kScreenW - 136 - statusW, 20), view.chapter, 1, Role::Muted);
  addTarget(sink, readerPanelStatusArea(), view.statusId);

  const Rect words = readerPanelWordArea();
  if (view.scrollMode) {
    d.nanoScrollPreview(Rect(0, words.y, kScreenW, words.h - (view.hint.isEmpty() ? 0 : 14)), view.words,
                        view.currentLocal);
  } else {
    d.nanoReaderPreview(Rect(0, words.y + 2, kScreenW, words.h - (view.hint.isEmpty() ? 4 : 18)), view.before,
                        view.word, view.after);
  }
  if (!view.hint.isEmpty()) {
    d.nanoLabel(Rect(12, bottom(words) - 16, kScreenW - 24, 16), view.hint, 1, Role::Subtle, Align::Center);
  }
  d.nanoProgress(Rect(12, 122, kScreenW - 24, 3), view.progressPercent, 0, 100);

  // Bottom bar: Menu | chapters | go to | bookmark | << | - WPM + | Czytaj.
  const int y = 132;
  const int h = kScreenH - y - 4;
  constexpr int kSmall = 44;
  int x = 10;
  const Rect menu(x, y, 92, h);
  d.nanoButton(menu, view.menuLabel, true, Icon::None, 1, "", "", sink.pressed(view.menuId));
  addTarget(sink, menu, view.menuId);
  x += menu.w + kGap;
  auto small = [&](int id, Icon icon) {
    if (id == kNoTarget) {
      return;
    }
    const Rect rect(x, y, kSmall, h);
    d.nanoButton(rect, "", true, icon, 1, "", "", sink.pressed(id));
    addTarget(sink, rect, id);
    x += kSmall + kGap;
  };
  small(view.chaptersId, Icon::List);
  small(view.gotoId, Icon::Target);
  const Rect bookmark(x, y, kSmall, h);
  const uint16_t bookmarkSurface = d.nanoColor(sink.pressed(view.bookmarkId) ? Role::SurfaceActive : Role::SurfaceMuted);
  d.nanoFillRoundRect(bookmark.x, bookmark.y, bookmark.w, bookmark.h, 8, bookmarkSurface);
  d.nanoIcon(bookmark, Icon::Bookmark, d.nanoColor(view.bookmarkFilled ? Role::Accent : Role::Foreground),
             bookmarkSurface);
  if (!view.bookmarkFilled) {
    // Hollow ribbon: punch the inside back out.
    const int cx = bookmark.x + bookmark.w / 2;
    const int cy = bookmark.y + bookmark.h / 2;
    d.nanoFillRect(cx - 4, cy - 7, 8, 11, bookmarkSurface);
  }
  addTarget(sink, bookmark, view.bookmarkId);
  x += bookmark.w + kGap;
  small(view.rewindId, Icon::Rewind);

  // WPM stepper: one pill with - and + ends.
  const int stepperW = 164;
  const Rect stepper(x, y, stepperW, h);
  d.nanoFillRoundRect(stepper.x, stepper.y, stepper.w, stepper.h, 8, d.nanoColor(Role::SurfaceMuted));
  const Rect minus(stepper.x, y, 42, h);
  const Rect plus(right(stepper) - 42, y, 42, h);
  const Rect value(right(minus), y, stepper.w - 84, h);
  if (sink.pressed(view.minusId)) {
    d.nanoFillRoundRect(minus.x, minus.y, minus.w, minus.h, 8, d.nanoColor(Role::SurfaceActive));
  }
  if (sink.pressed(view.plusId)) {
    d.nanoFillRoundRect(plus.x, plus.y, plus.w, plus.h, 8, d.nanoColor(Role::SurfaceActive));
  }
  if (sink.pressed(view.wpmId)) {
    d.nanoFillRect(value.x, value.y, value.w, value.h, d.nanoColor(Role::SurfaceActive));
  }
  d.nanoIcon(minus, Icon::Minus, d.nanoColor(Role::Foreground), d.nanoColor(Role::SurfaceMuted));
  d.nanoIcon(plus, Icon::Plus, d.nanoColor(Role::Foreground), d.nanoColor(Role::SurfaceMuted));
  d.nanoLabel(value, view.wpmLabel, 2, Role::Foreground, Align::Center);
  addTarget(sink, minus, view.minusId);
  addTarget(sink, plus, view.plusId);
  addTarget(sink, value, view.wpmId);
  x += stepperW + kGap;

  const Rect start(x, y, kScreenW - 10 - x, h);
  d.nanoButton(start, view.startLabel, true, Icon::Play, 1, "", "", sink.pressed(view.startId), true);
  addTarget(sink, start, view.startId);
}

// ─── Przejdz do ─────────────────────────────────────────────────────────────

namespace {
void paintSegments(DisplayManager &d, Sink &sink, const Rect &segments, const int ids[3], const String labels[3],
                   int active) {
  d.nanoFillRoundRect(segments.x, segments.y, segments.w, segments.h, segments.h / 2, d.nanoColor(Role::SurfaceMuted));
  const int segmentW = segments.w / 3;
  for (int i = 0; i < 3; ++i) {
    const int x = segments.x + i * segmentW;
    const Rect rect(x, segments.y, i == 2 ? right(segments) - x : segmentW, segments.h);
    const bool on = active == i;
    if (on) {
      d.nanoFillRoundRect(rect.x + 3, rect.y + 3, rect.w - 6, rect.h - 6, (rect.h - 6) / 2, d.nanoColor(Role::Accent));
    } else if (sink.pressed(ids[i])) {
      d.nanoFillRoundRect(rect.x + 3, rect.y + 3, rect.w - 6, rect.h - 6, (rect.h - 6) / 2,
                          d.nanoColor(Role::SurfaceActive));
    }
    d.nanoText(rect, labels[i], 1, d.nanoColor(on ? Role::OnAccent : Role::Foreground), Align::Center);
    addTarget(sink, rect, ids[i]);
  }
}
}  // namespace

Rect goToBarRect() { return Rect(kMargin + 4, 92, kScreenW - (kMargin + 4) * 2, 26); }

void paintGoTo(DisplayManager &d, Sink &sink, const GoToView &view) {
  const Rect area = fullContent();
  Header header = view.header;
  paintHeader(d, sink, Rect(area.x, area.y, area.w - 318, kHeaderH), header);
  paintSegments(d, sink, Rect(right(area) - 308, area.y, 308, kHeaderH), view.segmentIds, view.segmentLabels,
                view.segment);

  // Big readout + the chapter it lands in.
  const int valueW = std::min(300, DisplayManager::nanoTextWidth(view.value, 3) + 8);
  d.nanoText(Rect(area.x + 4, 46, valueW, 36), view.value, 3, d.nanoColor(Role::Foreground));
  d.nanoLabel(Rect(area.x + 4 + valueW + 14, 46, area.w - valueW - 22, 20), view.detail, 2, Role::Accent);
  d.nanoLabel(Rect(area.x + 4 + valueW + 14, 66, area.w - valueW - 22, 16), view.hint, 1, Role::Muted);

  // Bar: tap or drag anywhere on it.
  const Rect bar = goToBarRect();
  const int range = std::max(1, view.sliderMax - view.sliderMin);
  const int value = std::max(view.sliderMin, std::min(view.sliderValue, view.sliderMax));
  const int fill = bar.h + (bar.w - bar.h) * (value - view.sliderMin) / range;
  d.nanoFillRoundRect(bar.x, bar.y, bar.w, bar.h, bar.h / 2, d.nanoColor(Role::SurfaceMuted));
  d.nanoFillRoundRect(bar.x, bar.y, fill, bar.h, bar.h / 2, d.nanoColor(Role::Accent));
  d.nanoFillCircle(bar.x + fill - bar.h / 2, bar.y + bar.h / 2, bar.h / 2 - (view.dragging ? 2 : 4),
                   d.nanoColor(Role::OnAccent));

  // Fine steps and the action.
  const int y = 128;
  const int h = kScreenH - y - 6;
  const Rect minus(area.x, y, 64, h);
  const Rect plus(area.x + 64 + kGap, y, 64, h);
  d.nanoButton(minus, "", true, Icon::Minus, 1, "", "", sink.pressed(view.minusId));
  d.nanoButton(plus, "", true, Icon::Plus, 1, "", "", sink.pressed(view.plusId));
  addTarget(sink, minus, view.minusId);
  addTarget(sink, plus, view.plusId);
  const Rect read(right(area) - 220, y, 220, h);
  d.nanoButton(read, view.readLabel, true, Icon::Play, 1, "", "", sink.pressed(view.readId), true);
  addTarget(sink, read, view.readId);
}

// ─── Choice ─────────────────────────────────────────────────────────────────

void paintChoice(DisplayManager &d, Sink &sink, const ChoiceView &view) {
  const Rect area = fullContent();
  int top = area.y;
  if (view.header.backId != kNoTarget || !view.header.title.isEmpty()) {
    top = paintHeader(d, sink, Rect(area.x, area.y, area.w, kHeaderH), view.header);
  }
  if (!view.question.isEmpty()) {
    d.nanoLabel(Rect(area.x, top, area.w, 26), view.question, 2, Role::Muted, Align::Center);
    top += 32;
  }
  const int count = std::max<int>(1, static_cast<int>(view.options.size()));
  const int cellW = (area.w - kGap * (count - 1)) / count;
  for (int i = 0; i < count && i < static_cast<int>(view.options.size()); ++i) {
    const auto &option = view.options[static_cast<size_t>(i)];
    const int x = area.x + i * (cellW + kGap);
    const Rect rect(x, top, i == count - 1 ? right(area) - x : cellW, bottom(area) - top);
    d.nanoTile(rect, option.label, option.icon, option.detail, sink.pressed(option.id), option.accent);
    addTarget(sink, rect, option.id);
  }
}

// ─── Help page ──────────────────────────────────────────────────────────────

Rect helpBodyRect() {
  const Rect area = fullContent();
  return Rect(area.x + 6, area.y + kHeaderH + 8, area.w - 22, area.h - kHeaderH - 8);
}

int helpTextWidth() { return helpBodyRect().w; }

int helpContentHeight(const HelpView &view) {
  const int lineH = DisplayManager::nanoLineHeight(2);
  int height = 0;
  for (const String &line : view.lines) {
    height += line.isEmpty() ? lineH / 2 : lineH;
  }
  return height;
}

void paintHelp(DisplayManager &d, Sink &sink, const HelpView &view) {
  const Rect area = fullContent();
  paintHeader(d, sink, Rect(area.x, area.y, area.w, kHeaderH), view.header);
  const Rect body = helpBodyRect();
  const int lineH = DisplayManager::nanoLineHeight(2);
  d.nanoSetClip(body.x, body.y, body.w, body.h);
  int y = body.y - view.scroll;
  for (const String &line : view.lines) {
    const int h = line.isEmpty() ? lineH / 2 : lineH;
    if (y + h > body.y && y < bottom(body) && !line.isEmpty()) {
      d.nanoText(Rect(body.x, y, body.w, lineH), line, 2, d.nanoColor(Role::Foreground));
    }
    y += h;
  }
  d.nanoResetClip();
  // Scroll bar: where the visible part sits in the whole text.
  const int total = helpContentHeight(view);
  if (total > body.h) {
    const int trackX = right(area) - 6;
    d.nanoFillRoundRect(trackX, body.y, 4, body.h, 2, d.nanoColor(Role::SurfaceMuted));
    const int thumbH = std::max(16, body.h * body.h / total);
    const int thumbY = body.y + (body.h - thumbH) * view.scroll / std::max(1, total - body.h);
    d.nanoFillRoundRect(trackX, thumbY, 4, thumbH, 2, d.nanoColor(Role::Accent));
  }
}

// ─── Kolor litery ───────────────────────────────────────────────────────────

void paintColorPicker(DisplayManager &d, Sink &sink, const ColorPickerView &view) {
  const Rect area = fullContent();
  // Header with a live preview of the choice on the right, in the reading
  // colors.
  const int previewW = 176;
  paintHeader(d, sink, Rect(area.x, area.y, area.w - previewW - kGap, kHeaderH), view.header);
  const Rect preview(right(area) - previewW, area.y, previewW, kHeaderH);
  d.nanoFillRoundRect(preview.x, preview.y, preview.w, preview.h, 8, view.previewBackground);
  const String left = "prze";
  const String mid = "c";
  const String rest = "zytam";
  constexpr uint8_t kScale = 48;
  const int wl = d.nanoTypefaceTextWidth(left, kScale);
  const int wm = d.nanoTypefaceTextWidth(mid, kScale);
  const int wr = d.nanoTypefaceTextWidth(rest, kScale);
  const int tx = preview.x + (preview.w - wl - wm - wr) / 2;
  const int ty = preview.y + 1;
  d.nanoSetClip(preview.x, preview.y, preview.w, preview.h);
  d.nanoTypefaceText(tx, ty, left, view.previewWord, kScale);
  d.nanoTypefaceText(tx + wl, ty, mid, view.previewFocus, kScale);
  d.nanoTypefaceText(tx + wl + wm, ty, rest, view.previewWord, kScale);
  d.nanoResetClip();

  const Rect grid(area.x, area.y + kHeaderH + 8, area.w, area.h - kHeaderH - 8);
  const int columns = std::max(1, view.columns);
  const int rows = std::max(1, view.rows);
  const int gap = 5;
  const int cellW = (grid.w - gap * (columns - 1)) / columns;
  const int cellH = (grid.h - gap * (rows - 1)) / rows;
  for (size_t i = 0; i < view.swatches.size() && i < static_cast<size_t>(columns * rows); ++i) {
    const int column = static_cast<int>(i) % columns;
    const int row = static_cast<int>(i) / columns;
    const int x = grid.x + column * (cellW + gap);
    const Rect rect(x, grid.y + row * (cellH + gap), column == columns - 1 ? right(grid) - x : cellW, cellH);
    const auto &swatch = view.swatches[i];
    d.nanoColorSwatch(rect, swatch.color, swatch.selected, sink.pressed(swatch.id));
    addTarget(sink, rect, swatch.id);
  }
}

}  // namespace nano
