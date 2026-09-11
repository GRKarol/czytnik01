#pragma once

#include "Localization.h"

/// Extended translation keys for strings that were previously only
/// Polish/English via the polish() helper. These cover all 6 languages.
enum class TrKey : uint8_t {
  LoadingBook,
  PreparingSD,
  EjectWhenDone,
  LowBattery,
  PoweringOff,
  ChargeSoon,
  Remaining,
  Connectivity,
  AboutHelp,
  WifiAdvanced,
  PhoneSync,
  TurnOnToSeeWifi,
  HomeWifi,
  NotSet,
  Connected,
  Version,
  BrandLabel,
  PhoneAppLabel,
  DevModeOn,
  ReaderHand,
  FooterLabel,
  BatteryLabel,
  Screensaver,
  ReadingBattery,
  ReadingChapter,
  ReadingPercent,
  PauseBehaviour,
  BaseSpeed,
  Network,
  ChooseNetwork,
  ForgetNetwork,
  FirmwareUpdate,
  Instant,
  Sentence,
  LeftHand,
  RightHand,
  ChapterTime,
  BookTime,
  PercentRead,
  TimeRemaining,
  Voltage,
  Percentage,
  Maze,
  ScreenOff,
  Life,
  StartingWifi,
  CouldNotStart,
  Returning,
  Stopping,
  PleaseWait,
  WifiNotSet,
  SettingsWifi,
  Restarting,
  Yes,
  No,
  ScreensaverTimeout,
  ScreensaverAutoOff,
  ScreensaverSleepGuard,
  ScreensaverStyle,
  ScreensaverPreview,
  Stars,
  MatrixRain,
  ScreensaverHint,
  Minutes1,
  Minutes2,
  Minutes3,
  Minutes5,
  Minutes10,
  Minutes15,
  Minutes20,
  Minutes30,
  Never,
};

