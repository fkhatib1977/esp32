#pragma once

#include <WebServer.h>
#include "ConfigManager.h"
#include "AdminConfigManager.h"
#include <ArduinoJson.h>
#include <FS.h>

class WebServerManager {
public:
  explicit WebServerManager(uint16_t port = 80);
  void loop();

  void begin(ConfigManager& config, AdminConfigManager& adminConfig);

private:
  WebServer server;
  StaticJsonDocument<1024> doc;
  ConfigManager* configRef = nullptr;
  AdminConfigManager* adminConfigRef = nullptr;

  void serveConfigPage();
  void serveAdminPage();
  void resetConfig(String displayMessage = "Config Reset. Restarting...");
  void saveConfigHandler();
  void saveAdminConfigHandler();
};