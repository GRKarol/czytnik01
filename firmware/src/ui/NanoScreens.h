#pragma once

// Nano UI screen painters (NavMode::Modern). Each screen is described by a
// plain view struct and painted onto the Nano frame; tap targets go to a
// Sink. Nothing here reads App state, so the same code draws on the
// device (App builds the views in app/AppNano.inl) and on the PC
// (tools/nanosim renders every screen to an image).

#include <Arduino.h>

#include <vector>

#include "display/DisplayManager.h"

namespace nano {

using Icon = DisplayManager::NanoIcon;
using Role = DisplayManager::NanoRole;
using Align = DisplayManager::NanoAlign;
using Rect = ui::Rect;

constexpr int kScreenW = BoardConfig::DISPLAY_WIDTH;
constexpr int kScreenH = BoardConfig::DISPLAY_HEIGHT;
constexpr int kGap = 8;
constexpr int kMargin = 10;
constexpr int kHeaderH = 30;
constexpr int kCompactRailW = 60;
constexpr int kRailFooterH = 24;
constexpr int kNoTarget = -1;

// Rail placement, set by App before every Nano render.
struct Layout {
  bool railRight = false;
  bool compact = false;
  int railWidth = 160;
};
Layout &layout();
// Rail width that fits every tab label at body size in the active UI font.
int railWidthFor(const std::vector<String> &labels);
Rect railRect();
// Content area next to the rail / the whole screen (nested screens).
Rect tabContent();
Rect fullContent();

class Sink {
 public:
  virtual ~Sink() = default;
  virtual void target(const Rect &rect, int id) = 0;
  virtual bool pressed(int id) const = 0;
  virtual bool armed(int id) const {
    (void)id;
    return false;
  }
  // A settings tile that doubles as a slider was drawn here.
  virtual void slider(const Rect &rect, int id) {
    (void)rect;
    (void)id;
  }
};

// ── Shared pieces ──

struct RailTab {
  int id = kNoTarget;
  String label;
  Icon icon = Icon::None;
  bool active = false;
  bool badge = false;
};
void paintRail(DisplayManager &d, Sink &sink, const std::vector<RailTab> &tabs);

struct Header {
  int backId = kNoTarget;
  String title;
  String trailing;  // muted text on the right (count, position)
  size_t page = 0;
  size_t pageCount = 1;
  int prevId = kNoTarget;
  int nextId = kNoTarget;
  // Optional pill on the right (e.g. library sort).
  int pillId = kNoTarget;
  String pillLabel;
  Icon pillIcon = Icon::None;
};
// Paints the header row at the top of `area`; returns the y below it.
int paintHeader(DisplayManager &d, Sink &sink, const Rect &area, const Header &header);

struct Tile {
  int id = kNoTarget;
  String label;
  String detail;
  Icon icon = Icon::None;
  bool accent = false;
  bool enabled = true;
};
// `columns` x `rows` tiles filling `area`, only the first columns*rows used.
void paintTileGrid(DisplayManager &d, Sink &sink, const Rect &area, const std::vector<Tile> &tiles,
                   int columns, int rows);

// ── Czytaj ──

struct ReadHome {
  bool hasBook = false;
  String title;
  String author;
  String progressLabel;  // "42%"
  String hint;           // "Czytaj dalej" / "Wybierz ksiazke"
  int progressPercent = 0;
  uint16_t coverColor = 0x32FA;
  String coverInitials;
  int resumeId = kNoTarget;
  int fontsId = kNoTarget;
  String fontsLabel;
  std::vector<Tile> tiles;  // chapters, save points, library
};
void paintReadHome(DisplayManager &d, Sink &sink, const ReadHome &view);

// ── Sectioned settings (Ustawienia) ──

struct SectionItem {
  int id = kNoTarget;
  String label;
  Icon icon = Icon::None;
  bool toggle = false;
  bool on = false;
  bool fullWidth = false;
};
struct Section {
  String title;  // empty = no section label
  std::vector<SectionItem> items;
};
void paintSections(DisplayManager &d, Sink &sink, const Rect &area, const std::vector<Section> &sections);

// ── Motywy ──

struct ThemesView {
  int section = 0;  // 0 menu colors, 1 reading, 2 font, 3 layout
  static constexpr int kSections = 4;
  int segmentIds[kSections] = {kNoTarget, kNoTarget, kNoTarget, kNoTarget};
  String segmentLabels[kSections];
  struct PaletteChip {
    int id = kNoTarget;
    uint8_t palette = 0;
    String name;
    bool selected = false;
  };
  std::vector<PaletteChip> palettes;
  int ownAccentId = kNoTarget;  // kNoTarget = hide the switch
  String ownAccentLabel;
  bool ownAccentOn = false;
  // Reading: the three reading themes and the letter color.
  struct ReadingChip {
    int id = kNoTarget;
    uint8_t theme = 0;  // 0 dark, 1 light, 2 night
    String name;
    bool selected = false;
  };
  std::vector<ReadingChip> readingThemes;
  int letterColorId = kNoTarget;
  String letterColorLabel;
  String letterColorName;
  uint16_t letterColor = 0;
  String readingHint;
  struct FontChip {
    int id = kNoTarget;
    uint8_t family = 0;
    String name;
    String sample;
    bool selected = false;
  };
  std::vector<FontChip> fonts;
  // Layout: two chips (icons only / icons + labels); tapping the selected
  // one again moves the rail to the other side.
  struct LayoutChip {
    int id = kNoTarget;
    bool compact = false;
    bool railRight = false;
    String name;
    String detail;
    bool selected = false;
  };
  std::vector<LayoutChip> layouts;
  String layoutHint;
};
void paintThemes(DisplayManager &d, Sink &sink, const ThemesView &view);

// ── Biblioteka (bookshelf) ──

struct ShelfGeometry {
  Rect header;
  Rect viewport;
  Rect detail;
  int marker = 0;
};
ShelfGeometry shelfGeometry();
int32_t shelfClampOffset(size_t count, int32_t offset, int viewportWidth);
int32_t shelfCenteredOffset(size_t count, size_t index, int viewportWidth);
size_t shelfNearest(size_t count, int32_t offset, int markerX, int viewportX);
size_t shelfSpineIndexAt(int32_t contentX, size_t count);
int32_t shelfSpineLeft(size_t index);
int shelfSpineWidth(size_t index);
constexpr int kShelfDragThreshold = 20;

struct ShelfBook {
  String title;
  uint8_t progress = 0;
};
struct ShelfView {
  Header header;
  std::vector<ShelfBook> books;
  size_t selected = 0;
  int32_t offset = 0;
  String detailTitle;
  String detailAuthor;
  String detailStatus;
  String detailPercent;
  String emptyLabel;
};
void paintShelf(DisplayManager &d, Sink &sink, const ShelfView &view);

// ── Rozdzialy (chapter wheel) ──

constexpr int kWheelRowStep = 30;
constexpr int kWheelDragThreshold = 6;
Rect wheelViewport();
int wheelRowCenter(const Rect &viewport, int row, int offset);
int wheelRowHeight(bool centered);
bool wheelRowVisible(const Rect &viewport, int y, int height);

struct WheelView {
  Header header;
  // Titles of chapters [firstIndex, firstIndex + titles.size()).
  size_t firstIndex = 0;
  std::vector<String> titles;
  size_t count = 0;
  size_t centered = 0;
  int16_t offset = 0;
  size_t readingIndex = 0;
  int emptyId = kNoTarget;
  String emptyLabel;
};
void paintWheel(DisplayManager &d, Sink &sink, const WheelView &view);

// ── Lists (settings sub-screens, save points, plugins, pickers) ──

struct ListItem {
  enum class Kind : uint8_t { Button, Setting, Toggle, Slider, Label, Separator, Row };
  Kind kind = Kind::Button;
  int id = kNoTarget;
  String label;
  String value;
  Icon icon = Icon::None;
  bool on = false;
  bool armed = false;
  bool marked = false;  // current choice (check mark)
  bool fullWidth = false;
  int sliderValue = 0;
  int sliderMin = 0;
  int sliderMax = 1;
  bool dragging = false;
  DisplayManager::ReaderTypeface typeface = DisplayManager::ReaderTypeface::Count;
  // Kind::Row: trailing action (delete) inside the same row.
  int trailingId = kNoTarget;
  String trailingLabel;
  bool trailingArmed = false;
};
struct ListView {
  Header header;
  std::vector<ListItem> items;  // current page only
  int columns = 2;
  int rows = 3;
  bool fullScreen = false;  // no rail
};
void paintList(DisplayManager &d, Sink &sink, const ListView &view);

// ── Book details ──

struct BookDetailsView {
  Header header;
  String author;
  String percentLabel;
  int percent = 0;
  std::vector<Tile> actions;  // up to 4
};
void paintBookDetails(DisplayManager &d, Sink &sink, const BookDetailsView &view);

// ── Confirm dialog ──

struct ConfirmView {
  String question;
  String detail;
  int backId = kNoTarget;
  String backLabel;
  struct Action {
    int id = kNoTarget;
    String label;
    bool danger = false;
    bool armed = false;
  };
  std::vector<Action> actions;
};
void paintConfirm(DisplayManager &d, Sink &sink, const ConfirmView &view);

// ── Reader panel (paused, before reading starts) ──

struct ReaderPanelView {
  String chapter;
  String progressLabel;
  String timeLeft;
  int progressPercent = 0;
  String before;
  String word;
  String after;
  // Scroll reading mode: the page around the word instead of the RSVP line.
  bool scrollMode = false;
  std::vector<DisplayManager::ContextWord> words;
  size_t currentLocal = 0;
  int menuId = kNoTarget;
  int rewindId = kNoTarget;   // back to the start of the sentence
  int gotoId = kNoTarget;     // jump to %, page or chapter
  int statusId = kNoTarget;   // the top line + progress bar (also opens "go to")
  int chaptersId = kNoTarget;
  int bookmarkId = kNoTarget;
  bool bookmarkFilled = false;
  int minusId = kNoTarget;
  int wpmId = kNoTarget;
  int plusId = kNoTarget;
  String wpmLabel;
  int startId = kNoTarget;
  String startLabel;
  String menuLabel;
  String hint;  // gesture hint under the word, empty = none
};
Rect readerPanelWordArea();
Rect readerPanelBar();
Rect readerPanelStatusArea();
void paintReaderPanel(DisplayManager &d, Sink &sink, const ReaderPanelView &view);

// ── Przejdz do (jump to %, page or chapter) ──

struct GoToView {
  Header header;
  int segmentIds[3] = {kNoTarget, kNoTarget, kNoTarget};
  String segmentLabels[3];
  int segment = 0;
  String value;    // "42%", "Strona 120 / 412", "5 / 12"
  String detail;   // chapter at the target
  String hint;     // "1 strona = 250 slow"
  int sliderMin = 0;
  int sliderMax = 100;
  int sliderValue = 0;
  bool dragging = false;
  int minusId = kNoTarget;
  int plusId = kNoTarget;
  int readId = kNoTarget;
  String readLabel;
};
Rect goToBarRect();
void paintGoTo(DisplayManager &d, Sink &sink, const GoToView &view);

// ── Two big choices under a question (bookmark name, ...) ──

struct ChoiceView {
  Header header;
  String question;
  struct Option {
    int id = kNoTarget;
    String label;
    String detail;
    Icon icon = Icon::None;
    bool accent = false;
  };
  std::vector<Option> options;  // 2 or 3
};
void paintChoice(DisplayManager &d, Sink &sink, const ChoiceView &view);

// ── Kolor litery (palette) ──

struct ColorPickerView {
  Header header;
  int columns = 12;
  int rows = 3;
  struct Swatch {
    int id = kNoTarget;
    uint16_t color = 0;
    bool selected = false;
  };
  std::vector<Swatch> swatches;
  // Preview word in the reading colors with the chosen letter color.
  uint16_t previewBackground = 0;
  uint16_t previewWord = 0xFFFF;
  uint16_t previewFocus = 0x001F;
};
void paintColorPicker(DisplayManager &d, Sink &sink, const ColorPickerView &view);

}  // namespace nano
