/**
 * @file EasyESPWiFi.h
 * @brief Lightweight, cross-platform Wi-Fi manager & Captive Portal library for ESP32 and ESP8266.
 * @license MIT
 */

#ifndef EASY_ESP_WIFI_H
#define EASY_ESP_WIFI_H

#include <Arduino.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  #include <DNSServer.h>
  #include <EEPROM.h>
  using WebServerType = WebServer;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <DNSServer.h>
  #include <EEPROM.h>
  using WebServerType = ESP8266WebServer;
#else
  #error "EasyESPWiFi only supports ESP32 and ESP8266 architectures!"
#endif

// Avoid collision with ESP-IDF's internal MAX_SSID_LEN macro
#ifdef MAX_SSID_LEN
  #undef MAX_SSID_LEN
#endif

/// EEPROM Constants
constexpr uint16_t EEPROM_CONFIG_SIZE  = 128;
constexpr uint8_t  EEPROM_MAGIC_BYTE   = 0xA5;
constexpr uint16_t EEPROM_MAGIC_ADDR   = 0;
constexpr uint16_t EEPROM_SSID_ADDR    = 1;   // up to 32 chars + 1 null
constexpr uint16_t EEPROM_PASS_ADDR    = 34;  // up to 64 chars + 1 null

constexpr uint8_t  EASY_MAX_SSID_LEN   = 32;
constexpr uint8_t  EASY_MAX_PASS_LEN   = 64;

/// Captive Portal DNS Port
constexpr uint8_t DNS_PORT = 53;

class EasyESPWiFi {
public:
  EasyESPWiFi();
  ~EasyESPWiFi();

  /**
   * @brief Initializes Wi-Fi connection from saved EEPROM credentials.
   * If credentials are missing or connection fails, falls back to Captive Portal.
   * @param apName The SoftAP SSID to broadcast if portal starts (default: "My_ESP_AP").
   * @param timeoutSeconds Connection attempt timeout in seconds (default: 10s).
   * @return true if connected to Wi-Fi station, false if portal was launched.
   */
  bool begin(const char* apName = "My_ESP_AP", unsigned long timeoutSeconds = 10);

  /**
   * @brief Clears the magic byte in EEPROM so device boots into Captive Portal on next restart.
   */
  void resetCredentials();

  /**
   * @brief Checks if device is currently connected to a Wi-Fi station.
   */
  bool isConnected() const;

  /**
   * @brief Returns the active or saved SSID.
   */
  String getSSID() const;

  /**
   * @brief Returns local IP address as String.
   */
  String getIP() const;

  /**
   * @brief Returns current Wi-Fi RSSI (Signal Strength in dBm).
   */
  int getRSSI() const;

private:
  WebServerType* _server;
  DNSServer*     _dnsServer;
  String         _savedSSID;
  String         _savedPass;
  String         _apName;
  bool           _portalRunning;

  bool   _readCredentials();
  void   _writeCredentials(const String& ssid, const String& password);
  void   _startCaptivePortal();
  void   _setupWebServer();
  void   _handleRoot();
  void   _handleSave();
  void   _handleReset();
  void   _handleNotFound();
  String _buildPortalHTML();
};

#endif // EASY_ESP_WIFI_H
