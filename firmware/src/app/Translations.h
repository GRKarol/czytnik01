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
  }
  return "";
}

}  // namespace Translations2