namespace Translations {

inline const char *tr(UiLanguage lang, TrKey key) {
  switch (key) {
    case TrKey::LoadingBook:
      switch (lang) {
        case UiLanguage::Polish: return "Wczytywanie ksiazki";
        case UiLanguage::Spanish: return "Cargando libro";
        case UiLanguage::French: return "Chargement du livre";
        case UiLanguage::German: return "Buch laden";
        case UiLanguage::Romanian: return "Se incarca cartea";
        default: return "Loading book";
      }
    case TrKey::PreparingSD:
      switch (lang) {
        case UiLanguage::Polish: return "Przygotowuje SD";
        case UiLanguage::Spanish: return "Preparando SD";
        case UiLanguage::French: return "Preparation SD";
        case UiLanguage::German: return "SD vorbereiten";
        case UiLanguage::Romanian: return "Pregatire SD";
        default: return "Preparing SD";
      }
    case TrKey::EjectWhenDone:
      switch (lang) {
        case UiLanguage::Polish: return "Wysun po skonczeniu";
        case UiLanguage::Spanish: return "Expulsar al terminar";
        case UiLanguage::French: return "Ejecter une fois fini";
        case UiLanguage::German: return "Auswerfen wenn fertig";
        case UiLanguage::Romanian: return "Scoate cand e gata";
        default: return "Eject when done";
      }
    case TrKey::LowBattery:
      switch (lang) {
        case UiLanguage::Polish: return "NISKA BATERIA";
        case UiLanguage::Spanish: return "BATERIA BAJA";
        case UiLanguage::French: return "BATTERIE FAIBLE";
        case UiLanguage::German: return "AKKU SCHWACH";
        case UiLanguage::Romanian: return "BATERIE SLABA";
        default: return "LOW BATTERY";
      }
    case TrKey::PoweringOff:
      switch (lang) {
        case UiLanguage::Polish: return "Wylaczam";
        case UiLanguage::Spanish: return "Apagando";
        case UiLanguage::French: return "Extinction";
        case UiLanguage::German: return "Ausschalten";
        case UiLanguage::Romanian: return "Se opreste";
        default: return "Powering off";
      }
    case TrKey::ChargeSoon:
      switch (lang) {
        case UiLanguage::Polish: return " naladuj wkrotce";
        case UiLanguage::Spanish: return " cargue pronto";
        case UiLanguage::French: return " rechargez bientot";
        case UiLanguage::German: return " bald laden";
        case UiLanguage::Romanian: return " incarcati curand";
        default: return " charge soon";
      }
    case TrKey::Remaining:
      switch (lang) {
        case UiLanguage::Polish: return "% zostalo";
        case UiLanguage::Spanish: return "% restante";
        case UiLanguage::French: return "% restant";
        case UiLanguage::German: return "% verbleibend";
        case UiLanguage::Romanian: return "% ramas";
        default: return "% remaining";
      }
    case TrKey::Connectivity:
      switch (lang) {
        case UiLanguage::Polish: return "Polaczenia";
        case UiLanguage::Spanish: return "Conectividad";
        case UiLanguage::French: return "Connectivite";
        case UiLanguage::German: return "Verbindungen";
        case UiLanguage::Romanian: return "Conectivitate";
        default: return "Connectivity";
      }
    case TrKey::AboutHelp:
      switch (lang) {
        case UiLanguage::Polish: return "Informacje";
        case UiLanguage::Spanish: return "Informacion";
        case UiLanguage::French: return "Informations";
        case UiLanguage::German: return "Info / Hilfe";
        case UiLanguage::Romanian: return "Informatii";
        default: return "About / Help";
      }
    case TrKey::WifiAdvanced:
      switch (lang) {
        case UiLanguage::Polish: return "Wi-Fi (zaaw.)";
        case UiLanguage::Spanish: return "Wi-Fi (avanzado)";
        case UiLanguage::French: return "Wi-Fi (avance)";
        case UiLanguage::German: return "Wi-Fi (erweitert)";
        case UiLanguage::Romanian: return "Wi-Fi (avansat)";
        default: return "Wi-Fi (advanced)";
      }
    case TrKey::PhoneSync:
      switch (lang) {
        case UiLanguage::Polish: return "Sync z tel.: ";
        case UiLanguage::Spanish: return "Sync movil: ";
        case UiLanguage::French: return "Sync tel.: ";
        case UiLanguage::German: return "Handy-Sync: ";
        case UiLanguage::Romanian: return "Sync telefon: ";
        default: return "Phone sync: ";
      }
    case TrKey::TurnOnToSeeWifi:
      switch (lang) {
        case UiLanguage::Polish: return "    (wlacz aby zobaczyc kod Wi-Fi)";
        case UiLanguage::Spanish: return "    (active para ver codigo Wi-Fi)";
        case UiLanguage::French: return "    (activer pour voir code Wi-Fi)";
        case UiLanguage::German: return "    (einschalten fuer Wi-Fi-Code)";
        case UiLanguage::Romanian: return "    (porniti pt. cod Wi-Fi)";
        default: return "    (turn on to see Wi-Fi code)";
      }
    case TrKey::HomeWifi:
      switch (lang) {
        case UiLanguage::Polish: return "Wi-Fi domowe: ";
        case UiLanguage::Spanish: return "Wi-Fi hogar: ";
        case UiLanguage::French: return "Wi-Fi maison: ";
        case UiLanguage::German: return "Heim-Wi-Fi: ";
        case UiLanguage::Romanian: return "Wi-Fi acasa: ";
        default: return "Home Wi-Fi: ";
      }
    case TrKey::NotSet:
      switch (lang) {
        case UiLanguage::Polish: return "Brak";
        case UiLanguage::Spanish: return "Sin config.";
        case UiLanguage::French: return "Non defini";
        case UiLanguage::German: return "Nicht gesetzt";
        case UiLanguage::Romanian: return "Nesetat";
        default: return "Not set";
      }
    case TrKey::Connected:
      switch (lang) {
        case UiLanguage::Polish: return "POLACZONY";
        case UiLanguage::Spanish: return "CONECTADO";
        case UiLanguage::French: return "CONNECTE";
        case UiLanguage::German: return "VERBUNDEN";
        case UiLanguage::Romanian: return "CONECTAT";
        default: return "CONNECTED";
      }
    case TrKey::Version:
      switch (lang) {
        case UiLanguage::Polish: return "Wersja: ";
        case UiLanguage::Spanish: return "Version: ";
        case UiLanguage::French: return "Version : ";
        case UiLanguage::German: return "Version: ";
        case UiLanguage::Romanian: return "Versiune: ";
        default: return "Version: ";
      }
    case TrKey::BrandLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Marka: Flower (Czytnik01)";
        case UiLanguage::Spanish: return "Marca: Flower (Czytnik01)";
        case UiLanguage::French: return "Marque: Flower (Czytnik01)";
        case UiLanguage::German: return "Marke: Flower (Czytnik01)";
        case UiLanguage::Romanian: return "Marca: Flower (Czytnik01)";
        default: return "Brand: Flower (Czytnik01)";
      }
    case TrKey::PhoneAppLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Aplikacja: grkarol.github.io/czytnik01/app";
        case UiLanguage::Spanish: return "App movil: grkarol.github.io/czytnik01/app";
        case UiLanguage::French: return "App tel.: grkarol.github.io/czytnik01/app";
        case UiLanguage::German: return "Handy-App: grkarol.github.io/czytnik01/app";
        case UiLanguage::Romanian: return "Aplicatie: grkarol.github.io/czytnik01/app";
        default: return "Phone app: grkarol.github.io/czytnik01/app";
      }
    case TrKey::DevModeOn:
      switch (lang) {
        case UiLanguage::Polish: return "Tryb dev: WL (wylacz)";
        case UiLanguage::Spanish: return "Modo dev: ON (desactivar)";
        case UiLanguage::French: return "Mode dev: ON (desactiver)";
        case UiLanguage::German: return "Dev-Modus: AN (ausschalten)";
        case UiLanguage::Romanian: return "Mod dev: ON (dezactiveaza)";
        default: return "Developer mode: ON (turn off)";
      }
    case TrKey::ReaderHand:
      switch (lang) {
        case UiLanguage::Polish: return "Dlon: ";
        case UiLanguage::Spanish: return "Mano: ";
        case UiLanguage::French: return "Main: ";
        case UiLanguage::German: return "Hand: ";
        case UiLanguage::Romanian: return "Mana: ";
        default: return "Reader hand: ";
      }
    case TrKey::FooterLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Stopka: ";
        case UiLanguage::Spanish: return "Pie: ";
        case UiLanguage::French: return "Pied: ";
        case UiLanguage::German: return "Fusszeile: ";
        case UiLanguage::Romanian: return "Subsol: ";
        default: return "Footer label: ";
      }
    case TrKey::BatteryLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Bateria: ";
        case UiLanguage::Spanish: return "Bateria: ";
        case UiLanguage::French: return "Batterie: ";
        case UiLanguage::German: return "Akku: ";
        case UiLanguage::Romanian: return "Baterie: ";
        default: return "Battery label: ";
      }
    case TrKey::Screensaver:
      switch (lang) {
        case UiLanguage::Polish: return "Wygaszacz: ";
        case UiLanguage::Spanish: return "Salvapant.: ";
        case UiLanguage::French: return "Ecran veille: ";
        case UiLanguage::German: return "Bildschirmsch.: ";
        case UiLanguage::Romanian: return "Screensaver: ";
        default: return "Screensaver: ";
      }
    case TrKey::ReadingBattery:
      switch (lang) {
        case UiLanguage::Polish: return "Bateria w czyt.: ";
        case UiLanguage::Spanish: return "Bateria en lect.: ";
        case UiLanguage::French: return "Batterie en lect.: ";
        case UiLanguage::German: return "Akku beim Lesen: ";
        case UiLanguage::Romanian: return "Baterie in citire: ";
        default: return "Reading battery: ";
      }
    case TrKey::ReadingChapter:
      switch (lang) {
        case UiLanguage::Polish: return "Rozdz. w czyt.: ";
        case UiLanguage::Spanish: return "Cap. en lect.: ";
        case UiLanguage::French: return "Chap. en lect.: ";
        case UiLanguage::German: return "Kapitel beim Lesen: ";
        case UiLanguage::Romanian: return "Capitol in citire: ";
        default: return "Reading chapter: ";
      }
    case TrKey::ReadingPercent:
      switch (lang) {
        case UiLanguage::Polish: return "Procent w czyt.: ";
        case UiLanguage::Spanish: return "Porcent. en lect.: ";
        case UiLanguage::French: return "Pourcent. en lect.: ";
        case UiLanguage::German: return "Prozent beim Lesen: ";
        case UiLanguage::Romanian: return "Procent in citire: ";
        default: return "Reading percent: ";
      }
    case TrKey::PauseBehaviour:
      switch (lang) {
        case UiLanguage::Polish: return "Pauza: ";
        case UiLanguage::Spanish: return "Pausa: ";
        case UiLanguage::French: return "Pause: ";
        case UiLanguage::German: return "Pause: ";
        case UiLanguage::Romanian: return "Pauza: ";
        default: return "Pause behaviour: ";
      }
    case TrKey::BaseSpeed:
      switch (lang) {
        case UiLanguage::Polish: return "Tempo: ";
        case UiLanguage::Spanish: return "Velocidad: ";
        case UiLanguage::French: return "Vitesse: ";
        case UiLanguage::German: return "Tempo: ";
        case UiLanguage::Romanian: return "Viteza: ";
        default: return "Base speed: ";
      }
    case TrKey::Network:
      switch (lang) {
        case UiLanguage::Polish: return "Siec: ";
        case UiLanguage::Spanish: return "Red: ";
        case UiLanguage::French: return "Reseau: ";
        case UiLanguage::German: return "Netzwerk: ";
        case UiLanguage::Romanian: return "Retea: ";
        default: return "Network: ";
      }
    case TrKey::ChooseNetwork:
      switch (lang) {
        case UiLanguage::Polish: return "Wybierz siec";
        case UiLanguage::Spanish: return "Elegir red";
        case UiLanguage::French: return "Choisir reseau";
        case UiLanguage::German: return "Netzwerk waehlen";
        case UiLanguage::Romanian: return "Alege retea";
        default: return "Choose network";
      }
    case TrKey::ForgetNetwork:
      switch (lang) {
        case UiLanguage::Polish: return "Zapomnij siec";
        case UiLanguage::Spanish: return "Olvidar red";
        case UiLanguage::French: return "Oublier reseau";
        case UiLanguage::German: return "Netzwerk vergessen";
        case UiLanguage::Romanian: return "Uita reteaua";
        default: return "Forget network";
      }
    case TrKey::FirmwareUpdate:
      switch (lang) {
        case UiLanguage::Polish: return "Aktualizacja firmware";
        case UiLanguage::Spanish: return "Actualizar firmware";
        case UiLanguage::French: return "Mise a jour firmware";
        case UiLanguage::German: return "Firmware-Update";
        case UiLanguage::Romanian: return "Actualizare firmware";
        default: return "Firmware update";
      }
    case TrKey::Instant:
      switch (lang) {
        case UiLanguage::Polish: return "Natychm.";
        case UiLanguage::Spanish: return "Instantaneo";
        case UiLanguage::French: return "Instantane";
        case UiLanguage::German: return "Sofort";
        case UiLanguage::Romanian: return "Instant";
        default: return "Instant";
      }
    case TrKey::Sentence:
      switch (lang) {
        case UiLanguage::Polish: return "Zdanie";
        case UiLanguage::Spanish: return "Oracion";
        case UiLanguage::French: return "Phrase";
        case UiLanguage::German: return "Satz";
        case UiLanguage::Romanian: return "Propozitie";
        default: return "Sentence";
      }
    case TrKey::LeftHand:
      switch (lang) {
        case UiLanguage::Polish: return "Lewa";
        case UiLanguage::Spanish: return "Izquierda";
        case UiLanguage::French: return "Gauche";
        case UiLanguage::German: return "Links";
        case UiLanguage::Romanian: return "Stanga";
        default: return "Left";
      }
    case TrKey::RightHand:
      switch (lang) {
        case UiLanguage::Polish: return "Prawa";
        case UiLanguage::Spanish: return "Derecha";
        case UiLanguage::French: return "Droite";
        case UiLanguage::German: return "Rechts";
        case UiLanguage::Romanian: return "Dreapta";
        default: return "Right";
      }
    case TrKey::ChapterTime:
      switch (lang) {
        case UiLanguage::Polish: return "Czas rozdz.";
        case UiLanguage::Spanish: return "Tiempo cap.";
        case UiLanguage::French: return "Temps chap.";
        case UiLanguage::German: return "Kapitelzeit";
        case UiLanguage::Romanian: return "Timp capitol";
        default: return "Chapter time";
      }
    case TrKey::BookTime:
      switch (lang) {
        case UiLanguage::Polish: return "Czas ksiazki";
        case UiLanguage::Spanish: return "Tiempo libro";
        case UiLanguage::French: return "Temps livre";
        case UiLanguage::German: return "Buchzeit";
        case UiLanguage::Romanian: return "Timp carte";
        default: return "Book time";
      }
    case TrKey::PercentRead:
      switch (lang) {
        case UiLanguage::Polish: return "Procent";
        case UiLanguage::Spanish: return "Porcentaje";
        case UiLanguage::French: return "Pourcentage";
        case UiLanguage::German: return "Prozent";
        case UiLanguage::Romanian: return "Procent";
        default: return "Percent read";
      }
    case TrKey::TimeRemaining:
      switch (lang) {
        case UiLanguage::Polish: return "Czas pracy";
        case UiLanguage::Spanish: return "Tiempo restante";
        case UiLanguage::French: return "Temps restant";
        case UiLanguage::German: return "Restzeit";
        case UiLanguage::Romanian: return "Timp ramas";
        default: return "Time remaining";
      }
    case TrKey::Voltage:
      switch (lang) {
        case UiLanguage::Polish: return "Napiecie";
        case UiLanguage::Spanish: return "Voltaje";
        case UiLanguage::French: return "Tension";
        case UiLanguage::German: return "Spannung";
        case UiLanguage::Romanian: return "Tensiune";
        default: return "Voltage";
      }
    case TrKey::Percentage:
      switch (lang) {
        case UiLanguage::Polish: return "Procent";
        case UiLanguage::Spanish: return "Porcentaje";
        case UiLanguage::French: return "Pourcentage";
        case UiLanguage::German: return "Prozent";
        case UiLanguage::Romanian: return "Procent";
        default: return "Percentage";
      }
    case TrKey::Maze:
      switch (lang) {
        case UiLanguage::Polish: return "Labirynt";
        case UiLanguage::Spanish: return "Laberinto";
        case UiLanguage::French: return "Labyrinthe";
        case UiLanguage::German: return "Labyrinth";
        case UiLanguage::Romanian: return "Labirint";
        default: return "Maze";
      }
    case TrKey::ScreenOff:
      switch (lang) {
        case UiLanguage::Polish: return "Wylacz";
        case UiLanguage::Spanish: return "Apagar pant.";
        case UiLanguage::French: return "Ecran eteint";
        case UiLanguage::German: return "Bildschirm aus";
        case UiLanguage::Romanian: return "Ecran oprit";
        default: return "Screen off";
      }
    case TrKey::Life:
      switch (lang) {
        case UiLanguage::Polish: return "Zycie";
        case UiLanguage::Spanish: return "Vida";
        case UiLanguage::French: return "Vie";
        case UiLanguage::German: return "Leben";
        case UiLanguage::Romanian: return "Viata";
        default: return "Life";
      }
    case TrKey::StartingWifi:
      switch (lang) {
        case UiLanguage::Polish: return "Wlaczam Wi-Fi";
        case UiLanguage::Spanish: return "Iniciando Wi-Fi";
        case UiLanguage::French: return "Demarrage Wi-Fi";
        case UiLanguage::German: return "Wi-Fi starten";
        case UiLanguage::Romanian: return "Pornire Wi-Fi";
        default: return "Starting Wi-Fi";
      }
    case TrKey::CouldNotStart:
      switch (lang) {
        case UiLanguage::Polish: return "Nie udalo sie";
        case UiLanguage::Spanish: return "No se pudo iniciar";
        case UiLanguage::French: return "Echec demarrage";
        case UiLanguage::German: return "Start fehlgeschlagen";
        case UiLanguage::Romanian: return "Nu s-a putut porni";
        default: return "Could not start";
      }
    case TrKey::Returning:
      switch (lang) {
        case UiLanguage::Polish: return "Wracam";
        case UiLanguage::Spanish: return "Volviendo";
        case UiLanguage::French: return "Retour";
        case UiLanguage::German: return "Zurueck";
        case UiLanguage::Romanian: return "Revenire";
        default: return "Returning";
      }
    case TrKey::Stopping:
      switch (lang) {
        case UiLanguage::Polish: return "Zatrzymuje";
        case UiLanguage::Spanish: return "Deteniendo";
        case UiLanguage::French: return "Arret";
        case UiLanguage::German: return "Stoppen";
        case UiLanguage::Romanian: return "Se opreste";
        default: return "Stopping";
      }
    case TrKey::PleaseWait:
      switch (lang) {
        case UiLanguage::Polish: return "Czekaj";
        case UiLanguage::Spanish: return "Espere";
        case UiLanguage::French: return "Patientez";
        case UiLanguage::German: return "Bitte warten";
        case UiLanguage::Romanian: return "Asteptati";
        default: return "Please wait";
      }
    case TrKey::WifiNotSet:
      switch (lang) {
        case UiLanguage::Polish: return "Brak Wi-Fi";
        case UiLanguage::Spanish: return "Wi-Fi no config.";
        case UiLanguage::French: return "Wi-Fi non defini";
        case UiLanguage::German: return "Wi-Fi nicht gesetzt";
        case UiLanguage::Romanian: return "Wi-Fi nesetat";
        default: return "Wi-Fi not set";
      }
    case TrKey::SettingsWifi:
      switch (lang) {
        case UiLanguage::Polish: return "Ustawienia -> Wi-Fi";
        case UiLanguage::Spanish: return "Ajustes -> Wi-Fi";
        case UiLanguage::French: return "Reglages -> Wi-Fi";
        case UiLanguage::German: return "Optionen -> Wi-Fi";
        case UiLanguage::Romanian: return "Setari -> Wi-Fi";
        default: return "Settings -> Wi-Fi";
      }
    case TrKey::Restarting:
      switch (lang) {
        case UiLanguage::Polish: return "Restartuje";
        case UiLanguage::Spanish: return "Reiniciando";
        case UiLanguage::French: return "Redemarrage";
        case UiLanguage::German: return "Neustart";
        case UiLanguage::Romanian: return "Repornire";
        default: return "Restarting";
      }
    case TrKey::Yes:
      switch (lang) {
        case UiLanguage::Polish: return "Tak";
        case UiLanguage::Spanish: return "Si";
        case UiLanguage::French: return "Oui";
        case UiLanguage::German: return "Ja";
        case UiLanguage::Romanian: return "Da";
        default: return "On";
      }
    case TrKey::No:
      switch (lang) {
        case UiLanguage::Polish: return "Nie";
        case UiLanguage::Spanish: return "No";
        case UiLanguage::French: return "Non";
        case UiLanguage::German: return "Nein";
        case UiLanguage::Romanian: return "Nu";
        default: return "Off";
      }
    case TrKey::ScreensaverTimeout:
      switch (lang) {
        case UiLanguage::Polish: return "Czas wygaszacza: ";
        case UiLanguage::Spanish: return "Tiempo: ";
        case UiLanguage::French: return "Delai: ";
        case UiLanguage::German: return "Wartezeit: ";
        case UiLanguage::Romanian: return "Timp: ";
        default: return "Timeout: ";
      }
    case TrKey::ScreensaverAutoOff:
      switch (lang) {
        case UiLanguage::Polish: return "Auto-wylacz: ";
        case UiLanguage::Spanish: return "Auto-apagar: ";
        case UiLanguage::French: return "Auto-eteindre: ";
        case UiLanguage::German: return "Auto-aus: ";
        case UiLanguage::Romanian: return "Auto-oprire: ";
        default: return "Auto power-off: ";
      }
    case TrKey::ScreensaverSleepGuard:
      switch (lang) {
        case UiLanguage::Polish: return "Ochrona snu: ";
        case UiLanguage::Spanish: return "Protec. sueno: ";
        case UiLanguage::French: return "Protec. sommeil: ";
        case UiLanguage::German: return "Schlafschutz: ";
        case UiLanguage::Romanian: return "Protectie somn: ";
        default: return "Sleep guard: ";
      }
    case TrKey::ScreensaverStyle:
      switch (lang) {
        case UiLanguage::Polish: return "Styl: ";
        case UiLanguage::Spanish: return "Estilo: ";
        case UiLanguage::French: return "Style: ";
        case UiLanguage::German: return "Stil: ";
        case UiLanguage::Romanian: return "Stil: ";
        default: return "Style: ";
      }
    case TrKey::ScreensaverPreview:
      switch (lang) {
        case UiLanguage::Polish: return ">> Podglad <<";
        case UiLanguage::Spanish: return ">> Vista previa <<";
        case UiLanguage::French: return ">> Apercu <<";
        case UiLanguage::German: return ">> Vorschau <<";
        case UiLanguage::Romanian: return ">> Previz. <<";
        default: return ">> Preview <<";
      }
    case TrKey::Stars:
      switch (lang) {
        case UiLanguage::Polish: return "Gwiazdy";
        case UiLanguage::Spanish: return "Estrellas";
        case UiLanguage::French: return "Etoiles";
        case UiLanguage::German: return "Sterne";
        case UiLanguage::Romanian: return "Stele";
        default: return "Stars";
      }
    case TrKey::MatrixRain:
      switch (lang) {
        case UiLanguage::Polish: return "Matrix";
        case UiLanguage::Spanish: return "Matrix";
        case UiLanguage::French: return "Matrix";
        case UiLanguage::German: return "Matrix";
        case UiLanguage::Romanian: return "Matrix";
        default: return "Matrix";
      }
    case TrKey::ScreensaverHint:
      switch (lang) {
        case UiLanguage::Polish: return "Nacisnij przycisk by wybudzic";
        case UiLanguage::Spanish: return "Pulsa un boton para despertar";
        case UiLanguage::French: return "Appuyez pour reveiller";
        case UiLanguage::German: return "Taste drucken zum Aufwecken";
        case UiLanguage::Romanian: return "Apasa un buton pt. trezire";
        default: return "Press button to wake";
      }
    case TrKey::Minutes1:
      switch (lang) {
        case UiLanguage::Polish: return "1 min";
        case UiLanguage::Spanish: return "1 min";
        case UiLanguage::French: return "1 min";
        case UiLanguage::German: return "1 Min";
        case UiLanguage::Romanian: return "1 min";
        default: return "1 min";
      }
    case TrKey::Minutes2:
      switch (lang) {
        case UiLanguage::Polish: return "2 min";
        case UiLanguage::Spanish: return "2 min";
        case UiLanguage::French: return "2 min";
        case UiLanguage::German: return "2 Min";
        case UiLanguage::Romanian: return "2 min";
        default: return "2 min";
      }
    case TrKey::Minutes3:
      switch (lang) {
        case UiLanguage::Polish: return "3 min";
        case UiLanguage::Spanish: return "3 min";
        case UiLanguage::French: return "3 min";
        case UiLanguage::German: return "3 Min";
        case UiLanguage::Romanian: return "3 min";
        default: return "3 min";
      }
    case TrKey::Minutes5:
      switch (lang) {
        case UiLanguage::Polish: return "5 min";
        case UiLanguage::Spanish: return "5 min";
        case UiLanguage::French: return "5 min";
        case UiLanguage::German: return "5 Min";
        case UiLanguage::Romanian: return "5 min";
        default: return "5 min";
      }
    case TrKey::Minutes10:
      switch (lang) {
        case UiLanguage::Polish: return "10 min";
        case UiLanguage::Spanish: return "10 min";
        case UiLanguage::French: return "10 min";
        case UiLanguage::German: return "10 Min";
        case UiLanguage::Romanian: return "10 min";
        default: return "10 min";
      }
    case TrKey::Minutes15:
      switch (lang) {
        case UiLanguage::Polish: return "15 min";
        case UiLanguage::Spanish: return "15 min";
        case UiLanguage::French: return "15 min";
        case UiLanguage::German: return "15 Min";
        case UiLanguage::Romanian: return "15 min";
        default: return "15 min";
      }
    case TrKey::Minutes20:
      switch (lang) {
        case UiLanguage::Polish: return "20 min";
        case UiLanguage::Spanish: return "20 min";
        case UiLanguage::French: return "20 min";
        case UiLanguage::German: return "20 Min";
        case UiLanguage::Romanian: return "20 min";
        default: return "20 min";
      }
    case TrKey::Minutes30:
      switch (lang) {
        case UiLanguage::Polish: return "30 min";
        case UiLanguage::Spanish: return "30 min";
        case UiLanguage::French: return "30 min";
        case UiLanguage::German: return "30 Min";
        case UiLanguage::Romanian: return "30 min";
        default: return "30 min";
      }
    case TrKey::Never:
      switch (lang) {
        case UiLanguage::Polish: return "Nigdy";
        case UiLanguage::Spanish: return "Nunca";
        case UiLanguage::French: return "Jamais";
        case UiLanguage::German: return "Nie";
        case UiLanguage::Romanian: return "Niciodata";
        default: return "Never";
      }
  }
  return "";
}

}  // namespace Translations


