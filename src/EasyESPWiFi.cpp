/**
 * @file EasyESPWiFi.cpp
 * @brief Implementation of EasyESPWiFi library.
 * @license MIT
 */

#include "EasyESPWiFi.h"

EasyESPWiFi::EasyESPWiFi()
  : _server(nullptr),
    _dnsServer(nullptr),
    _savedSSID(""),
    _savedPass(""),
    _apName("My_ESP_AP"),
    _portalRunning(false) {
}

EasyESPWiFi::~EasyESPWiFi() {
  if (_server) {
    _server->stop();
    delete _server;
    _server = nullptr;
  }
  if (_dnsServer) {
    _dnsServer->stop();
    delete _dnsServer;
    _dnsServer = nullptr;
  }
}

bool EasyESPWiFi::_readCredentials() {
  EEPROM.begin(EEPROM_CONFIG_SIZE);
  uint8_t magic = EEPROM.read(EEPROM_MAGIC_ADDR);
  if (magic != EEPROM_MAGIC_BYTE) {
    EEPROM.end();
    return false;
  }

  char ssidBuf[EASY_MAX_SSID_LEN + 1];
  char passBuf[EASY_MAX_PASS_LEN + 1];
  memset(ssidBuf, 0, sizeof(ssidBuf));
  memset(passBuf, 0, sizeof(passBuf));

  for (uint16_t i = 0; i < EASY_MAX_SSID_LEN; i++) {
    char c = (char)EEPROM.read(EEPROM_SSID_ADDR + i);
    ssidBuf[i] = c;
    if (c == '\0') break;
  }
  ssidBuf[EASY_MAX_SSID_LEN] = '\0';

  for (uint16_t i = 0; i < EASY_MAX_PASS_LEN; i++) {
    char c = (char)EEPROM.read(EEPROM_PASS_ADDR + i);
    passBuf[i] = c;
    if (c == '\0') break;
  }
  passBuf[EASY_MAX_PASS_LEN] = '\0';

  EEPROM.end();

  _savedSSID = String(ssidBuf);
  _savedPass = String(passBuf);

  return (_savedSSID.length() > 0);
}

void EasyESPWiFi::_writeCredentials(const String& ssid, const String& password) {
  EEPROM.begin(EEPROM_CONFIG_SIZE);
  EEPROM.write(EEPROM_MAGIC_ADDR, EEPROM_MAGIC_BYTE);

  for (uint16_t i = 0; i < EASY_MAX_SSID_LEN; i++) {
    if (i < ssid.length()) {
      EEPROM.write(EEPROM_SSID_ADDR + i, ssid[i]);
    } else {
      EEPROM.write(EEPROM_SSID_ADDR + i, 0);
    }
  }

  for (uint16_t i = 0; i < EASY_MAX_PASS_LEN; i++) {
    if (i < password.length()) {
      EEPROM.write(EEPROM_PASS_ADDR + i, password[i]);
    } else {
      EEPROM.write(EEPROM_PASS_ADDR + i, 0);
    }
  }

  EEPROM.commit();
  EEPROM.end();

  _savedSSID = ssid;
  _savedPass = password;
}

void EasyESPWiFi::resetCredentials() {
  EEPROM.begin(EEPROM_CONFIG_SIZE);
  EEPROM.write(EEPROM_MAGIC_ADDR, 0x00);
  for (uint16_t i = 1; i < (EEPROM_SSID_ADDR + EASY_MAX_SSID_LEN + EASY_MAX_PASS_LEN + 2); i++) {
    EEPROM.write(i, 0x00);
  }
  EEPROM.commit();
  EEPROM.end();

  _savedSSID = "";
  _savedPass = "";
  Serial.println(F("[EasyESPWiFi] Credentials cleared from EEPROM."));
}

