#include "AdminConfigManager.h"
#include <SPIFFS.h>

AdminConfigManager::AdminConfigManager() {}

void AdminConfigManager::begin() {
  File file = SPIFFS.open("/admin.json", "r");
  if (!file || file.isDirectory()) return;
  if (file.size() == 0) {
    file.close();
    return;
  }

  DeserializationError error = deserializeJson(doc, file);
  file.close();
  if (error) return;
}

bool AdminConfigManager::updateFromJsonString(String source) {
  StaticJsonDocument<7168> json;
  DeserializationError error = deserializeJson(json, source);
  if (error) return false;

  File file = SPIFFS.open("/admin.json", "w");
  if (!file) return false;

  serializeJson(json, file);
  file.close();
  begin();
  return true;
}

void AdminConfigManager::writeConfig() {
  File file = SPIFFS.open("/admin.json", "w");
  if (!file) return;
  serializeJson(doc, file);
  file.close();
}

void AdminConfigManager::reset() {
  File file = SPIFFS.open("/admin.json", "w");
  if (!file) return;
  file.print("{}");
  file.close();
}

String AdminConfigManager::renderHtml(String htmlFilename) {
  File file = SPIFFS.open(htmlFilename, "r");
  if (!file || file.isDirectory()) return "<html><body><h1>File not found</h1></body></html>";

  String html;
  while (file.available()) html += file.readStringUntil('\n') + "\n";
  file.close();
  return html;
}

void AdminConfigManager::syncValuesFrom(const JsonObject& patch) {
  JsonObject root = doc.to<JsonObject>();
  for (JsonPair kv : patch) {
    const char* key = kv.key().c_str();
    JsonVariant value = kv.value();
    root[key] = value;
  }
  writeConfig();
}

std::vector<SensorConfig> AdminConfigManager::getSensors() const {
  ConfigNode root(doc);
  std::vector<ConfigNode> nodes = root.getArray("sensors");

  std::vector<SensorConfig> result;
  result.reserve(nodes.size()); // 🔒 Prevent reallocation during push_back

  for (const auto& node : nodes) {
    if (!node.has("name") || !node.has("interface")) {
      Serial.println("[AdminConfig] ⚠️ Sensor missing 'name' or 'interface'. Skipping.");
      continue;
    }

    SensorConfig sensor;
    sensor.name      = node.get<String>("name");
    sensor.interface = node.get<String>("interface");

    bool duplicate = false;
    for (const auto& existing : result) {
      if (existing.name == sensor.name) {
        Serial.printf("[AdminConfig] ⚠️ Duplicate sensor name '%s'. Skipping.\n", sensor.name.c_str());
        duplicate = true;
        break;
      }
    }
    if (duplicate) continue;

    sensor.readIntervalMs = node.get<uint32_t>("readIntervalMs");
    sensor.enabled        = node.get<bool>("enabled");

    if (sensor.interface == "i2c") {
      sensor.sdaPin = node.get<uint8_t>("sdaPin");
      sensor.sclPin = node.get<uint8_t>("sclPin");
    } else if (sensor.interface == "spi") {
      sensor.mosiPin = node.get<uint8_t>("mosiPin");
      sensor.misoPin = node.get<uint8_t>("misoPin");
      sensor.sckPin  = node.get<uint8_t>("sckPin");
      sensor.csPin   = node.get<uint8_t>("csPin");
    } else if (sensor.interface == "analog") {
      sensor.analogPin = node.get<uint8_t>("analogPin");
    } else if (sensor.interface == "onewire") {
      sensor.onewirePin = node.get<uint8_t>("onewirePin");
    } else {
      Serial.printf("[AdminConfig] ❌ Unknown interface '%s' for sensor '%s'. Skipping.\n",
                    sensor.interface.c_str(), sensor.name.c_str());
      continue;
    }

    result.push_back(sensor);
  }

  return result;
}

