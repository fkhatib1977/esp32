#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "BaseComponent.h"

class ConfigBase : public BaseComponent {
public:
  virtual ~ConfigBase() = default;

  // Load config from JSON string
  bool AdminConfigManager::updateFromJsonString(String source) {
    StaticJsonDocument<7168> json;
    DeserializationError error = deserializeJson(json, source);
    if (error) return false;

    File file = SPIFFS.open(_filename, "w");
    if (!file) return false;

    serializeJson(json, file);
    file.close();
    return true;
  }

  // Save current config to file
  virtual void writeConfig() = 0;

  // Reset config to empty/default
  virtual void reset() = 0;

  // Render HTML view of config
  virtual String renderHtml(String htmlFilename = "/config.html") = 0;

  // Apply a patch to the config
  virtual void syncValuesFrom(const JsonObject& patch) = 0;

  // Optional: expose whether config is marked as "configured"
  virtual bool isConfigured() = 0;
  virtual void setConfigured(bool value) = 0;

protected:
  String _filename;
};