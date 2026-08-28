#include <Arduino.h>
#include <esp_log.h>
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/gpio.h>

#include "app/App.h"
#include "board/BoardConfig.h"

App app;

namespace {

// Diagnostic only. On this board, power-on is a hardware cold boot done by
// the SYS_EN latch on the power-management chip (see BoardConfig::
// releaseBatteryPowerHold) before the ESP32 ever runs — by the time this
// line executes, the decision to boot has already been made in hardware.
// Serial logs confirm resetReason is POWERON or SW every time; wakeup cause
// is never EXT0 in practice, so there is no software hook available here to
// gate "hold long enough to power on." A prior attempt at that
// (requirePowerOnHoldOrResleep) never actually fired and was removed.
void logResetReason() {
  Serial.printf("[main] resetReason=%d wakeup cause=%d pwrPin=%d\n",
                static_cast<int>(esp_reset_reason()),
                static_cast<int>(esp_sleep_get_wakeup_cause()),
                digitalRead(BoardConfig::PIN_PWR_BUTTON));
  Serial.flush();
}

}  // namespace

// Called very early by ESP-IDF before app_main/setup.
// Forces backlight pin HIGH (off, active-low) at the hardware level
// to prevent pixel noise from being visible during boot.
extern "C" void app_main_early_init() __attribute__((constructor));
void app_main_early_init() {
  gpio_reset_pin(static_cast<gpio_num_t>(BoardConfig::PIN_LCD_BACKLIGHT));
  gpio_set_direction(static_cast<gpio_num_t>(BoardConfig::PIN_LCD_BACKLIGHT), GPIO_MODE_OUTPUT);
  gpio_set_level(static_cast<gpio_num_t>(BoardConfig::PIN_LCD_BACKLIGHT), 1);
}

void setup() {
  // Redundant backlight off — the constructor above should have done this
  // but ensure it stays off through Arduino init.
  pinMode(BoardConfig::PIN_LCD_BACKLIGHT, OUTPUT);
  digitalWrite(BoardConfig::PIN_LCD_BACKLIGHT, HIGH);

  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_INFO);
  const bool pwrButtonHeld = BoardConfig::begin();
  logResetReason();
  // Skip long serial wait — no need to block boot for 2s.
  delay(20);

  if (!pwrButtonHeld) {
    // Booted without PWR being pressed - the USB cable was plugged in just
    // to charge. Don't light up the reader; drop straight back into the
    // same deep-sleep "off" state a normal power-off uses, and wait for an
    // actual PWR press (BoardConfig::enablePwrButtonExt0Wakeup) before ever
    // running app.begin().
    Serial.println("[main] no PWR press at boot (charging); staying off");
    Serial.flush();
    BoardConfig::holdBacklightOffForDeepSleep();
    BoardConfig::releaseBatteryPowerHold();
    BoardConfig::enablePwrButtonExt0Wakeup();
    esp_deep_sleep_start();
  }

  Serial.println("[main] app setup");
  app.begin();
}

void loop() {
  const uint32_t now = millis();
  app.update(now);
  // Yield to FreeRTOS idle task — allows light sleep between iterations
  // when no work is pending. Saves ~30-40% CPU power in idle states.
  delay(1);
}
