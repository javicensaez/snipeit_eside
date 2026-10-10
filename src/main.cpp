#include <Arduino.h>
#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

constexpr uint32_t REFRESH_MS = 60UL * 1000UL;
const char *CONSUMABLES[] = {"74LS04", "74LS08", "74LS32"};
constexpr size_t NUM_CONSUMABLES = sizeof(CONSUMABLES) / sizeof(CONSUMABLES[0]);
int stock[NUM_CONSUMABLES] = {-1, -1, -1};
int ids[NUM_CONSUMABLES] = {-1, -1, -1};
int totals[NUM_CONSUMABLES] = {0, 0, 0};
int pending[NUM_CONSUMABLES] = {0, 0, 0};
uint32_t lastPressAt = 0;
constexpr uint32_t DEBOUNCE_MS = 1500;
uint32_t lastRefreshAt = 0;
bool needsRedraw = true;

void drawMessage(const char *l1, const char *l2);

// Botón A del M5Stack Fire (activo en LOW, pull-up externo)
constexpr uint8_t BUTTON_A_PIN = 39;
constexpr uint32_t LONG_PRESS_MS = 2000;
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t PORTAL_TIMEOUT_MS = 5UL * 60UL * 1000UL;

const char *AP_SSID = "SnipeIT-Setup";
const byte DNS_PORT = 53;

Preferences prefs;
WebServer server(80);
DNSServer dnsServer;

bool portalActive = false;
uint32_t portalStartedAt = 0;
uint32_t pressStartedAt = 0;
bool longPressHandled = false;

String htmlEscape(const String &in) {
  String out;
  out.reserve(in.length());
  for (char c : in) {
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default: out += c;
    }
  }
  return out;
}

bool connectToWifi(const String &ssid, const String &pass, uint32_t timeoutMs) {
  WiFi.begin(ssid.c_str(), pass.c_str());
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < timeoutMs) {
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

void connectSavedWifi() {
  prefs.begin("wifi", true);
  String ssid = prefs.getString("ssid", "");
  String pass = prefs.getString("pass", "");
  prefs.end();

  if (ssid.isEmpty()) {
    Serial.println("No hay WiFi guardada. Mantén pulsado A 2s para configurar.");
    return;
  }
  Serial.printf("Conectando a '%s'...\n", ssid.c_str());
  WiFi.mode(WIFI_STA);
  if (connectToWifi(ssid, pass, WIFI_CONNECT_TIMEOUT_MS)) {
    Serial.printf("Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("No se pudo conectar.");
  }
}

void handleRoot() {
  prefs.begin("wifi", true);
  String savedSsid = prefs.getString("ssid", "");
  String savedPass = prefs.getString("pass", "");
  prefs.end();
  prefs.begin("snipeit", true);
  String savedUrl = prefs.getString("url", "");
  String savedToken = prefs.getString("token", "");
  prefs.end();

  String page =
      "<!DOCTYPE html><html><head><meta charset='utf-8'>"
      "<meta name='viewport' content='width=device-width,initial-scale=1'>"
      "<title>Configurar WiFi</title>"
      "<style>body{font-family:sans-serif;margin:1.5em;max-width:420px}"
      "input,select,button{width:100%;padding:.6em;margin:.4em 0;font-size:1em}</style>"
      "</head><body><h2>Configurar WiFi</h2>"
      "<form method='POST' action='/save'>"
      "<label>Red WiFi</label><select onchange=\"document.getElementById('ssid').value=this.value\">"
      "<option value=''>-- Selecciona --</option>";

  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    String s = htmlEscape(WiFi.SSID(i));
    page += "<option value=\"" + s + "\">" + s + " (" + String(WiFi.RSSI(i)) + " dBm)</option>";
  }
  WiFi.scanDelete();

  page +=
      "</select>"
      "<input id='ssid' name='ssid' placeholder='SSID' maxlength='32' value=\"" +
      htmlEscape(savedSsid) +
      "\" required>"
      "<input name='pass' type='password' maxlength='64' placeholder=\"" +
      String(savedPass.isEmpty() ? "Contraseña" : "Contraseña guardada (vacío = conservar)") +
      "\">"
      "<h2>Snipe-IT</h2>"
      "<input name='url' type='url' placeholder='URL (https://snipeit.ejemplo.com)' maxlength='200' value=\"" +
      htmlEscape(savedUrl) +
      "\" required>"
      "<input name='token' type='password' maxlength='1024' placeholder=\"" +
      String(savedToken.isEmpty() ? "Token API" : "Token guardado (vacío = conservar)") +
      "\">"
      "<button type='submit'>Guardar y conectar</button></form></body></html>";

  server.send(200, "text/html", page);
}

void handleSave() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  if (ssid.isEmpty()) {
    server.send(400, "text/plain", "SSID requerido");
    return;
  }

  String url = server.arg("url");
  String token = server.arg("token");
  url.trim();
  token.trim();
  while (url.endsWith("/")) url.remove(url.length() - 1);
  if (!url.startsWith("http://") && !url.startsWith("https://")) {
    server.send(400, "text/plain", "URL invalida: debe empezar por http:// o https://");
    return;
  }

  prefs.begin("wifi", false);
  // Contraseña vacía con el mismo SSID: se conserva la guardada
  if (!(pass.isEmpty() && prefs.getString("ssid", "") == ssid)) {
    prefs.putString("pass", pass);
  }
  prefs.putString("ssid", ssid);
  prefs.end();

  prefs.begin("snipeit", false);
  prefs.putString("url", url);
  // Si se deja vacío se conserva el token ya guardado
  if (!token.isEmpty()) prefs.putString("token", token);
  prefs.end();

  server.send(200, "text/html",
              "<!DOCTYPE html><html><head><meta charset='utf-8'>"
              "<meta name='viewport' content='width=device-width,initial-scale=1'></head>"
              "<body style='font-family:sans-serif;margin:1.5em'>"
              "<h2>Guardado</h2><p>El dispositivo se reiniciará y se conectará a la red.</p></body></html>");
  delay(1500);
  ESP.restart();
}

