// Covers and spines from the Flower app (storage/BookExtras.h) on the Czytaj
// card, book details, the library shelf and the Book screensaver. Pictures
// come from extras_cover.img / extras_spine.img (make_test_pictures.py);
// without them the scenes are skipped.
#include <cstdio>
#include <string>
#include <vector>

#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

using namespace nano;

namespace {

struct ExtrasSink : Sink {
  void target(const Rect &, int) override {}
  bool pressed(int) const override { return false; }
};

bool loadPicture(const char *path, std::vector<uint16_t> &pixels, NanoImage &image) {
  FILE *f = fopen(path, "rb");
  if (f == nullptr) return false;
  unsigned char header[8];
  if (fread(header, 1, 8, f) != 8 || header[0] != 'F' || header[1] != 'B' || header[2] != 'I' || header[3] != '1') {
    fclose(f);
    return false;
  }
  image.width = static_cast<uint16_t>(header[4] | (header[5] << 8));
  image.height = static_cast<uint16_t>(header[6] | (header[7] << 8));
  pixels.resize(static_cast<size_t>(image.width) * image.height);
  const size_t read = fread(pixels.data(), 2, pixels.size(), f);  // little-endian host
  fclose(f);
  if (read != pixels.size()) return false;
  image.pixels = pixels.data();
  return true;
}

}  // namespace

void runExtrasScreens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  std::vector<uint16_t> coverPixels;
  std::vector<uint16_t> spinePixels;
  NanoImage cover;
  NanoImage spine;
  if (!loadPicture("extras_cover.img", coverPixels, cover) || !loadPicture("extras_spine.img", spinePixels, spine)) {
    std::printf("extras: no test pictures, run make_test_pictures.py\n");
    return;
  }
  ExtrasSink sink;
  std::vector<String> labels = {"Czytaj", "Ustawienia", "Motywy", "Urz\x97""dzenie", "Pluginy"};
  layout().railWidth = railWidthFor(labels);
  std::vector<RailTab> tabs = {{1, "Czytaj", Icon::Books, true, false},
                               {2, "Ustawienia", Icon::Sliders, false, false},
                               {3, "Motywy", Icon::Palette, false, false},
                               {4, labels[3], Icon::Device, false, false},
                               {5, "Pluginy", Icon::Apps, false, false}};

  d.nanoBeginFrame();
  {
    ReadHome v;
    v.hasBook = true;
    v.title = "Quo Vadis";
    v.author = "Henryk Sienkiewicz";
    v.progressLabel = "22%";
    v.progressPercent = 22;
    v.cover = cover;
    v.resumeId = 10;
    v.fontsId = 11;
    Tile a;
    a.label = "Rozdzia\x83""y";
    a.icon = Icon::List;
    Tile b;
    b.label = "Punkty zapisu";
    b.icon = Icon::Bookmark;
    Tile c;
    c.label = "Biblioteka";
    c.icon = Icon::Books;
    v.tiles = {a, b, c};
    paintReadHome(d, sink, v);
  }
  paintRail(d, sink, tabs);
  d.nanoEndFrame();
  dump(d, "extras_read");

  d.nanoBeginFrame();
  {
    BookDetailsView v;
    v.header.backId = 1;
    v.header.title = "Quo Vadis";
    v.author = "Henryk Sienkiewicz";
    v.percentLabel = "22%";
    v.percent = 22;
    v.cover = cover;
    const char *names[] = {"Czytaj dalej", "Rozdzia\x83""y", "Od pocz\x97""tku", "Usu\x9D"" z karty", "Przejd\xB5"" do"};
    const Icon icons[] = {Icon::Play, Icon::List, Icon::Restart, Icon::Trash, Icon::Target};
    for (int i = 0; i < 5; ++i) {
      Tile t;
      t.id = 20 + i;
      t.label = names[i];
      t.icon = icons[i];
      t.accent = i == 0;
      v.actions.push_back(t);
    }
    paintBookDetails(d, sink, v);
  }
  d.nanoEndFrame();
  dump(d, "extras_details");

  d.nanoBeginFrame();
  {
    ShelfView v;
    v.header.backId = 1;
    v.header.title = "Biblioteka";
    v.header.trailing = "10";
    const char *titles[] = {"Pan Tadeusz", "Lalka", "Quo Vadis", "Ferdydurke", "Solaris",
                            "Wiedzmin",    "Dziady", "Chlopi",   "Kordian",    "Potop"};
    for (int i = 0; i < 10; ++i) {
      ShelfBook book;
      book.title = titles[i];
      book.progress = static_cast<uint8_t>(i * 11 % 100);
      if (i == 2 || i == 5) book.spine = spine;
      v.books.push_back(book);
    }
    v.selected = 2;
    const ShelfGeometry g = shelfGeometry();
    v.offset = shelfCenteredOffset(v.books.size(), v.selected, g.viewport.w);
    v.detailTitle = "Quo Vadis";
    v.detailAuthor = "Henryk Sienkiewicz";
    v.detailPercent = "22%";
    paintShelf(d, sink, v);
  }
  paintRail(d, sink, tabs);
  d.nanoEndFrame();
  dump(d, "extras_library");

  SaverBookView saver;
  saver.hasBook = true;
  saver.title = "Quo Vadis";
  saver.author = "Henryk Sienkiewicz";
  saver.progressPercent = 22;
  saver.progressLabel = "22%";
  saver.cover = cover;
  paintSaverBook(d, saver);
  dump(d, "extras_saver");
}