// ─── Extended keys (Phase 2) ───────────────────────────────────────────────

enum class TrKey2 : uint8_t {
  Books,
  Articles,
  FocusTimer,
  SdCardCheck,
  RssFeeds,
  CompanionSync,
  RepairFolders,
  NotNow,
  CreateFolders,
  UpdateAvailable,
  SkipForNow,
  Update,
  CredentialsCleared,
  NoNetworksFound,
  NetworkSaved,
  PasswordRequired,
  ResetToDefault,
  OwnerSaved,
  CheckingFeeds,
  Starting,
  FoldersMissing,
  ConfirmRepair,
  RepairingFolders,
  FolderRepairFailed,
  FormatFat32,
  FoldersRepaired,
  CheckingCard,
  SdNotReady,
  CopyBooksNow,
  EjectThenHoldPwr,
  RemountingSd,
  ReleasePwr,
  HoldPwrToStart,
  // Plugin system
  PluginLibrary,
  PluginInstalled,
  PluginInstall,
  PluginRemove,
  PluginBuiltIn,
  PluginCannotRemove,
  PluginInstalling,
  PluginRemoving,
  PluginRestartRequired,
  PluginNoWifi,
  PluginFetchFailed,
  PluginInstallFailed,
  PluginRemoveFailed,
  PluginLaunch,
  PluginDownloading,
  PluginFetchingRegistry,
  PluginUpdate,
  PluginNotYetAvailable,
};