bool EasyESPWiFi::begin(const char* apName, unsigned long timeoutSeconds) {
  if (apName && strlen(apName) > 0) {
    _apName = String(apName);
  }

  Serial.println(F("\n=================================="));
  Serial.println(F("     EasyESPWiFi Initialization   "));
  Serial.println(F("=================================="));

  bool hasCreds = _readCredentials();

  if (hasCreds) {
    Serial.print(F("[EasyESPWiFi] Found saved SSID: "));
    Serial.println(_savedSSID);
    Serial.print(F("[EasyESPWiFi] Connecting in STA mode (timeout: "));
    Serial.print(timeoutSeconds);
    Serial.println(F("s)..."));

    WiFi.mode(WIFI_STA);
    WiFi.begin(_savedSSID.c_str(), _savedPass.c_str());

    unsigned long startMs = millis();
    unsigned long timeoutMs = timeoutSeconds * 1000UL;

    while (WiFi.status() != WL_CONNECTED && (millis() - startMs) < timeoutMs) {
      delay(300);
      Serial.print(F("."));
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println(F("[EasyESPWiFi] SUCCESS: Connected to Wi-Fi!"));
      Serial.print(F("[EasyESPWiFi] IP Address: "));
      Serial.println(WiFi.localIP());
      Serial.print(F("[EasyESPWiFi] Signal (RSSI): "));
      Serial.print(WiFi.RSSI());
      Serial.println(F(" dBm"));
      return true;
    }

    Serial.println(F("[EasyESPWiFi] Connection timed out or failed. Falling back to Captive Portal."));
  } else {
    Serial.println(F("[EasyESPWiFi] No saved credentials found. Launching Captive Portal."));
  }

  _startCaptivePortal();
  return false;
}

void EasyESPWiFi::_startCaptivePortal() {
  _portalRunning = true;

  WiFi.disconnect();
  delay(100);

  // SoftAP + Station mode allows scanning nearby APs while hosting portal
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(_apName.c_str());

  IPAddress apIP = WiFi.softAPIP();
  Serial.print(F("[EasyESPWiFi] SoftAP started: "));
  Serial.println(_apName);
  Serial.print(F("[EasyESPWiFi] Portal IP: http://"));
  Serial.println(apIP);

  if (!_dnsServer) {
    _dnsServer = new DNSServer();
  }
  // Redirect all DNS queries to SoftAP IP for captive detection
  _dnsServer->start(DNS_PORT, "*", apIP);

  _setupWebServer();

  Serial.println(F("[EasyESPWiFi] Captive portal running. Waiting for configuration..."));

  while (_portalRunning) {
    _dnsServer->processNextRequest();
    _server->handleClient();
    delay(10);
    yield();
  }
}

void EasyESPWiFi::_setupWebServer() {
  if (!_server) {
    _server = new WebServerType(80);
  }

  // Primary portal routes
  _server->on("/", HTTP_GET, [this]() { _handleRoot(); });
  _server->on("/save", HTTP_POST, [this]() { _handleSave(); });
  _server->on("/reset", HTTP_GET, [this]() { _handleReset(); });

  // Captive Portal Probe Redirections for various OS/Vendors
  // Android
  _server->on("/generate_204", [this]() { _handleRoot(); });
  _server->on("/gen_204", [this]() { _handleRoot(); });
  // Apple iOS / macOS Captive Network Assistant (CNA)
  _server->on("/hotspot-detect.html", [this]() { _handleRoot(); });
  _server->on("/canonical.html", [this]() { _handleRoot(); });
  // Windows
  _server->on("/connecttest.txt", [this]() { _handleRoot(); });
  _server->on("/ncsi.txt", [this]() { _handleRoot(); });

  // Wildcard fallback
  _server->onNotFound([this]() { _handleNotFound(); });

  _server->begin();
  Serial.println(F("[EasyESPWiFi] HTTP Server started on port 80"));
}

void EasyESPWiFi::_handleNotFound() {
  String host = _server->hostHeader();
  IPAddress apIP = WiFi.softAPIP();
  String apIPStr = apIP.toString();

  // If request is directed to another domain (Captive redirect), route to AP IP
  if (!host.startsWith(apIPStr)) {
    _server->sendHeader("Location", String("http://") + apIPStr + "/", true);
    _server->send(302, "text/plain", "");
    return;
  }

  _handleRoot();
}

