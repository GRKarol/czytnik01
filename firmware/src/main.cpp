#include <Arduino.h>
#include <esp_log.h>
#include <driver/gpio.h>

#include "app/App.h"
#include "board/BoardConfig.h"

App app;

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
  BoardConfig::begin();
  // Skip long serial wait — no need to block boot for 2s.
  delay(20);
  Serial.println("[main] app setup");
  app.begin();
}

void loop() {
  const uint32_t now = millis();
  app.update(now);
}