namespace Translations2 {

inline const char *tr2(UiLanguage lang, TrKey2 key) {
  switch (key) {
    case TrKey2::Books:
      switch (lang) {
        case UiLanguage::Polish: return "Ksiazki";
        case UiLanguage::Spanish: return "Libros";
        case UiLanguage::French: return "Livres";
        case UiLanguage::German: return "Buecher";
        case UiLanguage::Romanian: return "Carti";
        default: return "Books";
      }
    case TrKey2::Articles:
      switch (lang) {
        case UiLanguage::Polish: return "Artykuly";
        case UiLanguage::Spanish: return "Articulos";
        case UiLanguage::French: return "Articles";
        case UiLanguage::German: return "Artikel";
        case UiLanguage::Romanian: return "Articole";
        default: return "Articles";
      }
    case TrKey2::FocusTimer:
      switch (lang) {
        case UiLanguage::Polish: return "Klepsydra";
        case UiLanguage::Spanish: return "Temporizador";
        case UiLanguage::French: return "Minuteur";
        case UiLanguage::German: return "Fokus-Timer";
        case UiLanguage::Romanian: return "Cronometru";
        default: return "Focus Timer";
      }
    case TrKey2::SdCardCheck:
      switch (lang) {
        case UiLanguage::Polish: return "Sprawdz karte SD";
        case UiLanguage::Spanish: return "Verificar SD";
        case UiLanguage::French: return "Verifier carte SD";
        case UiLanguage::German: return "SD-Karte pruefen";
        case UiLanguage::Romanian: return "Verificare card SD";
        default: return "SD card check";
      }
    case TrKey2::RssFeeds:
      switch (lang) {
        case UiLanguage::Polish: return "Kanaly RSS";
        case UiLanguage::Spanish: return "Fuentes RSS";
        case UiLanguage::French: return "Flux RSS";
        case UiLanguage::German: return "RSS-Feeds";
        case UiLanguage::Romanian: return "Fluxuri RSS";
        default: return "RSS feeds";
      }
    case TrKey2::CompanionSync:
      switch (lang) {
        case UiLanguage::Polish: return "Sync z telefonem";
        case UiLanguage::Spanish: return "Sync con movil";
        case UiLanguage::French: return "Sync telephone";
        case UiLanguage::German: return "Handy-Sync";
        case UiLanguage::Romanian: return "Sync telefon";
        default: return "Companion sync";
      }
    case TrKey2::RepairFolders:
      switch (lang) {
        case UiLanguage::Polish: return "Naprawic foldery?";
        case UiLanguage::Spanish: return "Reparar carpetas?";
        case UiLanguage::French: return "Reparer dossiers?";
        case UiLanguage::German: return "Ordner reparieren?";
        case UiLanguage::Romanian: return "Repara foldere?";
        default: return "Repair folders?";
      }
    case TrKey2::NotNow:
      switch (lang) {
        case UiLanguage::Polish: return "Nie teraz";
        case UiLanguage::Spanish: return "Ahora no";
        case UiLanguage::French: return "Pas maintenant";
        case UiLanguage::German: return "Nicht jetzt";
        case UiLanguage::Romanian: return "Nu acum";
        default: return "Not now";
      }
    case TrKey2::CreateFolders:
      switch (lang) {
        case UiLanguage::Polish: return "Utworz foldery";
        case UiLanguage::Spanish: return "Crear carpetas";
        case UiLanguage::French: return "Creer dossiers";
        case UiLanguage::German: return "Ordner erstellen";
        case UiLanguage::Romanian: return "Creeaza foldere";
        default: return "Create folders";
      }
    case TrKey2::UpdateAvailable:
      switch (lang) {
        case UiLanguage::Polish: return "Dostepna aktualizacja";
        case UiLanguage::Spanish: return "Actualizacion disponible";
        case UiLanguage::French: return "Mise a jour disponible";
        case UiLanguage::German: return "Update verfuegbar";
        case UiLanguage::Romanian: return "Actualizare disponibila";
        default: return "Update available";
      }
    case TrKey2::SkipForNow:
      switch (lang) {
        case UiLanguage::Polish: return "Pomin";
        case UiLanguage::Spanish: return "Omitir";
        case UiLanguage::French: return "Ignorer";
        case UiLanguage::German: return "Ueberspringen";
        case UiLanguage::Romanian: return "Sari peste";
        default: return "Skip for now";
      }
    case TrKey2::Update:
      switch (lang) {
        case UiLanguage::Polish: return "Aktualizuj";
        case UiLanguage::Spanish: return "Actualizar";
        case UiLanguage::French: return "Mettre a jour";
        case UiLanguage::German: return "Aktualisieren";
        case UiLanguage::Romanian: return "Actualizeaza";
        default: return "Update";
      }
    case TrKey2::CredentialsCleared:
      switch (lang) {
        case UiLanguage::Polish: return "Dane usuniete";
        case UiLanguage::Spanish: return "Credenciales borradas";
        case UiLanguage::French: return "Identifiants effaces";
        case UiLanguage::German: return "Zugangsdaten geloescht";
        case UiLanguage::Romanian: return "Credentiale sterse";
        default: return "Credentials cleared";
      }
    case TrKey2::NoNetworksFound:
      switch (lang) {
        case UiLanguage::Polish: return "Brak sieci";
        case UiLanguage::Spanish: return "Sin redes";
        case UiLanguage::French: return "Aucun reseau";
        case UiLanguage::German: return "Keine Netzwerke";
        case UiLanguage::Romanian: return "Nicio retea gasita";
        default: return "No networks found";
      }
    case TrKey2::NetworkSaved:
      switch (lang) {
        case UiLanguage::Polish: return "Siec zapisana";
        case UiLanguage::Spanish: return "Red guardada";
        case UiLanguage::French: return "Reseau enregistre";
        case UiLanguage::German: return "Netzwerk gespeichert";
        case UiLanguage::Romanian: return "Retea salvata";
        default: return "Network saved";
      }
    case TrKey2::PasswordRequired:
      switch (lang) {
        case UiLanguage::Polish: return "Wymagane haslo";
        case UiLanguage::Spanish: return "Contrasena requerida";
        case UiLanguage::French: return "Mot de passe requis";
        case UiLanguage::German: return "Passwort erforderlich";
        case UiLanguage::Romanian: return "Parola necesara";
        default: return "Password required";
      }
    case TrKey2::ResetToDefault:
      switch (lang) {
        case UiLanguage::Polish: return "Przywrocono domyslne";
        case UiLanguage::Spanish: return "Restablecido";
        case UiLanguage::French: return "Reinitialise";
        case UiLanguage::German: return "Zurueckgesetzt";
        case UiLanguage::Romanian: return "Resetat la implicit";
        default: return "Reset to default";
      }
    case TrKey2::OwnerSaved:
      switch (lang) {
        case UiLanguage::Polish: return "Zapisano";
        case UiLanguage::Spanish: return "Guardado";
        case UiLanguage::French: return "Enregistre";
        case UiLanguage::German: return "Gespeichert";
        case UiLanguage::Romanian: return "Salvat";
        default: return "Owner saved";
      }
    case TrKey2::CheckingFeeds:
      switch (lang) {
        case UiLanguage::Polish: return "Sprawdzam kanaly";
        case UiLanguage::Spanish: return "Verificando fuentes";
        case UiLanguage::French: return "Verification flux";
        case UiLanguage::German: return "Feeds pruefen";
        case UiLanguage::Romanian: return "Verificare fluxuri";
        default: return "Checking feeds";
      }
    case TrKey2::Starting:
      switch (lang) {
        case UiLanguage::Polish: return "Uruchamiam";
        case UiLanguage::Spanish: return "Iniciando";
        case UiLanguage::French: return "Demarrage";
        case UiLanguage::German: return "Starten";
        case UiLanguage::Romanian: return "Pornire";
        default: return "Starting";
      }
    case TrKey2::FoldersMissing:
      switch (lang) {
        case UiLanguage::Polish: return "Brak folderow";
        case UiLanguage::Spanish: return "Faltan carpetas";
        case UiLanguage::French: return "Dossiers manquants";
        case UiLanguage::German: return "Ordner fehlen";
        case UiLanguage::Romanian: return "Foldere lipsa";
        default: return "Folders missing";
      }
    case TrKey2::ConfirmRepair:
      switch (lang) {
        case UiLanguage::Polish: return "Potwierdz naprawe";
        case UiLanguage::Spanish: return "Confirmar reparacion";
        case UiLanguage::French: return "Confirmer reparation";
        case UiLanguage::German: return "Reparatur bestaetigen";
        case UiLanguage::Romanian: return "Confirma repararea";
        default: return "Confirm repair";
      }
    case TrKey2::RepairingFolders:
      switch (lang) {
        case UiLanguage::Polish: return "Naprawiam foldery";
        case UiLanguage::Spanish: return "Reparando carpetas";
        case UiLanguage::French: return "Reparation dossiers";
        case UiLanguage::German: return "Ordner reparieren";
        case UiLanguage::Romanian: return "Reparare foldere";
        default: return "Repairing folders";
      }
    case TrKey2::FolderRepairFailed:
      switch (lang) {
        case UiLanguage::Polish: return "Naprawa nieudana";
        case UiLanguage::Spanish: return "Reparacion fallida";
        case UiLanguage::French: return "Reparation echouee";
        case UiLanguage::German: return "Reparatur fehlgeschlagen";
        case UiLanguage::Romanian: return "Reparare esuata";
        default: return "Folder repair failed";
      }
    case TrKey2::FormatFat32:
      switch (lang) {
        case UiLanguage::Polish: return "Sformatuj FAT32 MBR";
        case UiLanguage::Spanish: return "Formatear FAT32 MBR";
        case UiLanguage::French: return "Formater FAT32 MBR";
        case UiLanguage::German: return "FAT32 MBR formatieren";
        case UiLanguage::Romanian: return "Formateaza FAT32 MBR";
        default: return "Format FAT32 MBR";
      }
    case TrKey2::FoldersRepaired:
      switch (lang) {
        case UiLanguage::Polish: return "Foldery naprawione";
        case UiLanguage::Spanish: return "Carpetas reparadas";
        case UiLanguage::French: return "Dossiers repares";
        case UiLanguage::German: return "Ordner repariert";
        case UiLanguage::Romanian: return "Foldere reparate";
        default: return "Folders repaired";
      }
    case TrKey2::CheckingCard:
      switch (lang) {
        case UiLanguage::Polish: return "Sprawdzam karte";
        case UiLanguage::Spanish: return "Verificando tarjeta";
        case UiLanguage::French: return "Verification carte";
        case UiLanguage::German: return "Karte pruefen";
        case UiLanguage::Romanian: return "Verificare card";
        default: return "Checking card";
      }
    case TrKey2::SdNotReady:
      switch (lang) {
        case UiLanguage::Polish: return "SD niegotowa";
        case UiLanguage::Spanish: return "SD no lista";
        case UiLanguage::French: return "SD non prete";
        case UiLanguage::German: return "SD nicht bereit";
        case UiLanguage::Romanian: return "SD nu e gata";
        default: return "SD not ready";
      }
    case TrKey2::CopyBooksNow:
      switch (lang) {
        case UiLanguage::Polish: return "Kopiuj ksiazki";
        case UiLanguage::Spanish: return "Copie libros ahora";
        case UiLanguage::French: return "Copiez livres";
        case UiLanguage::German: return "Buecher kopieren";
        case UiLanguage::Romanian: return "Copiati cartile";
        default: return "Copy books now";
      }
    case TrKey2::EjectThenHoldPwr:
      switch (lang) {
        case UiLanguage::Polish: return "Wysun i przytrzymaj PWR";
        case UiLanguage::Spanish: return "Expulsar, mantener PWR";
        case UiLanguage::French: return "Ejecter puis maintenir PWR";
        case UiLanguage::German: return "Auswerfen, PWR halten";
        case UiLanguage::Romanian: return "Scoate si tine PWR";
        default: return "Eject then hold PWR";
      }
    case TrKey2::RemountingSd:
      switch (lang) {
        case UiLanguage::Polish: return "Ponowne montowanie SD";
        case UiLanguage::Spanish: return "Remontando SD";
        case UiLanguage::French: return "Remontage SD";
        case UiLanguage::German: return "SD neu einbinden";
        case UiLanguage::Romanian: return "Remontare SD";
        default: return "Remounting SD";
      }
    case TrKey2::ReleasePwr:
      switch (lang) {
        case UiLanguage::Polish: return "Pusc PWR";
        case UiLanguage::Spanish: return "Suelte PWR";
        case UiLanguage::French: return "Relacher PWR";
        case UiLanguage::German: return "PWR loslassen";
        case UiLanguage::Romanian: return "Elibereaza PWR";
        default: return "Release PWR";
      }
    case TrKey2::HoldPwrToStart:
      switch (lang) {
        case UiLanguage::Polish: return "Przytrzymaj PWR aby wlaczyc";
        case UiLanguage::Spanish: return "Mantenga PWR para iniciar";
        case UiLanguage::French: return "Maintenir PWR pour demarrer";
        case UiLanguage::German: return "PWR halten zum Starten";
        case UiLanguage::Romanian: return "Tine PWR pt. pornire";
        default: return "Hold PWR to start";
      }
    case TrKey2::PluginLibrary:
      switch (lang) {
        case UiLanguage::Polish: return "Biblioteka funkcji";
        case UiLanguage::Spanish: return "Biblioteca de funciones";
        case UiLanguage::French: return "Bibliotheque de fonctions";
        case UiLanguage::German: return "Funktionsbibliothek";
        case UiLanguage::Romanian: return "Biblioteca de functii";
        default: return "Function library";
      }
    case TrKey2::PluginInstalled:
      switch (lang) {
        case UiLanguage::Polish: return "[zainstalowany]";
        case UiLanguage::Spanish: return "[instalado]";
        case UiLanguage::French: return "[installe]";
        case UiLanguage::German: return "[installiert]";
        case UiLanguage::Romanian: return "[instalat]";
        default: return "[installed]";
      }
    case TrKey2::PluginInstall:
      switch (lang) {
        case UiLanguage::Polish: return "Zainstaluj: ";
        case UiLanguage::Spanish: return "Instalar: ";
        case UiLanguage::French: return "Installer: ";
        case UiLanguage::German: return "Installieren: ";
        case UiLanguage::Romanian: return "Instaleaza: ";
        default: return "Install: ";
      }
    case TrKey2::PluginRemove:
      switch (lang) {
        case UiLanguage::Polish: return "Usun: ";
        case UiLanguage::Spanish: return "Quitar: ";
        case UiLanguage::French: return "Retirer: ";
        case UiLanguage::German: return "Entfernen: ";
        case UiLanguage::Romanian: return "Elimina: ";
        default: return "Remove: ";
      }
    case TrKey2::PluginBuiltIn:
      switch (lang) {
        case UiLanguage::Polish: return "Wbudowany";
        case UiLanguage::Spanish: return "Integrado";
        case UiLanguage::French: return "Integre";
        case UiLanguage::German: return "Eingebaut";
        case UiLanguage::Romanian: return "Integrat";
        default: return "Built-in";
      }
    case TrKey2::PluginCannotRemove:
      switch (lang) {
        case UiLanguage::Polish: return "Nie mozna usunac";
        case UiLanguage::Spanish: return "No se puede quitar";
        case UiLanguage::French: return "Impossible de retirer";
        case UiLanguage::German: return "Kann nicht entfernt werden";
        case UiLanguage::Romanian: return "Nu se poate elimina";
        default: return "Cannot remove";
      }
    case TrKey2::PluginInstalling:
      switch (lang) {
        case UiLanguage::Polish: return "Instalowanie...";
        case UiLanguage::Spanish: return "Instalando...";
        case UiLanguage::French: return "Installation...";
        case UiLanguage::German: return "Installiere...";
        case UiLanguage::Romanian: return "Se instaleaza...";
        default: return "Installing...";
      }
    case TrKey2::PluginRemoving:
      switch (lang) {
        case UiLanguage::Polish: return "Usuwanie...";
        case UiLanguage::Spanish: return "Quitando...";
        case UiLanguage::French: return "Retrait...";
        case UiLanguage::German: return "Entferne...";
        case UiLanguage::Romanian: return "Se elimina...";
        default: return "Removing...";
      }
    case TrKey2::PluginRestartRequired:
      switch (lang) {
        case UiLanguage::Polish: return "Restart za chwile...";
        case UiLanguage::Spanish: return "Reiniciando...";
        case UiLanguage::French: return "Redemarrage...";
        case UiLanguage::German: return "Neustart...";
        case UiLanguage::Romanian: return "Repornire...";
        default: return "Restarting...";
      }
    case TrKey2::PluginNoWifi:
      switch (lang) {
        case UiLanguage::Polish: return "Wymagane Wi-Fi";
        case UiLanguage::Spanish: return "Wi-Fi requerido";
        case UiLanguage::French: return "Wi-Fi requis";
        case UiLanguage::German: return "Wi-Fi erforderlich";
        case UiLanguage::Romanian: return "Wi-Fi necesar";
        default: return "Wi-Fi required";
      }
    case TrKey2::PluginFetchFailed:
      switch (lang) {
        case UiLanguage::Polish: return "Blad pobierania";
        case UiLanguage::Spanish: return "Error de descarga";
        case UiLanguage::French: return "Erreur de telechargement";
        case UiLanguage::German: return "Download fehlgeschlagen";
        case UiLanguage::Romanian: return "Eroare descarcare";
        default: return "Download failed";
      }
    case TrKey2::PluginInstallFailed:
      switch (lang) {
        case UiLanguage::Polish: return "Instalacja nieudana";
        case UiLanguage::Spanish: return "Instalacion fallida";
        case UiLanguage::French: return "Echec installation";
        case UiLanguage::German: return "Installation fehlgeschlagen";
        case UiLanguage::Romanian: return "Instalare esuata";
        default: return "Install failed";
      }
    case TrKey2::PluginRemoveFailed:
      switch (lang) {
        case UiLanguage::Polish: return "Usuwanie nieudane";
        case UiLanguage::Spanish: return "Error al quitar";
        case UiLanguage::French: return "Echec du retrait";
        case UiLanguage::German: return "Entfernen fehlgeschlagen";
        case UiLanguage::Romanian: return "Eliminare esuata";
        default: return "Remove failed";
      }
    case TrKey2::PluginLaunch:
      switch (lang) {
        case UiLanguage::Polish: return "Uruchom";
        case UiLanguage::Spanish: return "Ejecutar";
        case UiLanguage::French: return "Lancer";
        case UiLanguage::German: return "Starten";
        case UiLanguage::Romanian: return "Lanseaza";
        default: return "Launch";
      }
    case TrKey2::PluginDownloading:
      switch (lang) {
        case UiLanguage::Polish: return "Pobieranie...";
        case UiLanguage::Spanish: return "Descargando...";
        case UiLanguage::French: return "Telechargement...";
        case UiLanguage::German: return "Herunterladen...";
        case UiLanguage::Romanian: return "Descarca...";
        default: return "Downloading...";
      }
    case TrKey2::PluginFetchingRegistry:
      switch (lang) {
        case UiLanguage::Polish: return "Pobieranie listy...";
        case UiLanguage::Spanish: return "Obteniendo lista...";
        case UiLanguage::French: return "Chargement liste...";
        case UiLanguage::German: return "Liste laden...";
        case UiLanguage::Romanian: return "Incarcare lista...";
        default: return "Fetching list...";
      }
    case TrKey2::PluginUpdate:
      switch (lang) {
        case UiLanguage::Polish: return "[aktualizacja]";
        case UiLanguage::Spanish: return "[actualizar]";
        case UiLanguage::French: return "[mise a jour]";
        case UiLanguage::German: return "[aktualisieren]";
        case UiLanguage::Romanian: return "[actualizare]";
        default: return "[update]";
      }
    case TrKey2::PluginNotYetAvailable:
      switch (lang) {
        case UiLanguage::Polish: return "Wkrotce dostepny";
        case UiLanguage::Spanish: return "Disponible pronto";
        case UiLanguage::French: return "Bientot disponible";
        case UiLanguage::German: return "Bald verfuegbar";
        case UiLanguage::Romanian: return "Disponibil curand";
        default: return "Coming soon";
      }
  }
  return "";
}

}  // namespace Translations2


