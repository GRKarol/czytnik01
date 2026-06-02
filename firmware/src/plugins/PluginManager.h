#pragma once

#include <Arduino.h>
#include <Preferences.h>

#ifndef PLUGIN_TIMER_ENABLED
#define PLUGIN_TIMER_ENABLED 0
#endif

#ifndef PLUGIN_RSS_ENABLED
#define PLUGIN_RSS_ENABLED 0
#endif

/**
 * PluginManager — zarządza stanem pluginów na urządzeniu.
 *
 * Stan pluginów = uint32_t bitmask przechowywany w NVS pod kluczem "pl_mask".
 * Bit 0 = timer (PLUGIN_TIMER_ENABLED), bit 1 = rss (PLUGIN_RSS_ENABLED).
 *
 * Klasa jest bezstanowa poza referencją do Preferences — nie trzyma
 * własnego stanu w RAM poza cache'em maski, który jest odświeżany przy
 * każdym begin().
 */
class PluginManager {
 public:
  // Definicja wszystkich pluginów (kolejność = numer bitu w bitmask)
  enum class PluginId : uint8_t {
    Timer = 0,
    Rss   = 1,
    Count = 2,
  };

  // Klucz NVS dla bitmask
  static constexpr const char* kNvsKey = "pl_mask";

  // Inicjalizuje — wczytuje bitmask z NVS
  void begin(Preferences& prefs) {
    prefs_ = &prefs;
    mask_ = prefs.getUInt(kNvsKey, defaultMask());
  }

  // Zwraca bitmask odpowiadającą temu co jest aktualnie skompilowane
  // (zgodnie z build flags). Używamy jej przy pierwszym uruchomieniu.
  // UWAGA: Timer jest zawsze wbudowany (FocusTimer zawsze kompilowany),
  // więc jego bit jest zawsze ustawiany niezależnie od PLUGIN_TIMER_ENABLED.
  static uint32_t defaultMask() {
    uint32_t m = 0;
    // Timer jest zawsze wbudowany — bit zawsze ustawiony
    m |= (1u << static_cast<uint8_t>(PluginId::Timer));
#if PLUGIN_RSS_ENABLED
    m |= (1u << static_cast<uint8_t>(PluginId::Rss));
#endif
    return m;
  }

  // Czy dany plugin jest aktualnie zainstalowany (wg NVS)?
  bool isInstalled(PluginId id) const {
    return (mask_ & (1u << static_cast<uint8_t>(id))) != 0;
  }

  // Czy dany plugin jest aktualnie AKTYWNY (wg build flag)?
  static bool isActive(PluginId id) {
    return (defaultMask() & (1u << static_cast<uint8_t>(id))) != 0;
  }

  // Oblicza jaka bitmask będzie potrzebna po zainstalowaniu pluginu
  uint32_t maskAfterInstall(PluginId id) const {
    return mask_ | (1u << static_cast<uint8_t>(id));
  }

  // Oblicza jaka bitmask będzie potrzebna po odinstalowaniu pluginu
  uint32_t maskAfterRemove(PluginId id) const {
    return mask_ & ~(1u << static_cast<uint8_t>(id));
  }

  // Aktualizuje maskę w NVS (po sukcesie OTA)
  void setMask(uint32_t newMask) {
    mask_ = newMask;
    if (prefs_ != nullptr) {
      prefs_->putUInt(kNvsKey, newMask);
    }
  }

  uint32_t mask() const { return mask_; }

  // Buduje nazwę pliku .bin dla danej bitmask
  // Np. mask=0b00 → "flower-firmware.bin"
  //        mask=0b01 → "flower-firmware-timer.bin"
  //        mask=0b11 → "flower-firmware-timer-rss.bin"
  static String variantFilename(uint32_t mask) {
    String name = "flower-firmware";
    if (mask & (1u << static_cast<uint8_t>(PluginId::Timer))) {
      name += "-timer";
    }
    if (mask & (1u << static_cast<uint8_t>(PluginId::Rss))) {
      name += "-rss";
    }
    name += ".bin";
    return name;
  }

  // Nazwa pluginu (PL lub EN) wg aktualnego języka
  static const char* pluginName(PluginId id, bool polish) {
    switch (id) {
      case PluginId::Timer:
        return polish ? "Klepsydra" : "Focus Timer";
      case PluginId::Rss:
        return polish ? "Kanaly RSS" : "RSS feeds";
      default:
        return "";
    }
  }

  // Opis pluginu
  static const char* pluginDesc(PluginId id, bool polish) {
    switch (id) {
      case PluginId::Timer:
        return polish ? "Timer sesji czytania" : "Reading session timer";
      case PluginId::Rss:
        return polish ? "Pobieranie artykulow" : "Article downloads";
      default:
        return "";
    }
  }

 private:
  Preferences* prefs_ = nullptr;
  uint32_t mask_ = 0;
};