AuthCredentials AdminConfigManager::getAdminAuth() const {
  ConfigNode root(doc);
  return {
    root.get<String>("auth.username"),
    root.get<String>("auth.password")
  };
}

AuthCredentials AdminConfigManager::getAccessPointCred() const {
  ConfigNode root(doc);
  return {
    root.get<String>("accessPoint.name"),
    root.get<String>("accessPoint.password")
  };
}

NtpConfig AdminConfigManager::getNtpConfig() const {
  ConfigNode root(doc);
  NtpConfig config;
  config.enabled    = root.get<bool>("ntp.enabled");
  config.url        = root.get<String>("ntp.url");
  config.username   = root.get<String>("ntp.username");
  config.password   = root.get<String>("ntp.password");
  config.gmtOffset  = root.get<int>("ntp.gmtOffset");
  config.dstOffset  = root.get<int>("ntp.dstOffset");
  return config;
}

RtcConfig AdminConfigManager::getRtcConfig() const {
  ConfigNode root(doc);
  RtcConfig config;
  config.sdaPin = root.get<uint8_t>("rtc.sdaPin");
  config.sclPin = root.get<uint8_t>("rtc.sclPin");

  if (root.has("rtc.timeAdjust")) {
    ConfigNode adj = root.getNode("rtc.timeAdjust");
    config.timeAdjust.enabled = adj.get<bool>("enabled");
    config.timeAdjust.year    = adj.get<int>("year");
    config.timeAdjust.month   = adj.get<int>("month");
    config.timeAdjust.day     = adj.get<int>("day");
    config.timeAdjust.hour    = adj.get<int>("hour");
    config.timeAdjust.minute  = adj.get<int>("minute");
    config.timeAdjust.second  = adj.get<int>("second");
  }

  return config;
}

TimeProviderType AdminConfigManager::getTimeProviderType() const {
  ConfigNode root(doc);
  String providerStr = root.get<String>("timeProvider");
  providerStr.toLowerCase();
  if (providerStr == "rtc") return TimeProviderType::RTC;
  if (providerStr == "ntp") return TimeProviderType::NTP;
  return TimeProviderType::UNKNOWN;
}

bool AdminConfigManager::debugEnabled() const {
  ConfigNode root(doc);
  return root.get<bool>("debugEnabled");
}

MqttConfig AdminConfigManager::getMqttConfig() const {
  ConfigNode root(doc);
  return {
    root.get<bool>("mqtt.enabled"),
    root.get<String>("mqtt.sensorId"),
    root.get<String>("mqtt.broker"),
    root.get<uint16_t>("mqtt.port"),
    root.get<String>("mqtt.clientId"),
    root.get<String>("mqtt.topic"),
    root.get<String>("mqtt.username"),
    root.get<String>("mqtt.password"),
    root.get<uint16_t>("mqtt.batchSize"),
    root.get<uint32_t>("mqtt.flushIntervalMs"),
    root.get<uint32_t>("mqtt.bufferSize")
  };
}

OtaConfig AdminConfigManager::getOtaConfig() const {
  ConfigNode root(doc);
  return {
    root.get<String>("ota.checkInUrl"),
    root.get<String>("ota.host"),
    root.get<uint32_t>("ota.checkIntervalMs"),
    root.get<bool>("ota.enabled"),
    root.get<bool>("ota.allowDowngrade"),
    root.get<bool>("ota.autoApply"),
    root.get<String>("ota.currentVersion"),
    {
      root.get<String>("ota.credentials.username"),
      root.get<String>("ota.credentials.password")
    }
  };
}

TftDisplayConfig AdminConfigManager::getTftDisplayConfig() const {
  ConfigNode root(doc);
  return {
    root.get<uint8_t>("tftDisplay.cs"),
    root.get<uint8_t>("tftDisplay.dc"),
    root.get<uint8_t>("tftDisplay.rst"),
    root.get<uint8_t>("tftDisplay.orientation"),
    root.get<uint8_t>("tftDisplay.totalLines")
  };
}