String EasyESPWiFi::_buildPortalHTML() {
  // Scan nearby Wi-Fi networks
  int n = WiFi.scanNetworks();

  String html = F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
                  "<title>EasyESPWiFi Setup</title><style>"
                  ":root{--bg:#0f172a;--card:#1e293b;--border:#334155;--text:#f8fafc;--muted:#94a3b8;--accent:#38bdf8;--btn-from:#0284c7;--btn-to:#6366f1;}"
                  "*{box-sizing:border-box;margin:0;padding:0;font-family:system-ui,-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,sans-serif;}"
                  "body{background:var(--bg);color:var(--text);display:flex;align-items:center;justify-content:center;min-height:100vh;padding:16px;}"
                  ".card{background:var(--card);border:1px solid var(--border);border-radius:18px;max-width:420px;width:100%;padding:28px;box-shadow:0 25px 50px -12px rgba(0,0,0,0.5);}"
                  ".header{text-align:center;margin-bottom:24px;}"
                  ".icon{width:52px;height:52px;background:rgba(56,189,248,0.1);color:var(--accent);border-radius:14px;display:inline-flex;align-items:center;justify-content:center;margin-bottom:12px;}"
                  "h1{font-size:22px;font-weight:700;color:#fff;letter-spacing:-0.5px;}"
                  "p.sub{font-size:13px;color:var(--muted);margin-top:4px;}"
                  ".form-group{margin-bottom:18px;}"
                  "label{display:block;font-size:13px;font-weight:600;margin-bottom:8px;color:#cbd5e1;}"
                  "select,input{width:100%;padding:12px 14px;background:#090d16;border:1px solid var(--border);border-radius:10px;color:#fff;font-size:14px;outline:none;transition:border-color 0.2s,box-shadow 0.2s;}"
                  "select:focus,input:focus{border-color:var(--accent);box-shadow:0 0 0 3px rgba(56,189,248,0.2);}"
                  ".btn{width:100%;padding:13px;background:linear-gradient(135deg,var(--btn-from),var(--btn-to));border:none;border-radius:10px;color:#fff;font-size:15px;font-weight:600;cursor:pointer;transition:transform 0.15s,opacity 0.2s;box-shadow:0 10px 15px -3px rgba(2,132,199,0.3);margin-top:8px;}"
                  ".btn:active{transform:scale(0.98);}"
                  ".btn-secondary{background:#334155;color:#cbd5e1;box-shadow:none;margin-top:10px;font-size:13px;padding:10px;}"
                  ".hint{font-size:11px;color:var(--muted);margin-top:5px;}"
                  ".footer{text-align:center;margin-top:20px;font-size:11px;color:#64748b;}"
                  "</style></head><body><div class='card'>"
                  "<div class='header'>"
                  "<div class='icon'>"
                  "<svg width='28' height='28' viewBox='0 0 24 24' fill='none' stroke='currentColor' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'><path d='M5 12.55a11 11 0 0 1 14.08 0'/><path d='M1.42 9a16 16 0 0 1 21.16 0'/><path d='M8.53 16.11a6 6 0 0 1 6.95 0'/><line x1='12' y1='20' x2='12.01' y2='20'/></svg>"
                  "</div>"
                  "<h1>EasyESPWiFi</h1>"
                  "<p class='sub'>Select your Wi-Fi network and connect</p>"
                  "</div>"
                  "<form method='POST' action='/save'>");

  html += F("<div class='form-group'><label for='scanList'>Nearby Networks (Live Scan)</label>"
            "<select id='scanList' onchange='document.getElementById(\"ssid\").value=this.value;'>"
            "<option value=''>-- Choose an Available Network --</option>");

  if (n <= 0) {
    html += F("<option value=''>No networks found (Refresh to rescan)</option>");
  } else {
    for (int i = 0; i < n; ++i) {
      String netSSID = WiFi.SSID(i);
      int32_t rssi   = WiFi.RSSI(i);
      int quality    = (rssi <= -100) ? 0 : (rssi >= -50) ? 100 : (2 * (rssi + 100));

      html += "<option value='" + netSSID + "'>" + netSSID + " (" + String(rssi) + " dBm, " + String(quality) + "%)</option>";
    }
  }

  html += F("</select><p class='hint'>Selecting an item autofills the Network Name below.</p></div>");

  html += F("<div class='form-group'><label for='ssid'>Network Name (SSID)</label>"
            "<input type='text' id='ssid' name='ssid' required maxlength='32' placeholder='Enter or select SSID' value='");
  html += _savedSSID;
  html += F("'></div>"
            "<div class='form-group'><label for='pass'>Password</label>"
            "<input type='password' id='pass' name='password' maxlength='64' placeholder='Enter Wi-Fi Password'>"
            "<p class='hint'>Leave blank if the network is open / unencrypted.</p></div>"
            "<button type='submit' class='btn'>Save &amp; Connect</button>"
            "</form>"
            "<form method='GET' action='/reset' onsubmit='return confirm(\"Are you sure you want to reset credentials?\");'>"
            "<button type='submit' class='btn btn-secondary'>Clear Saved Credentials</button>"
            "</form>"
            "<div class='footer'>EasyESPWiFi &bull; Zero-Config Wi-Fi Manager</div>"
            "</div></body></html>");

  return html;
}

