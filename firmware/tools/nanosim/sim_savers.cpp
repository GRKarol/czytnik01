// Screensaver scenes (App::ScreensaverMode Book / Words / Waves and the
// palette-colored cell grid), in two palettes to check the colors follow
// Motywy > Kolory.
#include <vector>

#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

void runSaverScreens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  const uint8_t palettes[] = {0, 5};
  const char *suffix[] = {"", "_dracula"};
  for (int p = 0; p < 2; ++p) {
    d.setNanoPalette(palettes[p], false);
    std::string s = suffix[p];

    nano::SaverBookView book;
    book.hasBook = true;
    book.title = "Pan Tadeusz";
    book.author = "Adam Mickiewicz";
    book.progressPercent = 37;
    book.progressLabel = "37%";
    book.coverColor = 0x1AF5;
    book.coverInitials = "PT";
    book.driftX = -40;
    book.driftY = 6;
    book.overlay.label = "Ksi\x97""\xB5""ka";
    book.overlay.labelAlpha = 255;
    nano::paintSaverBook(d, book);
    dump(d, ("saver_book" + s).c_str());

    nano::SaverWordsView words;
    const char *text[3] = {"Ostatni zajazd na Litwie, historia szlachecka z roku 1811 i 1812",
                           "Litwo! Ojczyzno moja! ty jeste\x9F"" jak zdrowie. Ile ci\x99"" trzeba ceni\x9B"",",
                           "ten tylko si\x99"" dowie, kto ci\x99"" straci\x83"". Dzi\x9F"" pi\x99""kno\x9F""\x9B"" tw\x97"" w ca\x83""ej ozdobie"};
    const int laneY[] = {32, 84, 132};
    const uint8_t laneSize[] = {1, 3, 2};
    const uint8_t laneAlpha[] = {90, 235, 140};
    for (int i = 0; i < 3; ++i) {
      nano::SaverLane lane;
      String all = text[i];
      int start = 0;
      while (start < static_cast<int>(all.length())) {
        int end = all.indexOf(' ', start);
        if (end < 0) end = all.length();
        lane.words.push_back(all.substring(start, end));
        start = end + 1;
      }
      lane.y = laneY[i];
      lane.size = laneSize[i];
      lane.alpha = laneAlpha[i];
      lane.offset = 40 + 60 * i;
      lane.markCentre = i == 1;
      words.lanes.push_back(lane);
    }
    words.overlay.hint = "Dotknij, aby wr\xF3""ci\x9B""";
    words.overlay.hintAlpha = 200;
    nano::paintSaverWords(d, words);
    dump(d, ("saver_words" + s).c_str());

    nano::SaverWavesView waves;
    waves.phase = 120;
    nano::paintSaverWaves(d, waves);
    dump(d, ("saver_waves" + s).c_str());

    const uint16_t cols = 320, rows = 86;
    std::vector<uint32_t> cells((cols * rows + 31) / 32, 0);
    std::vector<uint32_t> dim((cols * rows + 31) / 32, 0);
    for (uint32_t i = 0; i < cols * rows; ++i) {
      const uint32_t h = (i * 2654435761u) >> 24;
      if (h < 30) cells[i / 32] |= 1u << (i % 32);
      else if (h < 60) dim[i / 32] |= 1u << (i % 32);
    }
    d.renderLifeScreensaver(cells, cols, rows, 1 + p, &dim, "", 0, "\xB4" "ycie", 255);
    dump(d, ("saver_life" + s).c_str());
  }
  d.setNanoPalette(0, false);
}
