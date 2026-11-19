# EasyESPWiFi

[![Arduino Library Manager](https://img.shields.io/badge/Arduino_Library_Manager-EasyESPWiFi-00979C.svg?logo=arduino&logoColor=white)](https://github.com/Ifty3/EasyESPWiFi)
[![Release](https://img.shields.io/github/v/release/Ifty3/EasyESPWiFi?color=brightgreen)](https://github.com/Ifty3/EasyESPWiFi/releases)
[![Hardware: ESP32 | ESP8266](https://img.shields.io/badge/Hardware-ESP32%20%7C%20ESP8266-E7352C.svg?logo=espressif&logoColor=white)](https://www.espressif.com/)
[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)

Lightweight, zero-dependency, cross-platform Wi-Fi manager and Captive Portal library designed specifically for **ESP32** and **ESP8266** microcontrollers.

Eliminate hardcoded Wi-Fi credentials in your Arduino sketches. With a single line of code in `setup()`, `EasyESPWiFi` automatically handles stored credential retrieval, station connection, or fallback captive portal provisioning with a responsive Dark Mode Web UI.

---

## Installation

### Method 1: Via Arduino Library Manager (Recommended)
1. Open the **Arduino IDE** (version 1.8.x or 2.x).
2. Go to **Sketch** &rarr; **Include Library** &rarr; **Manage Libraries...** (or press `Ctrl + Shift + I` / `Cmd + Shift + I`).
3. Search for **EasyESPWiFi** in the search field.
4. Select the latest version and click **Install**.

### Method 2: Via Arduino CLI
```bash
arduino-cli lib install "EasyESPWiFi"
```

### Method 3: Manual Installation (ZIP)
1. Download the latest release `.zip` archive from [GitHub Releases](https://github.com/Ifty3/EasyESPWiFi/releases).
2. In Arduino IDE, navigate to **Sketch** &rarr; **Include Library** &rarr; **Add .ZIP Library...**
3. Select the downloaded `.zip` file.

---

## Key Features

- **Single-Line Wi-Fi Setup**: Call `wifi.begin("My_ESP_AP");` in `setup()` to manage connection attempts and provisioning fallbacks.
- **Cross-Platform Compatibility**: Supports all ESP32 (WROOM, S2, S3, C3) and ESP8266 (NodeMCU, ESP-01, Wemos D1 Mini) boards using standard core headers.
- **EEPROM Storage with Magic Byte Validation**: Stores credentials locally using flash sector emulation with magic byte (`0xA5`) checking to prevent uninitialized memory reads.
- **Modern Responsive Dark Mode UI**: Clean, mobile-first captive portal card with live network scanning and RSSI signal indicators.
- **Captive Portal Auto-Popup**: Built-in DNS server on port 53 redirects captive portal probes for Android, iOS/macOS (Apple CNA), and Windows.
- **Zero External Dependencies**: Operates exclusively with native core platform libraries (`WiFi`, `WebServer`/`ESP8266WebServer`, `DNSServer`, `EEPROM`).

---

## Directory Layout (Arduino Library Manager Compliant)

```text
EasyESPWiFi/
├── src/
│   ├── EasyESPWiFi.h
│   └── EasyESPWiFi.cpp
├── examples/
│   └── BasicPortal/
│       └── BasicPortal.ino
├── tests/
│   └── verify_library.py
├── keywords.txt
├── library.properties
└── README.md
```

---

## Quick Start Example

```cpp
#include <EasyESPWiFi.h>

EasyESPWiFi wifi;

void setup() {
  Serial.begin(115200);

  // Single-line setup: Connects to saved Wi-Fi or opens captive portal "My_ESP_AP"
  if (wifi.begin("My_ESP_AP", 10)) {
    Serial.println("Connected to Wi-Fi!");
    Serial.print("IP Address: ");
    Serial.println(wifi.getIP());
  }
}

void loop() {
  if (wifi.isConnected()) {
    // Normal sketch routine
  }
}
```

---

## API Reference

### `bool begin(const char* apName = "My_ESP_AP", unsigned long timeoutSeconds = 10)`
Reads EEPROM for saved credentials. If found, attempts to connect to the Wi-Fi network within `timeoutSeconds` (default: 10s). If credentials do not exist or connection fails, starts SoftAP with `apName` and launches the Captive Portal DNS and Web Server.

### `void resetCredentials()`
Clears the magic validation byte in EEPROM. Next reboot triggers the Captive Portal.

### `bool isConnected() const`
Returns `true` if the device is currently connected to an access point in station mode.

### `String getSSID() const`
Returns the active network SSID.

### `String getIP() const`
Returns the assigned station IP address.

### `int getRSSI() const`
Returns the current received signal strength indicator (dBm).

---

## EEPROM Memory Layout

The library reserves 128 bytes of EEPROM flash sector emulation:

| Offset | Size (Bytes) | Description |
|:-------|:-------------|:------------|
| `0` | 1 | Magic Validation Byte (`0xA5`) |
| `1` | 33 | Wi-Fi SSID (Null-terminated, max 32 chars) |
| `34` | 65 | Wi-Fi Password (Null-terminated, max 64 chars) |

---

## License

MIT License. Open-source and free for commercial and non-commercial projects.