void EasyESPWiFi::_handleRoot() {
  _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  _server->sendHeader("Pragma", "no-cache");
  _server->sendHeader("Expires", "-1");
  _server->send(200, "text/html", _buildPortalHTML());
}

void EasyESPWiFi::_handleSave() {
  String ssid = _server->arg("ssid");
  String pass = _server->arg("password");

  ssid.trim();
  pass.trim();

  if (ssid.length() == 0) {
    _server->send(400, "text/html", F("<html><body style='background:#0f172a;color:#fff;font-family:sans-serif;padding:30px;'>"
                                      "<h2>Error: SSID cannot be empty!</h2>"
                                      "<p><a href='/' style='color:#38bdf8;'>Go back</a></p></body></html>"));
    return;
  }

  Serial.println(F("[EasyESPWiFi] Saving new credentials:"));
  Serial.print(F("  SSID: "));
  Serial.println(ssid);

  _writeCredentials(ssid, pass);

  String resp = F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
                  "<title>Saving Credentials</title><style>"
                  "body{background:#0f172a;color:#f8fafc;font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px;text-align:center;}"
                  ".card{background:#1e293b;border:1px solid #334155;border-radius:18px;max-width:380px;width:100%;padding:32px;box-shadow:0 25px 50px -12px rgba(0,0,0,0.5);}"
                  ".spinner{width:40px;height:40px;border:3px solid #334155;border-top-color:#38bdf8;border-radius:50%;animation:spin 1s linear infinite;margin:0 auto 16px;}"
                  "@keyframes spin{to{transform:rotate(360deg);}}"
                  "h1{font-size:20px;color:#38bdf8;margin-bottom:8px;}"
                  "p{font-size:14px;color:#94a3b8;line-height:1.5;}"
                  "</style></head><body><div class='card'>"
                  "<div class='spinner'></div>"
                  "<h1>Credentials Saved!</h1>"
                  "<p>Connecting to <strong>");
  resp += ssid;
  resp += F("</strong>...</p>"
            "<p style='margin-top:12px;'>The ESP device is restarting now. Please reconnect your device to your local network.</p>"
            "</div></body></html>");

  _server->send(200, "text/html", resp);
  delay(1200);

  Serial.println(F("[EasyESPWiFi] Rebooting device to apply Wi-Fi connection..."));
  ESP.restart();
}

void EasyESPWiFi::_handleReset() {
  resetCredentials();

  String resp = F("<!DOCTYPE html><html lang='en'><head><meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width,initial-scale=1.0'>"
                  "<title>Credentials Cleared</title><style>"
                  "body{background:#0f172a;color:#f8fafc;font-family:system-ui,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh;padding:20px;text-align:center;}"
                  ".card{background:#1e293b;border:1px solid #334155;border-radius:18px;max-width:380px;width:100%;padding:32px;}"
                  "h1{font-size:20px;color:#f87171;margin-bottom:8px;}"
                  "p{font-size:14px;color:#94a3b8;margin-bottom:16px;}"
                  "a{color:#38bdf8;text-decoration:none;font-weight:600;}"
                  "</style></head><body><div class='card'>"
                  "<h1>Credentials Cleared!</h1>"
                  "<p>All saved Wi-Fi credentials have been removed from EEPROM.</p>"
                  "<a href='/'>&larr; Return to Portal</a>"
                  "</div></body></html>");

  _server->send(200, "text/html", resp);
}

bool EasyESPWiFi::isConnected() const {
  return (WiFi.status() == WL_CONNECTED);
}

String EasyESPWiFi::getSSID() const {
  if (isConnected()) {
    return WiFi.SSID();
  }
  return _savedSSID;
}

String EasyESPWiFi::getIP() const {
  if (isConnected()) {
    return WiFi.localIP().toString();
  }
  return String("0.0.0.0");
}

int EasyESPWiFi::getRSSI() const {
  if (isConnected()) {
    return WiFi.RSSI();
  }
  return 0;
}