// Cualquier otra ruta redirige al portal
void handleNotFound() {
  server.sendHeader("Location", "http://" + WiFi.softAPIP().toString() + "/", true);
  server.send(302, "text/plain", "");
}

void startPortal() {
  Serial.println("Iniciando portal cautivo...");
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID);
  delay(100);

  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.onNotFound(handleNotFound);
  server.begin();

  portalActive = true;
  portalStartedAt = millis();
  drawMessage("Portal WiFi activo:", AP_SSID);
  Serial.printf("Portal activo. Conéctate a '%s' (IP %s)\n", AP_SSID,
                WiFi.softAPIP().toString().c_str());
}

void stopPortal() {
  server.stop();
  dnsServer.stop();
  portalActive = false;
  WiFi.softAPdisconnect(true);
  Serial.println("Portal cerrado por timeout.");
  connectSavedWifi();
  lastRefreshAt = 0;
  needsRedraw = true;
}

void checkButton() {
  if (digitalRead(BUTTON_A_PIN) != LOW) {
    pressStartedAt = 0;
    longPressHandled = false;
    return;
  }
  if (pressStartedAt == 0) pressStartedAt = millis();
  if (!longPressHandled && !portalActive && millis() - pressStartedAt >= LONG_PRESS_MS) {
    longPressHandled = true;
    startPortal();
  }
}

// Devuelve las unidades restantes del consumible, o -1 si falla
int fetchConsumable(const String &baseUrl, const String &token, const char *name,
                    int *outId = nullptr, int *outQty = nullptr) {
  String url = baseUrl + "/api/v1/consumables?limit=10&search=" + name;
  WiFiClientSecure secure;
  WiFiClient plain;
  HTTPClient http;
  http.setTimeout(10000);

  bool ok;
  if (url.startsWith("https://")) {
    secure.setInsecure();  // no se valida el certificado del servidor
    ok = http.begin(secure, url);
  } else {
    ok = http.begin(plain, url);
  }
  if (!ok) return -1;

  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("Accept", "application/json");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Snipe-IT %s -> HTTP %d\n", name, code);
    http.end();
    return -1;
  }

  String body = http.getString();
  http.end();
  Serial.printf("%s -> %u bytes: %.200s\n", name, body.length(), body.c_str());

  JsonDocument filter;
  filter["rows"][0]["id"] = true;
  filter["rows"][0]["name"] = true;
  filter["rows"][0]["qty"] = true;
  filter["rows"][0]["remaining"] = true;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body, DeserializationOption::Filter(filter));
  if (err) {
    Serial.printf("JSON error: %s\n", err.c_str());
    return -1;
  }

  JsonArray rows = doc["rows"];
  JsonObject match;
  for (JsonObject row : rows) {
    String n = row["name"] | "";
    if (n.equalsIgnoreCase(name)) { match = row; break; }
  }
  if (match.isNull() && rows.size() > 0) match = rows[0];
  if (match.isNull()) return -1;
  if (outId) *outId = match["id"] | -1;
  if (outQty) *outQty = match["qty"] | 0;
  return match["remaining"] | (match["qty"] | -1);
}