// ─── Extended keys (Phase 3) — last batch of polish()-helper strings ───────

enum class TrKey3 : uint8_t {
  SavePointAdded,
  NameBookmark,
  EnterNamePrompt,
  BookmarkAdded,
  ReadingSettings,
  AdvancedModeColon,
  PresetsLabel,
  TutorialLabel,
  ColorRed,
  ColorBlue,
  ColorGreen,
  ColorYellow,
  ColorOrange,
  ColorPurple,
  PacingNone,
  PacingLight,
  PacingMedium,
  PacingStrong,
  PacingVeryStrong,
  SaveBtnColon,
  FocusColorColon,
  HelpQColon,
  NavigationColon,
  ScanCode,
  InstallApp,
  TapContinue,
  TutorialRsvpDesc,
  SpeedLabel,
  TutorialSpeedDesc,
  PauseLabel,
  TutorialPauseDesc,
  TutorialMenuDesc,
  HelpQLabel,
  TutorialHelpDesc,
  ChannelStaging,
  ChannelProduction,
  ButtonsLabel,
  PercentComplete,
  ReadFromPlace,
  DeleteBookLabel,
  ErrorLabel,
  DeleteConfirmColon,
  NoGoBack,
  YesDelete,
  DeletedLabel,
  CannotDelete,
  AddSavePoint,
  DeleteSpace,
  OpenBookFirst,
  BookNotFound,
  CannotOpen,
  ActivePlugins,
  NoActivePlugins,
  PluginEnabledTag,
  PluginDisabledTag,
  DisablePlugin,
  EnablePlugin,
  SaveCurrentPreset,
  PresetLimitReachedParen,
  PresetNameLabel,
  InvalidName,
  SavedLabel,
  LimitReachedShort,
  SdCardErrorLabel,
  LoadedLabel,
  PresetLoadError,
  ApplyColon,
  DeletePresetColon,
  PresetDeleteFailed,
  BackWifiHeader,
  BackSyncHeader,
  UsbBackHint,
  ConnectUsbCable,
  SdVisibleOnPhone,
  VolumeDown,
  VolumeUp,
  PositionLabel,
  VolumeAbbrev,
};

