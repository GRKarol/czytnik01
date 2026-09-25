#pragma once

#include <Arduino.h>

// Minimal built-in icon set for grid buttons. IDs only — the vector glyphs
// are drawn by DisplayManager::drawIcon() (needs the virtual frame buffer,
// so it can't live as a free function here). Bitmaps loaded later (RGB565
// C-arrays, same pattern as the embedded fonts) take priority over these
// vector placeholders — see DisplayManager::Button::iconBitmap.
namespace ui {

enum class IconId : uint8_t {
  None = 0,
  Back,
  Book,
  SavePoint,
  Settings,
  Plugin,
  Power,
  Wifi,
  Play,
  Delete,
  Reset,
  Check,
  Record,
  Stop,
  Eye,
  // Dock icons for App::NavMode::Modern's bottom navigation bar (see
  // DisplayManager::drawModernDock()) — no existing glyph fit "device
  // hub" or "saved preset" well enough to reuse.
  Device,
  Preset,
};

}  // namespace ui
