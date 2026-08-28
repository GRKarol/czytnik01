#pragma once

// Immediate-mode grid layout helpers for the touch UI.
//
// Every menu-style screen (Main/Settings/BookPicker/…) shares one rule:
// build a Rect for each visible item with the SAME math used to hit-test
// taps, so drawing and touch can never drift apart. Screens call
// layoutGrid() once per render, keep the resulting Rects around, and reuse
// them verbatim in their touch handler.

#include <Arduino.h>
#include <algorithm>
#include <vector>

namespace ui {

struct Rect {
  uint16_t x = 0;
  uint16_t y = 0;
  uint16_t w = 0;
  uint16_t h = 0;

  bool contains(uint16_t px, uint16_t py) const {
    return px >= x && px < x + w && py >= y && py < y + h;
  }
};

struct GridSpec {
  size_t columns = 1;
  size_t rows = 1;
  size_t itemsPerPage = 1;
};

// Confirm-style dialogs (2-3 actions) read better as tall single-column
// buttons; longer lists (settings, library, plugins…) use a fixed 2-row grid
// whose column count grows with the item count. The screen is 640x172 —
// short and wide — so pinning rows at 2 (instead of the old fixed 4) keeps
// each cell closer to a real app tile (roughly 2:1 to 4:1) instead of a
// full-width list bar. Caps at 4 columns (8 per page); screens with more
// items paginate via the existing page-dot indicator / swipe.
inline GridSpec computeGridSpec(size_t itemCount) {
  GridSpec spec;
  if (itemCount == 0) {
    spec.columns = 1;
    spec.rows = 1;
    spec.itemsPerPage = 1;
    return spec;
  }
  if (itemCount <= 3) {
    spec.columns = 1;
    spec.rows = itemCount;
    spec.itemsPerPage = itemCount;
    return spec;
  }
  spec.rows = 2;
  const size_t neededColumns = (itemCount + spec.rows - 1) / spec.rows;  // ceil
  spec.columns = std::min<size_t>(4, std::max<size_t>(2, neededColumns));
  spec.itemsPerPage = spec.columns * spec.rows;
  return spec;
}

// Lays out up to spec.itemsPerPage rects (row-major) inside the given area.
// If fewer items are on the last page, rows are simply left empty rather
// than stretched — keeps tap targets a predictable, consistent size.
inline std::vector<Rect> layoutGrid(const GridSpec &spec, size_t itemsOnPage, uint16_t areaX,
                                    uint16_t areaY, uint16_t areaW, uint16_t areaH,
                                    uint16_t gap) {
  std::vector<Rect> rects;
  if (itemsOnPage == 0 || spec.columns == 0 || spec.rows == 0) {
    return rects;
  }
  itemsOnPage = std::min(itemsOnPage, spec.itemsPerPage);

  const uint16_t cellW = static_cast<uint16_t>(
      (areaW - gap * (spec.columns - 1)) / static_cast<uint16_t>(spec.columns));
  const uint16_t cellH = static_cast<uint16_t>(
      (areaH - gap * (spec.rows - 1)) / static_cast<uint16_t>(spec.rows));

  rects.reserve(itemsOnPage);
  for (size_t i = 0; i < itemsOnPage; ++i) {
    const size_t row = i / spec.columns;
    const size_t col = i % spec.columns;
    Rect r;
    r.x = static_cast<uint16_t>(areaX + col * (cellW + gap));
    r.y = static_cast<uint16_t>(areaY + row * (cellH + gap));
    r.w = cellW;
    r.h = cellH;
    rects.push_back(r);
  }
  return rects;
}

}  // namespace ui
