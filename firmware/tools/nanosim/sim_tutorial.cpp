// Samouczek pages (generated texts: Polish rows of tools/translations.csv).
#include <vector>

#include "display/DisplayManager.h"
#include "ui/NanoScreens.h"

namespace {
struct NullSink : nano::Sink {
  void target(const ui::Rect &, int) override {}
  bool pressed(int) const override { return false; }
};
}  // namespace

void runTutorialScreens(DisplayManager &d, void (*dump)(const DisplayManager &, const char *)) {
  NullSink sink;
  {
    nano::TutorialView v;
    v.caption = "Samouczek";
    v.page = 0;
    v.pageCount = 6;
    v.art = nano::TutorialArt::Rsvp;
    v.title = "Jedno s\x83""owo naraz";
    v.body = "Czytnik pokazuje s\x83""owa jedno po drugim w tym samym miejscu. Patrz na kolorow\x97"" liter\x99"". Oko nie skacze po linijkach, wi\x99""c czytasz szybciej i mniej si\x99"" m\x99""czysz.";
    v.artWord = "czytanie";
    v.artStart = "Czytaj";
    v.artUnit = "s\x83""/min";
    v.artTile = "Tempo";
    v.artTabs[0] = "Czytaj";
    v.artTabs[1] = "Ustawienia";
    v.artTabs[2] = "Motywy";
    v.artTabs[3] = "Urz\x97""dzenie";
    v.backId = -1;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 2;
    v.nextLabel = "Dalej";
    v.skipId = 3;
    v.skipLabel = "Pomi\x9D""";
    nano::paintTutorial(d, sink, v);
    dump(d, "tutorial_1");
  }
  {
    nano::TutorialView v;
    v.caption = "Samouczek";
    v.page = 1;
    v.pageCount = 6;
    v.art = nano::TutorialArt::Start;
    v.title = "Start i pauza";
    v.body = "Na ekranie przed czytaniem dotknij Czytaj. S\x83""owa lec\x97"", dop\xF3""ki nie dotkniesz ekranu. Mo\xB5""esz te\xB5"" przytrzyma\x9B"" palec na s\x83""owie: czytnik czyta, dop\xF3""ki trzymasz.";
    v.artWord = "czytanie";
    v.artStart = "Czytaj";
    v.artUnit = "s\x83""/min";
    v.artTile = "Tempo";
    v.artTabs[0] = "Czytaj";
    v.artTabs[1] = "Ustawienia";
    v.artTabs[2] = "Motywy";
    v.artTabs[3] = "Urz\x97""dzenie";
    v.backId = 1;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 2;
    v.nextLabel = "Dalej";
    v.skipId = 3;
    v.skipLabel = "Pomi\x9D""";
    nano::paintTutorial(d, sink, v);
    dump(d, "tutorial_2");
  }
  {
    nano::TutorialView v;
    v.caption = "Samouczek";
    v.page = 2;
    v.pageCount = 6;
    v.art = nano::TutorialArt::Speed;
    v.title = "Tempo";
    v.body = "Przyciski - i + na ekranie przed czytaniem zmieniaj\x97"" liczb\x99"" s\x83""\xF3""w na minut\x99"". Dla wi\x99""kszo\x9F""ci os\xF3""b wygodny start to 250-350. Dok\x83""adnie ustawisz tempo w Ustawieniach.";
    v.artWord = "czytanie";
    v.artStart = "Czytaj";
    v.artUnit = "s\x83""/min";
    v.artTile = "Tempo";
    v.artTabs[0] = "Czytaj";
    v.artTabs[1] = "Ustawienia";
    v.artTabs[2] = "Motywy";
    v.artTabs[3] = "Urz\x97""dzenie";
    v.backId = 1;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 2;
    v.nextLabel = "Dalej";
    v.skipId = 3;
    v.skipLabel = "Pomi\x9D""";
    nano::paintTutorial(d, sink, v);
    dump(d, "tutorial_3");
  }
  {
    nano::TutorialView v;
    v.caption = "Samouczek";
    v.page = 3;
    v.pageCount = 6;
    v.art = nano::TutorialArt::Scrub;
    v.title = "Cofanie i skoki";
    v.body = "Przesu\x9D"" palcem w bok po s\x83""owie, \xB5""eby cofn\x97""\x9B"" si\x99"" albo p\xF3""j\x9F""\x9B"" dalej. Dotknij paska u g\xF3""ry, \xB5""eby przej\x9F""\x9B"" do procentu, strony albo rozdzia\x83""u. Przycisk << wraca na pocz\x97""tek zdania.";
    v.artWord = "czytanie";
    v.artStart = "Czytaj";
    v.artUnit = "s\x83""/min";
    v.artTile = "Tempo";
    v.artTabs[0] = "Czytaj";
    v.artTabs[1] = "Ustawienia";
    v.artTabs[2] = "Motywy";
    v.artTabs[3] = "Urz\x97""dzenie";
    v.backId = 1;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 2;
    v.nextLabel = "Dalej";
    v.skipId = 3;
    v.skipLabel = "Pomi\x9D""";
    nano::paintTutorial(d, sink, v);
    dump(d, "tutorial_4");
  }
  {
    nano::TutorialView v;
    v.caption = "Samouczek";
    v.page = 4;
    v.pageCount = 6;
    v.art = nano::TutorialArt::Menu;
    v.title = "Menu";
    v.body = "Przycisk Menu otwiera zak\x83""adki z boku ekranu: Czytaj, Ustawienia, Motywy i Urz\x97""dzenie. Kr\xF3""tkie naci\x9F""ni\x99""cie przycisku zasilania te\xB5"" otwiera menu, a przytrzymanie wy\x83""\x97""cza czytnik.";
    v.artWord = "czytanie";
    v.artStart = "Czytaj";
    v.artUnit = "s\x83""/min";
    v.artTile = "Tempo";
    v.artTabs[0] = "Czytaj";
    v.artTabs[1] = "Ustawienia";
    v.artTabs[2] = "Motywy";
    v.artTabs[3] = "Urz\x97""dzenie";
    v.backId = 1;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 2;
    v.nextLabel = "Dalej";
    v.skipId = 3;
    v.skipLabel = "Pomi\x9D""";
    nano::paintTutorial(d, sink, v);
    dump(d, "tutorial_5");
  }
  {
    nano::TutorialView v;
    v.caption = "Samouczek";
    v.page = 5;
    v.pageCount = 6;
    v.art = nano::TutorialArt::Help;
    v.title = "Pomoc";
    v.body = "K\xF3""\x83""ko ? przy opcji w Ustawieniach otwiera jej pe\x83""ny opis. Gdy znasz ju\xB5"" wszystko, wy\x83""\x97""cz pomoc, a kafelki b\x99""d\x97"" szersze. Ten samouczek znajdziesz w zak\x83""adce Urz\x97""dzenie.";
    v.artWord = "czytanie";
    v.artStart = "Czytaj";
    v.artUnit = "s\x83""/min";
    v.artTile = "Tempo";
    v.artTabs[0] = "Czytaj";
    v.artTabs[1] = "Ustawienia";
    v.artTabs[2] = "Motywy";
    v.artTabs[3] = "Urz\x97""dzenie";
    v.backId = 1;
    v.backLabel = "Wr\xF3""\x9B""";
    v.nextId = 2;
    v.nextLabel = "Gotowe";
    v.skipId = -1;
    v.skipLabel = "Pomi\x9D""";
    nano::paintTutorial(d, sink, v);
    dump(d, "tutorial_6");
  }
}
