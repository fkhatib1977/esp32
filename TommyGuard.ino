#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

const char* ssid = "TommyGuard";
const char* password = "12345678";
WebServer server(80);

#define BOOT_BUTTON 0  // GPIO0
bool buttonPressed = false;
bool pendingRestart = false;
unsigned long pressStart = 0;

// ─── Global JSON Document ──────────────────────
StaticJsonDocument<512> doc;

File openConfigFile(const char* mode) {
  return SPIFFS.open("/appSettings.json", mode);
}

void writeConfig() {
  SPIFFS.remove("/appSettings.json");  // 💥 Ensure clean slate

  File file = openConfigFile(FILE_WRITE);
  if (!file) {
    Serial.println("❌ Failed to open file for writing");
    return;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("❌ Failed to write to file");
  } else {
    Serial.println("📝 Settings saved to /appSettings.json.");
  }

  file.close();
}

void restoreSettings() {
  File backupFile = SPIFFS.open("/appSettings_bkup.json", FILE_READ);
  if (!backupFile) {
    Serial.println("🚨 Backup file not found! Restore aborted.");
    return;
  }

  DeserializationError error = deserializeJson(doc, backupFile);
  backupFile.close();

  if (error) {
    Serial.println("❌ Failed to parse backup config.");
    return;
  }

  writeConfig();
  Serial.println("🔄 Settings restored from backup.");
}

void loadConfig() {
  File configFile = openConfigFile(FILE_READ);
  if (!configFile) {
    Serial.println("⚠️ Config file not found, using defaults.");
    return;
  }

  DeserializationError error = deserializeJson(doc, configFile);
  configFile.close();

  if (error) {
    Serial.println("❌ Failed to parse config.");
    return;
  }

  Serial.println("✅ Config file loaded.");
}

String renderConfigHtml() {
  File file = SPIFFS.open("/index.html", FILE_READ);
  if (!file) return "<h1>index.html not found</h1>";

  String html = file.readString();
  file.close();

  html.replace("{{clientId}}", doc["identification"]["clientId"] | "");
  html.replace("{{locationId}}", doc["identification"]["locationId"] | "");
  html.replace("{{unitId}}", doc["identification"]["unitId"] | "");

  html.replace("{{isStatic}}", (doc["lan"]["isStatic"] | true) ? "selected" : "");
  html.replace("{{isDhcp}}", (doc["lan"]["isStatic"] | true) ? "" : "selected");
  html.replace("{{ip}}", doc["lan"]["ip"] | "");
  html.replace("{{subnet}}", doc["lan"]["subnet"] | "");
  html.replace("{{gateway}}", doc["lan"]["gateway"] | "");
  html.replace("{{dns1}}", doc["lan"]["dns1"] | "");
  html.replace("{{dns2}}", doc["lan"]["dns2"] | "");

  html.replace("{{cameraIp}}", doc["camera"]["ip"] | "");
  html.replace("{{cameraUsername}}", doc["camera"]["username"] | "");
  html.replace("{{cameraPassword}}", doc["camera"]["password"] | "");

  JsonObject camParams = doc["camera"]["params"][0];
  html.replace("{{camNumber}}", String(camParams["camera"] | 1));
  html.replace("{{topLeft}}", (camParams["position"] == "TopLeft") ? "selected" : "");
  html.replace("{{topRight}}", (camParams["position"] == "TopRight") ? "selected" : "");
  html.replace("{{center}}", (camParams["position"] == "Center") ? "selected" : "");
  html.replace("{{bottomLeft}}", (camParams["position"] == "BottomLeft") ? "selected" : "");
  html.replace("{{bottomRight}}", (camParams["position"] == "BottomRight") ? "selected" : "");
  html.replace("{{camPosition}}", camParams["position"] | "topLeft");
  html.replace("{{fontSize}}", String(camParams["fontSize"] | 14));
  html.replace("{{textColor}}", camParams["textColor"] | "red");
  html.replace("{{camText}}", camParams["text"] | "");

  html.replace("{{nvrIp}}", doc["nvr"]["ip"] | "");
  html.replace("{{nvrUsername}}", doc["nvr"]["username"] | "");
  html.replace("{{nvrPassword}}", doc["nvr"]["password"] | "");
  html.replace("{{nvrPort}}", String(doc["nvr"]["params"]["port"] | 1));

  return html;
}

void serveConfigPage() {
  String html = renderConfigHtml();
  server.send(200, "text/html", html);
}

void resetConfig() {
  File configFile = openConfigFile(FILE_WRITE);
  configFile.print("{}");
  configFile.close();

  server.send(200, "text/html", "<h1>Config Reset. Restarting...</h1>");
  scheduleRestart();
}

void saveConfigHandler() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not found");
    return;
  }

  String body = server.arg("plain");
  DeserializationError error = deserializeJson(doc, body);

  if (error) {
    server.send(400, "text/plain", "Invalid JSON");
    return;
  }

  writeConfig();
  server.send(200, "text/plain", "Settings saved successfully");
  scheduleRestart();
}

void scheduleRestart() {
  Serial.println("🔁 Restart requested.");
  pendingRestart = true;
}

void handleShortPress() {
  doc["isConfigured"] = false;
  writeConfig();
  scheduleRestart();
}

void handleLongPress() {
  Serial.println("🛠️ Long press detected!");
  Serial.println("🔄 Restoring settings and restarting...");
  restoreSettings();
  scheduleRestart();
}

void monitorBootButton() {
  if (digitalRead(BOOT_BUTTON) == LOW && !buttonPressed) {
    pressStart = millis();
    buttonPressed = true;
  }

  if (digitalRead(BOOT_BUTTON) == HIGH && buttonPressed) {
    unsigned long pressDuration = millis() - pressStart;
    buttonPressed = false;

    if (pressDuration >= 10000) {
      handleLongPress();
    } else {
      handleShortPress();
    }
  }
}

void startAccessPoint() {
  Serial.println("📶 Starting Access Point: " + String(ssid));
  WiFi.softAP(ssid, password);
  Serial.println("✅ Access Point started (SSID: TommyGuard)");
}

void launchWebServer() {
  server.on("/", serveConfigPage);
  server.on("/reset", HTTP_GET, resetConfig);
  server.on("/save", HTTP_POST, saveConfigHandler);
  server.begin();
  Serial.println("🌐 Configuration web server started.");
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_BUTTON, INPUT_PULLUP);

  if (!SPIFFS.begin(true)) {
    Serial.println("❌ SPIFFS Mount Failed");
    return;
  }

  loadConfig();

  if (!doc["isConfigured"]) {
    Serial.println("🧰 Device not configured — entering setup mode.");
    startAccessPoint();
    launchWebServer();
    return;
  }

  Serial.println("🔧 Device in operation mode.");
}

void loop() {
  server.handleClient();
  monitorBootButton();

  if (pendingRestart) {
    delay(500);
    ESP.restart();
  }
}