namespace Translations3 {

inline const char *tr3(UiLanguage lang, TrKey3 key) {
  switch (key) {
    case TrKey3::SavePointAdded:
      switch (lang) {
        case UiLanguage::Polish: return "Punkt zapisu dodany";
        case UiLanguage::Spanish: return "Punto guardado agregado";
        case UiLanguage::French: return "Point de sauvegarde ajoute";
        case UiLanguage::German: return "Lesezeichen hinzugefuegt";
        case UiLanguage::Romanian: return "Punct salvat adaugat";
        default: return "Save point added";
      }
    case TrKey3::NameBookmark:
      switch (lang) {
        case UiLanguage::Polish: return "Nazwij zakladke";
        case UiLanguage::Spanish: return "Nombra el marcador";
        case UiLanguage::French: return "Nommer le signet";
        case UiLanguage::German: return "Lesezeichen benennen";
        case UiLanguage::Romanian: return "Numeste marcaj";
        default: return "Name bookmark";
      }
    case TrKey3::EnterNamePrompt:
      switch (lang) {
        case UiLanguage::Polish: return "Wpisz nazwe:";
        case UiLanguage::Spanish: return "Escribe nombre:";
        case UiLanguage::French: return "Entrez le nom :";
        case UiLanguage::German: return "Namen eingeben:";
        case UiLanguage::Romanian: return "Introdu numele:";
        default: return "Enter name:";
      }
    case TrKey3::BookmarkAdded:
      switch (lang) {
        case UiLanguage::Polish: return "Zakladka dodana";
        case UiLanguage::Spanish: return "Marcador agregado";
        case UiLanguage::French: return "Signet ajoute";
        case UiLanguage::German: return "Lesezeichen hinzugefuegt";
        case UiLanguage::Romanian: return "Marcaj adaugat";
        default: return "Bookmark added";
      }
    case TrKey3::ReadingSettings:
      switch (lang) {
        case UiLanguage::Polish: return "Czytanie";
        case UiLanguage::Spanish: return "Lectura";
        case UiLanguage::French: return "Lecture";
        case UiLanguage::German: return "Lesen";
        case UiLanguage::Romanian: return "Citire";
        default: return "Reading";
      }
    case TrKey3::AdvancedModeColon:
      switch (lang) {
        case UiLanguage::Polish: return "Tryb zaawansowany: ";
        case UiLanguage::Spanish: return "Modo avanzado: ";
        case UiLanguage::French: return "Mode avance : ";
        case UiLanguage::German: return "Erweiterter Modus: ";
        case UiLanguage::Romanian: return "Mod avansat: ";
        default: return "Advanced mode: ";
      }
    case TrKey3::PresetsLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Presety";
        case UiLanguage::Spanish: return "Preajustes";
        case UiLanguage::French: return "Prereglages";
        case UiLanguage::German: return "Voreinstellungen";
        case UiLanguage::Romanian: return "Presetari";
        default: return "Presets";
      }
    case TrKey3::TutorialLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Samouczek";
        case UiLanguage::Spanish: return "Tutorial";
        case UiLanguage::French: return "Tutoriel";
        case UiLanguage::German: return "Anleitung";
        case UiLanguage::Romanian: return "Tutorial";
        default: return "Tutorial";
      }
    case TrKey3::ColorRed:
      switch (lang) {
        case UiLanguage::Polish: return "Czerwony";
        case UiLanguage::Spanish: return "Rojo";
        case UiLanguage::French: return "Rouge";
        case UiLanguage::German: return "Rot";
        case UiLanguage::Romanian: return "Rosu";
        default: return "Red";
      }
    case TrKey3::ColorBlue:
      switch (lang) {
        case UiLanguage::Polish: return "Niebieski";
        case UiLanguage::Spanish: return "Azul";
        case UiLanguage::French: return "Bleu";
        case UiLanguage::German: return "Blau";
        case UiLanguage::Romanian: return "Albastru";
        default: return "Blue";
      }
    case TrKey3::ColorGreen:
      switch (lang) {
        case UiLanguage::Polish: return "Zielony";
        case UiLanguage::Spanish: return "Verde";
        case UiLanguage::French: return "Vert";
        case UiLanguage::German: return "Gruen";
        case UiLanguage::Romanian: return "Verde";
        default: return "Green";
      }
    case TrKey3::ColorYellow:
      switch (lang) {
        case UiLanguage::Polish: return "Zolty";
        case UiLanguage::Spanish: return "Amarillo";
        case UiLanguage::French: return "Jaune";
        case UiLanguage::German: return "Gelb";
        case UiLanguage::Romanian: return "Galben";
        default: return "Yellow";
      }
    case TrKey3::ColorOrange:
      switch (lang) {
        case UiLanguage::Polish: return "Pomaranczowy";
        case UiLanguage::Spanish: return "Naranja";
        case UiLanguage::French: return "Orange";
        case UiLanguage::German: return "Orange";
        case UiLanguage::Romanian: return "Portocaliu";
        default: return "Orange";
      }
    case TrKey3::ColorPurple:
      switch (lang) {
        case UiLanguage::Polish: return "Fioletowy";
        case UiLanguage::Spanish: return "Morado";
        case UiLanguage::French: return "Violet";
        case UiLanguage::German: return "Violett";
        case UiLanguage::Romanian: return "Violet";
        default: return "Purple";
      }
    case TrKey3::PacingNone:
      switch (lang) {
        case UiLanguage::Polish: return "Brak (0 ms)";
        case UiLanguage::Spanish: return "Ninguno (0 ms)";
        case UiLanguage::French: return "Aucun (0 ms)";
        case UiLanguage::German: return "Keine (0 ms)";
        case UiLanguage::Romanian: return "Fara (0 ms)";
        default: return "None (0 ms)";
      }
    case TrKey3::PacingLight:
      switch (lang) {
        case UiLanguage::Polish: return "Lekkie (100 ms)";
        case UiLanguage::Spanish: return "Ligero (100 ms)";
        case UiLanguage::French: return "Leger (100 ms)";
        case UiLanguage::German: return "Leicht (100 ms)";
        case UiLanguage::Romanian: return "Usor (100 ms)";
        default: return "Light (100 ms)";
      }
    case TrKey3::PacingMedium:
      switch (lang) {
        case UiLanguage::Polish: return "Srednie (200 ms)";
        case UiLanguage::Spanish: return "Medio (200 ms)";
        case UiLanguage::French: return "Moyen (200 ms)";
        case UiLanguage::German: return "Mittel (200 ms)";
        case UiLanguage::Romanian: return "Mediu (200 ms)";
        default: return "Medium (200 ms)";
      }
    case TrKey3::PacingStrong:
      switch (lang) {
        case UiLanguage::Polish: return "Mocne (300 ms)";
        case UiLanguage::Spanish: return "Fuerte (300 ms)";
        case UiLanguage::French: return "Fort (300 ms)";
        case UiLanguage::German: return "Stark (300 ms)";
        case UiLanguage::Romanian: return "Puternic (300 ms)";
        default: return "Strong (300 ms)";
      }
    case TrKey3::PacingVeryStrong:
      switch (lang) {
        case UiLanguage::Polish: return "Bardzo mocne (400 ms)";
        case UiLanguage::Spanish: return "Muy fuerte (400 ms)";
        case UiLanguage::French: return "Tres fort (400 ms)";
        case UiLanguage::German: return "Sehr stark (400 ms)";
        case UiLanguage::Romanian: return "Foarte puternic (400 ms)";
        default: return "Very strong (400 ms)";
      }
    case TrKey3::SaveBtnColon:
      switch (lang) {
        case UiLanguage::Polish: return "Przycisk zapisu: ";
        case UiLanguage::Spanish: return "Boton guardar: ";
        case UiLanguage::French: return "Bouton sauveg. : ";
        case UiLanguage::German: return "Speicherbtn.: ";
        case UiLanguage::Romanian: return "Buton salvare: ";
        default: return "Save btn: ";
      }
    case TrKey3::FocusColorColon:
      switch (lang) {
        case UiLanguage::Polish: return "Kolor litery: ";
        case UiLanguage::Spanish: return "Color letra: ";
        case UiLanguage::French: return "Couleur lettre : ";
        case UiLanguage::German: return "Buchstabenfarbe: ";
        case UiLanguage::Romanian: return "Culoare litera: ";
        default: return "Focus color: ";
      }
    case TrKey3::HelpQColon:
      switch (lang) {
        case UiLanguage::Polish: return "Pomoc (?): ";
        case UiLanguage::Spanish: return "Ayuda (?): ";
        case UiLanguage::French: return "Aide (?) : ";
        case UiLanguage::German: return "Hilfe (?): ";
        case UiLanguage::Romanian: return "Ajutor (?): ";
        default: return "Help (?): ";
      }
    case TrKey3::NavigationColon:
      switch (lang) {
        case UiLanguage::Polish: return "Nawigacja: ";
        case UiLanguage::Spanish: return "Navegacion: ";
        case UiLanguage::French: return "Navigation : ";
        case UiLanguage::German: return "Navigation: ";
        case UiLanguage::Romanian: return "Navigare: ";
        default: return "Navigation: ";
      }
    case TrKey3::ScanCode:
      switch (lang) {
        case UiLanguage::Polish: return "Zeskanuj kod";
        case UiLanguage::Spanish: return "Escanea el codigo";
        case UiLanguage::French: return "Scannez le code";
        case UiLanguage::German: return "Code scannen";
        case UiLanguage::Romanian: return "Scaneaza codul";
        default: return "Scan the code";
      }
    case TrKey3::InstallApp:
      switch (lang) {
        case UiLanguage::Polish: return "Zainstaluj aplikacje";
        case UiLanguage::Spanish: return "Instala la app";
        case UiLanguage::French: return "Installez l'app";
        case UiLanguage::German: return "App installieren";
        case UiLanguage::Romanian: return "Instaleaza aplicatia";
        default: return "Install the app";
      }
    case TrKey3::TapContinue:
      switch (lang) {
        case UiLanguage::Polish: return "Dotknij ekranu, by przejsc dalej";
        case UiLanguage::Spanish: return "Toca la pantalla para continuar";
        case UiLanguage::French: return "Touchez l'ecran pour continuer";
        case UiLanguage::German: return "Bildschirm beruehren zum Fortfahren";
        case UiLanguage::Romanian: return "Atinge ecranul pentru a continua";
        default: return "Tap the screen to continue";
      }
    case TrKey3::TutorialRsvpDesc:
      switch (lang) {
        case UiLanguage::Polish: return "Slowa jedno po drugim. Litera ORP kieruje wzrok.";
        case UiLanguage::Spanish: return "Palabras una por una. La letra ORP guia tu vista.";
        case UiLanguage::French: return "Mots un par un. La lettre ORP guide votre regard.";
        case UiLanguage::German: return "Woerter eins nach dem anderen. Der ORP-Buchstabe fuehrt den Blick.";
        case UiLanguage::Romanian: return "Cuvinte unul cate unul. Litera ORP iti ghideaza privirea.";
        default: return "Words one at a time. ORP letter guides your eye.";
      }
    case TrKey3::SpeedLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Tempo";
        case UiLanguage::Spanish: return "Velocidad";
        case UiLanguage::French: return "Vitesse";
        case UiLanguage::German: return "Tempo";
        case UiLanguage::Romanian: return "Viteza";
        default: return "Speed";
      }
    case TrKey3::TutorialSpeedDesc:
      switch (lang) {
        case UiLanguage::Polish: return "Przytrzymaj + gora/dol: zmiana predkosci.";
        case UiLanguage::Spanish: return "Manten + arriba/abajo: cambia velocidad.";
        case UiLanguage::French: return "Maintenir + haut/bas : change la vitesse.";
        case UiLanguage::German: return "Halten + hoch/runter: Tempo aendern.";
        case UiLanguage::Romanian: return "Tine apasat + sus/jos: schimba viteza.";
        default: return "Hold + up/down: change speed.";
      }
    case TrKey3::PauseLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Pauza";
        case UiLanguage::Spanish: return "Pausa";
        case UiLanguage::French: return "Pause";
        case UiLanguage::German: return "Pause";
        case UiLanguage::Romanian: return "Pauza";
        default: return "Pause";
      }
    case TrKey3::TutorialPauseDesc:
      switch (lang) {
        case UiLanguage::Polish: return "Dotknij ekranu by pauzowac/wznowic.";
        case UiLanguage::Spanish: return "Toca la pantalla para pausar/reanudar.";
        case UiLanguage::French: return "Touchez l'ecran pour pause/reprise.";
        case UiLanguage::German: return "Bildschirm beruehren zum Pausieren/Fortsetzen.";
        case UiLanguage::Romanian: return "Atinge ecranul pentru pauza/reluare.";
        default: return "Tap screen to pause/resume.";
      }
    case TrKey3::TutorialMenuDesc:
      switch (lang) {
        case UiLanguage::Polish: return "Przycisk z boku otwiera menu.";
        case UiLanguage::Spanish: return "El boton lateral abre el menu.";
        case UiLanguage::French: return "Le bouton lateral ouvre le menu.";
        case UiLanguage::German: return "Seitentaste oeffnet das Menue.";
        case UiLanguage::Romanian: return "Butonul lateral deschide meniul.";
        default: return "Side button opens the menu.";
      }
    case TrKey3::HelpQLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Pomoc ?";
        case UiLanguage::Spanish: return "Ayuda ?";
        case UiLanguage::French: return "Aide ?";
        case UiLanguage::German: return "Hilfe ?";
        case UiLanguage::Romanian: return "Ajutor ?";
        default: return "Help ?";
      }
    case TrKey3::TutorialHelpDesc:
      switch (lang) {
        case UiLanguage::Polish: return "W ustaw. Ekran/Tempo: boczny przycisk pokazuje opis.";
        case UiLanguage::Spanish: return "En Pantalla/Ritmo: el boton lateral muestra info.";
        case UiLanguage::French: return "Dans Affichage/Rythme : le bouton lateral affiche des infos.";
        case UiLanguage::German: return "In Anzeige/Tempo: Seitentaste zeigt Infos.";
        case UiLanguage::Romanian: return "In Afisaj/Ritm: butonul lateral arata info.";
        default: return "In Display/Pacing settings: side button shows info.";
      }
    case TrKey3::ChannelStaging:
      switch (lang) {
        case UiLanguage::Polish: return "Testowy";
        case UiLanguage::Spanish: return "Pruebas";
        case UiLanguage::French: return "Test";
        case UiLanguage::German: return "Testkanal";
        case UiLanguage::Romanian: return "Testare";
        default: return "Staging";
      }
    case TrKey3::ChannelProduction:
      switch (lang) {
        case UiLanguage::Polish: return "Produkcyjny";
        case UiLanguage::Spanish: return "Produccion";
        case UiLanguage::French: return "Production";
        case UiLanguage::German: return "Produktion";
        case UiLanguage::Romanian: return "Productie";
        default: return "Production";
      }
    case TrKey3::ButtonsLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Przyciski";
        case UiLanguage::Spanish: return "Botones";
        case UiLanguage::French: return "Boutons";
        case UiLanguage::German: return "Tasten";
        case UiLanguage::Romanian: return "Butoane";
        default: return "Buttons";
      }
    case TrKey3::PercentComplete:
      switch (lang) {
        case UiLanguage::Polish: return "ukonczone";
        case UiLanguage::Spanish: return "completado";
        case UiLanguage::French: return "termine";
        case UiLanguage::German: return "abgeschlossen";
        case UiLanguage::Romanian: return "finalizat";
        default: return "complete";
      }
    case TrKey3::ReadFromPlace:
      switch (lang) {
        case UiLanguage::Polish: return "Czytaj od miejsca";
        case UiLanguage::Spanish: return "Leer desde el punto";
        case UiLanguage::French: return "Lire depuis l'endroit";
        case UiLanguage::German: return "Weiterlesen ab Stelle";
        case UiLanguage::Romanian: return "Citeste de la loc";
        default: return "Read from place";
      }
    case TrKey3::DeleteBookLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Usun ksiazke";
        case UiLanguage::Spanish: return "Eliminar libro";
        case UiLanguage::French: return "Supprimer le livre";
        case UiLanguage::German: return "Buch loeschen";
        case UiLanguage::Romanian: return "Sterge cartea";
        default: return "Delete book";
      }
    case TrKey3::ErrorLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Blad";
        case UiLanguage::Spanish: return "Error";
        case UiLanguage::French: return "Erreur";
        case UiLanguage::German: return "Fehler";
        case UiLanguage::Romanian: return "Eroare";
        default: return "Error";
      }
    case TrKey3::DeleteConfirmColon:
      switch (lang) {
        case UiLanguage::Polish: return "Usunac: ";
        case UiLanguage::Spanish: return "Eliminar: ";
        case UiLanguage::French: return "Supprimer : ";
        case UiLanguage::German: return "Loeschen: ";
        case UiLanguage::Romanian: return "Sterge: ";
        default: return "Delete: ";
      }
    case TrKey3::NoGoBack:
      switch (lang) {
        case UiLanguage::Polish: return "Nie, wroc";
        case UiLanguage::Spanish: return "No, volver";
        case UiLanguage::French: return "Non, retour";
        case UiLanguage::German: return "Nein, zurueck";
        case UiLanguage::Romanian: return "Nu, inapoi";
        default: return "No, go back";
      }
    case TrKey3::YesDelete:
      switch (lang) {
        case UiLanguage::Polish: return "Tak, usun";
        case UiLanguage::Spanish: return "Si, eliminar";
        case UiLanguage::French: return "Oui, supprimer";
        case UiLanguage::German: return "Ja, loeschen";
        case UiLanguage::Romanian: return "Da, sterge";
        default: return "Yes, delete";
      }
    case TrKey3::DeletedLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Usunieto";
        case UiLanguage::Spanish: return "Eliminado";
        case UiLanguage::French: return "Supprime";
        case UiLanguage::German: return "Geloescht";
        case UiLanguage::Romanian: return "Sters";
        default: return "Deleted";
      }
    case TrKey3::CannotDelete:
      switch (lang) {
        case UiLanguage::Polish: return "Nie mozna usunac";
        case UiLanguage::Spanish: return "No se puede eliminar";
        case UiLanguage::French: return "Impossible de supprimer";
        case UiLanguage::German: return "Loeschen nicht moeglich";
        case UiLanguage::Romanian: return "Nu se poate sterge";
        default: return "Cannot delete";
      }
    case TrKey3::AddSavePoint:
      switch (lang) {
        case UiLanguage::Polish: return "+ Dodaj punkt zapisu";
        case UiLanguage::Spanish: return "+ Agregar punto";
        case UiLanguage::French: return "+ Ajouter un point";
        case UiLanguage::German: return "+ Speicherpunkt hinzu.";
        case UiLanguage::Romanian: return "+ Adauga punct";
        default: return "+ Add save point";
      }
    case TrKey3::DeleteSpace:
      switch (lang) {
        case UiLanguage::Polish: return "Usun ";
        case UiLanguage::Spanish: return "Eliminar ";
        case UiLanguage::French: return "Supprimer ";
        case UiLanguage::German: return "Loeschen ";
        case UiLanguage::Romanian: return "Sterge ";
        default: return "Delete ";
      }
    case TrKey3::OpenBookFirst:
      switch (lang) {
        case UiLanguage::Polish: return "Najpierw otworz ksiazke";
        case UiLanguage::Spanish: return "Primero abre un libro";
        case UiLanguage::French: return "Ouvrez d'abord un livre";
        case UiLanguage::German: return "Zuerst ein Buch oeffnen";
        case UiLanguage::Romanian: return "Deschide mai intai o carte";
        default: return "Open a book first";
      }
    case TrKey3::BookNotFound:
      switch (lang) {
        case UiLanguage::Polish: return "Ksiazka nie znaleziona";
        case UiLanguage::Spanish: return "Libro no encontrado";
        case UiLanguage::French: return "Livre introuvable";
        case UiLanguage::German: return "Buch nicht gefunden";
        case UiLanguage::Romanian: return "Cartea nu a fost gasita";
        default: return "Book not found";
      }
    case TrKey3::CannotOpen:
      switch (lang) {
        case UiLanguage::Polish: return "Nie mozna otworzyc";
        case UiLanguage::Spanish: return "No se puede abrir";
        case UiLanguage::French: return "Impossible d'ouvrir";
        case UiLanguage::German: return "Oeffnen nicht moeglich";
        case UiLanguage::Romanian: return "Nu se poate deschide";
        default: return "Cannot open";
      }
    case TrKey3::ActivePlugins:
      switch (lang) {
        case UiLanguage::Polish: return "Aktywne";
        case UiLanguage::Spanish: return "Activos";
        case UiLanguage::French: return "Actifs";
        case UiLanguage::German: return "Aktiv";
        case UiLanguage::Romanian: return "Active";
        default: return "Active";
      }
    case TrKey3::NoActivePlugins:
      switch (lang) {
        case UiLanguage::Polish: return "Brak aktywnych pluginow";
        case UiLanguage::Spanish: return "Sin plugins activos";
        case UiLanguage::French: return "Aucun plugin actif";
        case UiLanguage::German: return "Keine aktiven Plugins";
        case UiLanguage::Romanian: return "Niciun plugin activ";
        default: return "No active plugins";
      }
    case TrKey3::PluginEnabledTag:
      switch (lang) {
        case UiLanguage::Polish: return " [wlaczony]";
        case UiLanguage::Spanish: return " [activado]";
        case UiLanguage::French: return " [active]";
        case UiLanguage::German: return " [aktiviert]";
        case UiLanguage::Romanian: return " [activat]";
        default: return " [enabled]";
      }
    case TrKey3::PluginDisabledTag:
      switch (lang) {
        case UiLanguage::Polish: return " [wylaczony]";
        case UiLanguage::Spanish: return " [desactivado]";
        case UiLanguage::French: return " [desactive]";
        case UiLanguage::German: return " [deaktiviert]";
        case UiLanguage::Romanian: return " [dezactivat]";
        default: return " [disabled]";
      }
    case TrKey3::DisablePlugin:
      switch (lang) {
        case UiLanguage::Polish: return "Wylacz";
        case UiLanguage::Spanish: return "Desactivar";
        case UiLanguage::French: return "Desactiver";
        case UiLanguage::German: return "Deaktivieren";
        case UiLanguage::Romanian: return "Dezactiveaza";
        default: return "Disable";
      }
    case TrKey3::EnablePlugin:
      switch (lang) {
        case UiLanguage::Polish: return "Wlacz";
        case UiLanguage::Spanish: return "Activar";
        case UiLanguage::French: return "Activer";
        case UiLanguage::German: return "Aktivieren";
        case UiLanguage::Romanian: return "Activeaza";
        default: return "Enable";
      }
    case TrKey3::SaveCurrentPreset:
      switch (lang) {
        case UiLanguage::Polish: return "+ Zapisz obecne";
        case UiLanguage::Spanish: return "+ Guardar actual";
        case UiLanguage::French: return "+ Enregistrer actuel";
        case UiLanguage::German: return "+ Aktuelles speichern";
        case UiLanguage::Romanian: return "+ Salveaza actualul";
        default: return "+ Save Current";
      }
    case TrKey3::PresetLimitReachedParen:
      switch (lang) {
        case UiLanguage::Polish: return "(Limit 10 osiagniety)";
        case UiLanguage::Spanish: return "(Limite de 10 alcanzado)";
        case UiLanguage::French: return "(Limite de 10 atteinte)";
        case UiLanguage::German: return "(Limit von 10 erreicht)";
        case UiLanguage::Romanian: return "(Limita de 10 atinsa)";
        default: return "(Limit 10 reached)";
      }
    case TrKey3::PresetNameLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Nazwa presetu";
        case UiLanguage::Spanish: return "Nombre del preajuste";
        case UiLanguage::French: return "Nom du preregl.";
        case UiLanguage::German: return "Preset-Name";
        case UiLanguage::Romanian: return "Nume preset";
        default: return "Preset Name";
      }
    case TrKey3::InvalidName:
      switch (lang) {
        case UiLanguage::Polish: return "Nieprawidlowa nazwa";
        case UiLanguage::Spanish: return "Nombre invalido";
        case UiLanguage::French: return "Nom invalide";
        case UiLanguage::German: return "Ungueltiger Name";
        case UiLanguage::Romanian: return "Nume invalid";
        default: return "Invalid name";
      }
    case TrKey3::SavedLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Zapisano";
        case UiLanguage::Spanish: return "Guardado";
        case UiLanguage::French: return "Enregistre";
        case UiLanguage::German: return "Gespeichert";
        case UiLanguage::Romanian: return "Salvat";
        default: return "Saved";
      }
    case TrKey3::LimitReachedShort:
      switch (lang) {
        case UiLanguage::Polish: return "Limit osiagniety";
        case UiLanguage::Spanish: return "Limite alcanzado";
        case UiLanguage::French: return "Limite atteinte";
        case UiLanguage::German: return "Limit erreicht";
        case UiLanguage::Romanian: return "Limita atinsa";
        default: return "Limit reached";
      }
    case TrKey3::SdCardErrorLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Blad karty SD";
        case UiLanguage::Spanish: return "Error de tarjeta SD";
        case UiLanguage::French: return "Erreur carte SD";
        case UiLanguage::German: return "SD-Kartenfehler";
        case UiLanguage::Romanian: return "Eroare card SD";
        default: return "SD card error";
      }
    case TrKey3::LoadedLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Wczytano";
        case UiLanguage::Spanish: return "Cargado";
        case UiLanguage::French: return "Charge";
        case UiLanguage::German: return "Geladen";
        case UiLanguage::Romanian: return "Incarcat";
        default: return "Loaded";
      }
    case TrKey3::PresetLoadError:
      switch (lang) {
        case UiLanguage::Polish: return "Blad wczytywania presetu";
        case UiLanguage::Spanish: return "Error al cargar el preajuste";
        case UiLanguage::French: return "Erreur de chargement du preregl.";
        case UiLanguage::German: return "Fehler beim Laden des Presets";
        case UiLanguage::Romanian: return "Eroare la incarcarea presetului";
        default: return "Error loading preset";
      }
    case TrKey3::ApplyColon:
      switch (lang) {
        case UiLanguage::Polish: return "Zastosuj: ";
        case UiLanguage::Spanish: return "Aplicar: ";
        case UiLanguage::French: return "Appliquer : ";
        case UiLanguage::German: return "Anwenden: ";
        case UiLanguage::Romanian: return "Aplica: ";
        default: return "Apply: ";
      }
    case TrKey3::DeletePresetColon:
      switch (lang) {
        case UiLanguage::Polish: return "Usun: ";
        case UiLanguage::Spanish: return "Eliminar: ";
        case UiLanguage::French: return "Supprimer : ";
        case UiLanguage::German: return "Loeschen: ";
        case UiLanguage::Romanian: return "Sterge: ";
        default: return "Delete: ";
      }
    case TrKey3::PresetDeleteFailed:
      switch (lang) {
        case UiLanguage::Polish: return "Blad usuwania";
        case UiLanguage::Spanish: return "Error al eliminar";
        case UiLanguage::French: return "Echec de la suppression";
        case UiLanguage::German: return "Loeschen fehlgeschlagen";
        case UiLanguage::Romanian: return "Stergere esuata";
        default: return "Delete failed";
      }
    case TrKey3::BackWifiHeader:
      switch (lang) {
        case UiLanguage::Polish: return "< Wroc | Wi-Fi";
        case UiLanguage::Spanish: return "< Volver | Wi-Fi";
        case UiLanguage::French: return "< Retour | Wi-Fi";
        case UiLanguage::German: return "< Zurueck | Wi-Fi";
        case UiLanguage::Romanian: return "< Inapoi | Wi-Fi";
        default: return "< Back | Wi-Fi";
      }
    case TrKey3::BackSyncHeader:
      switch (lang) {
        case UiLanguage::Polish: return "< Wroc | Sync";
        case UiLanguage::Spanish: return "< Volver | Sync";
        case UiLanguage::French: return "< Retour | Sync";
        case UiLanguage::German: return "< Zurueck | Sync";
        case UiLanguage::Romanian: return "< Inapoi | Sync";
        default: return "< Back | Sync";
      }
    case TrKey3::UsbBackHint:
      switch (lang) {
        case UiLanguage::Polish: return "USB | Tap = wroc";
        case UiLanguage::Spanish: return "USB | Toca = volver";
        case UiLanguage::French: return "USB | Touchez = retour";
        case UiLanguage::German: return "USB | Tippen = zurueck";
        case UiLanguage::Romanian: return "USB | Atinge = inapoi";
        default: return "USB | Tap = back";
      }
    case TrKey3::ConnectUsbCable:
      switch (lang) {
        case UiLanguage::Polish: return "Podlacz kabel USB";
        case UiLanguage::Spanish: return "Conecta el cable USB";
        case UiLanguage::French: return "Branchez le cable USB";
        case UiLanguage::German: return "USB-Kabel anschliessen";
        case UiLanguage::Romanian: return "Conecteaza cablul USB";
        default: return "Connect USB cable";
      }
    case TrKey3::SdVisibleOnPhone:
      switch (lang) {
        case UiLanguage::Polish: return "SD widoczna na telefonie/PC";
        case UiLanguage::Spanish: return "SD visible en telefono/PC";
        case UiLanguage::French: return "SD visible sur telephone/PC";
        case UiLanguage::German: return "SD sichtbar auf Handy/PC";
        case UiLanguage::Romanian: return "SD vizibil pe telefon/PC";
        default: return "SD visible on phone/PC";
      }
    case TrKey3::VolumeDown:
      switch (lang) {
        case UiLanguage::Polish: return "Gl -";
        case UiLanguage::Spanish: return "Vol -";
        case UiLanguage::French: return "Vol -";
        case UiLanguage::German: return "Lst -";
        case UiLanguage::Romanian: return "Vol -";
        default: return "Vol -";
      }
    case TrKey3::VolumeUp:
      switch (lang) {
        case UiLanguage::Polish: return "Gl +";
        case UiLanguage::Spanish: return "Vol +";
        case UiLanguage::French: return "Vol +";
        case UiLanguage::German: return "Lst +";
        case UiLanguage::Romanian: return "Vol +";
        default: return "Vol +";
      }
    case TrKey3::PositionLabel:
      switch (lang) {
        case UiLanguage::Polish: return "Pozycja";
        case UiLanguage::Spanish: return "Posicion";
        case UiLanguage::French: return "Position";
        case UiLanguage::German: return "Position";
        case UiLanguage::Romanian: return "Pozitie";
        default: return "Position";
      }
    case TrKey3::VolumeAbbrev:
      switch (lang) {
        case UiLanguage::Polish: return "Gl";
        case UiLanguage::Spanish: return "Vol";
        case UiLanguage::French: return "Vol";
        case UiLanguage::German: return "Lst";
        case UiLanguage::Romanian: return "Vol";
        default: return "Vol";
      }
  }
  return "";
}

}  // namespace Translations3
