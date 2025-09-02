#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <vector>

#include "ConfigBase.h"
#include "Types.h"
#include "ConfigNode.h"  // Enables schema-driven access

class AdminConfigManager : public ConfigBase {
public:
  AdminConfigManager();

  void begin();
  bool updateFromJsonString(String source) override;
  void writeConfig() override;
  void reset() override;
  String renderHtml(String htmlFilename = "/admin.html") override;
  void syncValuesFrom(const JsonObject& patch) override;
  bool isConfigured() override { return true; }
  void setConfigured(bool) override {}

  std::vector<SensorConfig> getSensors() const;
  AuthCredentials getAdminAuth() const;
  AuthCredentials getAccessPointCred() const;
  NtpConfig getNtpConfig() const;
  RtcConfig getRtcConfig() const;
  TimeProviderType getTimeProviderType() const;
  bool debugEnabled() const;
  MqttConfig getMqttConfig() const;
  OtaConfig getOtaConfig() const;
  TftDisplayConfig getTftDisplayConfig() const;

private:
  StaticJsonDocument<7168> doc;
};