void drawStock() {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(WHITE, BLACK);
  M5.Display.setTextSize(3);
  for (size_t i = 0; i < NUM_CONSUMABLES; i++) {
    M5.Display.setCursor(10, 30 + i * 60);
    if (stock[i] < 0) {
      M5.Display.setTextColor(RED, BLACK);
      M5.Display.printf("%s: --", CONSUMABLES[i]);
    } else {
      int shown = stock[i] - pending[i];
      M5.Display.setTextColor(shown == 0 ? RED : (pending[i] ? YELLOW : GREEN), BLACK);
      M5.Display.printf("%s: %d   ", CONSUMABLES[i], shown);
    }
  }
  needsRedraw = false;
}

void drawMessage(const char *l1, const char *l2) {
  M5.Display.fillScreen(BLACK);
  M5.Display.setTextColor(YELLOW, BLACK);
  M5.Display.setTextSize(2);
  M5.Display.setCursor(10, 60);
  M5.Display.println(l1);
  M5.Display.setCursor(10, 100);
  M5.Display.println(l2);
}

void refreshStock() {
  lastRefreshAt = millis();
  if (WiFi.status() != WL_CONNECTED) return;
  prefs.begin("snipeit", true);
  String url = prefs.getString("url", "");
  String token = prefs.getString("token", "");
  prefs.end();
  if (url.isEmpty() || token.isEmpty()) return;

  for (size_t i = 0; i < NUM_CONSUMABLES; i++) {
    stock[i] = fetchConsumable(url, token, CONSUMABLES[i], &ids[i], &totals[i]);
    Serial.printf("%s: %d\n", CONSUMABLES[i], stock[i]);
  }
  needsRedraw = true;
}

// Resta 1 a la cantidad total del consumible en Snipe-IT
void decrementConsumable(size_t i, int amount) {
  if (WiFi.status() != WL_CONNECTED || ids[i] < 0 || stock[i] <= 0) {
    Serial.printf("%s: no se puede descontar\n", CONSUMABLES[i]);
    return;
  }
  if (amount > stock[i]) amount = stock[i];
  prefs.begin("snipeit", true);
  String url = prefs.getString("url", "");
  String token = prefs.getString("token", "");
  prefs.end();

  WiFiClientSecure secure;
  WiFiClient plain;
  HTTPClient http;
  http.setTimeout(10000);
  String endpoint = url + "/api/v1/consumables/" + String(ids[i]);
  bool ok;
  if (endpoint.startsWith("https://")) {
    secure.setInsecure();
    ok = http.begin(secure, endpoint);
  } else {
    ok = http.begin(plain, endpoint);
  }
  if (!ok) return;

  http.addHeader("Authorization", "Bearer " + token);
  http.addHeader("Accept", "application/json");
  http.addHeader("Content-Type", "application/json");
  int code = http.sendRequest("PATCH", "{\"qty\":" + String(totals[i] - amount) + "}");
  String resp = http.getString();
  http.end();
  Serial.printf("Descontar %s -> HTTP %d %.200s\n", CONSUMABLES[i], code, resp.c_str());

  if (code == HTTP_CODE_OK) {
    int newStock = fetchConsumable(url, token, CONSUMABLES[i], &ids[i], &totals[i]);
    if (newStock >= 0) stock[i] = newStock;
    needsRedraw = true;
  }
}

// Acumula una pulsación; no se puede pedir más de lo que queda
void queueDecrement(size_t i) {
  if (stock[i] < 0 || pending[i] >= stock[i]) return;
  pending[i]++;
  lastPressAt = millis();
  needsRedraw = true;
}

void checkButtons() {
  M5.update();
  if (M5.BtnA.wasHold() && !portalActive) {
    startPortal();
    return;
  }
  if (portalActive) return;
  if (M5.BtnA.wasClicked()) queueDecrement(0);
  if (M5.BtnB.wasClicked()) queueDecrement(1);
  if (M5.BtnC.wasClicked()) queueDecrement(2);

  // Envía los descuentos acumulados cuando dejan de llegar pulsaciones
  if (lastPressAt != 0 && millis() - lastPressAt >= DEBOUNCE_MS) {
    lastPressAt = 0;
    for (size_t i = 0; i < NUM_CONSUMABLES; i++) {
      if (pending[i] > 0) {
        int n = pending[i];
        pending[i] = 0;
        decrementConsumable(i, n);
      }
    }
    needsRedraw = true;
  }
}

void setup() {
  M5.begin();
  Serial.begin(115200);
  M5.BtnA.setHoldThresh(LONG_PRESS_MS);
  M5.Display.setBrightness(100);
  drawMessage("Conectando WiFi...", "");
  connectSavedWifi();
  drawStock();
  refreshStock();
}

void loop() {
  checkButtons();
  if (portalActive) {
    dnsServer.processNextRequest();
    server.handleClient();
    if (millis() - portalStartedAt > PORTAL_TIMEOUT_MS) stopPortal();
    return;
  }
  if (millis() - lastRefreshAt >= REFRESH_MS) refreshStock();
  if (needsRedraw) drawStock();
}
