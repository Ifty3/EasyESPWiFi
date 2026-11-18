/*
  EasyESPWiFi - BasicPortal.ino
  Demonstrates zero-code Wi-Fi provisioning with a fallback Captive Portal
  for both ESP32 and ESP8266 boards.

  How it works:
  1. Checks EEPROM for previously saved Wi-Fi SSID and Password.
  2. If found, connects in Station mode within 10 seconds.
  3. If not found or connection fails, starts SoftAP "My_ESP_AP"
     and launches a captive portal (at 192.168.4.1) with a modern Dark Mode UI.
  4. User selects a network from the dropdown, enters the password, and clicks "Save & Connect".
  5. The ESP saves credentials to EEPROM and restarts to connect automatically.

  Resetting credentials:
  Hold down the onboard FLASH/BOOT button (GPIO 0 on most ESP32 & ESP8266 boards)
  on boot, or call wifi.resetCredentials() in code.
*/

#include <EasyESPWiFi.h>

EasyESPWiFi wifi;

// BOOT / FLASH button pin on ESP32 and NodeMCU boards
const int TRIGGER_PIN = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println(F("\n--- EasyESPWiFi Basic Portal Demo ---"));

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  // Check if reset button is held down at startup
  if (digitalRead(TRIGGER_PIN) == LOW) {
    Serial.println(F("BOOT button held! Resetting saved Wi-Fi credentials..."));
    wifi.resetCredentials();
    delay(1000);
  }

  // Single-line Wi-Fi initialization:
  // Automatically connects to saved Wi-Fi or launches Captive Portal "My_ESP_AP"
  if (wifi.begin("My_ESP_AP", 10)) {
    Serial.println(F("\n=================================="));
    Serial.println(F("   Connected to Wi-Fi Network!    "));
    Serial.println(F("=================================="));
    Serial.print(F("SSID:       "));
    Serial.println(wifi.getSSID());
    Serial.print(F("IP Address: "));
    Serial.println(wifi.getIP());
    Serial.print(F("Signal:     "));
    Serial.print(wifi.getRSSI());
    Serial.println(F(" dBm"));
    Serial.println(F("==================================\n"));
  }
}

void loop() {
  // Check Wi-Fi connection health periodically
  if (wifi.isConnected()) {
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 5000) {
      lastPrint = millis();
      Serial.print(F("[Status] Wi-Fi Active | IP: "));
      Serial.print(wifi.getIP());
      Serial.print(F(" | RSSI: "));
      Serial.print(wifi.getRSSI());
      Serial.println(F(" dBm"));
    }
  } else {
    Serial.println(F("[Warning] Wi-Fi disconnected!"));
    delay(2000);
  }

  // Your main sketch logic runs here...
  delay(100);
}
