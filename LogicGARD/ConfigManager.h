#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <vector>

#include "Types.h"
#include "BaseComponent.h"
#include "ConfigBase.h"
#include "ConfigNode.h"

class ConfigManager : public ConfigBase {
public:
  explicit ConfigManager();

  void begin(String filename = "/user.json");
  void writeConfig() override;
  void reset() override;
  bool updateFromJsonString(String source) override;
  String renderHtml(String htmlFilename = "/user.html") override;
  void syncValuesFrom(const JsonObject& patch) override;
  bool isConfigured() override;
  void setConfigured(bool value) override;

  DeviceIdentity getDeviceIdentity();
  NetworkConfig getNetworkConfig();
  std::vector<CameraConfig> getCameraConfigList();
  ApiConfig getCameraApiConfig(const ConfigNode& node);
  void updateSensorList(const std::vector<String>& sensorList);
  void restoreSettings(const String& filename = "/user_bkup.json");

  template<typename T>
  T get(const String& path) {
    JsonVariant target = getNested(doc.as<JsonObject>(), path.c_str());
    if (target.isNull()) {
      Serial.printf("⚠️ Path not found: %s\n", path.c_str());
      return T();
    }
    return target.as<T>();
  }

private:
  StaticJsonDocument<7168> doc;
  String _filename;

  JsonVariant getNested(JsonObject root, const char* path) const;
  std::vector<OverlayConfig> getOverlayConfigList(const ConfigNode& node);

  String get(const String& path) {
    JsonVariant target = getNested(doc.as<JsonObject>(), path.c_str());
    if (target.isNull()) {
      Serial.printf("⚠️ Path not found: %s\n", path.c_str());
      return "";
    }
    return target.as<String>();
  }
};