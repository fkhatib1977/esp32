#include <SPIFFS.h>
#include "WebServerManager.h"

WebServerManager::WebServerManager(uint16_t port) : server(port) {}

void WebServerManager::begin(ConfigManager& config, AdminConfigManager& adminConfig) {
  configRef = &config;
  adminConfigRef = &adminConfig;

  server.on("/", [this]() { serveConfigPage(); });
  server.on("/admin", [this]() { serveAdminPage(); });
  server.on("/reset", HTTP_GET, [this]() { resetConfig(); });
  server.on("/save-user", HTTP_POST, [this]() { saveConfigHandler(); });
  server.on("/save-admin", HTTP_POST, [this]() { saveAdminConfigHandler(); });
  server.serveStatic("/", SPIFFS, "/");

  server.begin();
  Serial.println("🌐 Configuration web server started.");
}

void WebServerManager::serveConfigPage() {  
  std::vector<SensorConfig> sensors = adminConfigRef->getSensors();
  std::vector<String> sensorNames;

  for (const SensorConfig& sensor : sensors) {
    sensorNames.push_back(sensor.name);  // Assuming SensorConfig has a .name field
  }

  configRef->updateSensorList(sensorNames);

  Serial.println("serving config page ...");
  String html = configRef->renderHtml();
  Serial.println("sending rendered html ...");
  server.send(200, "text/html", html);
  Serial.println("sent!");
}

void WebServerManager::serveAdminPage() {
  AuthCredentials auth = adminConfigRef->getAdminAuth();
  if (!server.authenticate(auth.username.c_str(), auth.password.c_str())) {
    return server.requestAuthentication();
  }

  String html = adminConfigRef->renderHtml();
  server.send(200, "text/html", html);
}


void WebServerManager::resetConfig(String displayMessage) {
  configRef->reset();
  server.send(200, "text/html", String("<h1>") + displayMessage + "</h1>");
  ESP.restart();
}

void WebServerManager::saveConfigHandler() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not found");
    return;
  }

  String body = server.arg("plain");

  DynamicJsonDocument incoming(8192);
  DeserializationError err = deserializeJson(incoming, body);
  if (err) {
    server.send(400, "text/plain", "Invalid JSON format");
    return;
  }

  JsonObject obj = incoming.as<JsonObject>();

  // Delegate patching to ConfigManager
  configRef->syncValuesFrom(obj);
  server.send(200, "text/plain", "Settings saved successfully. Restarting ...");
  ESP.restart();
}

void WebServerManager::saveAdminConfigHandler() {
  if (!server.hasArg("plain")) {
    server.send(400, "text/plain", "Body not found");
    return;
  }

  String body = server.arg("plain");
  adminConfigRef->updateFromJsonString(body.c_str());

  server.send(200, "text/plain", "Settings saved successfully. Restarting ...");
  ESP.restart();
}

void WebServerManager::loop() {
    server.handleClient();
}
