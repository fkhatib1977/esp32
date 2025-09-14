#include "ConfigManager.h"
#include <SPIFFS.h>
#include <vector>

ConfigManager::ConfigManager() {}

void ConfigManager::begin(String filename) {
  _filename = filename;
  BaseComponent::debugLog("[ConfigManager] Opening config file: " + _filename);

  File file = SPIFFS.open(_filename, FILE_READ);
  if (!file || file.isDirectory()) {
    Serial.println("[ConfigManager] ❌ Failed to open config file");
    return;
  }

  DeserializationError error = deserializeJson(doc, file);
  file.close();

  if (error) {
    Serial.print("[ConfigManager] ❌ Failed to parse config file: ");
    Serial.println(error.c_str());
  } else {
    BaseComponent::debugLog("[ConfigManager] ✅ Config loaded");
  }
}

void ConfigManager::writeConfig() {
  File file = SPIFFS.open(_filename, FILE_WRITE);
  if (!file) {
    Serial.println("[ConfigManager] ❌ Failed to open file for writing");
    return;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("[ConfigManager] ❌ Failed to write to file");
  } else {
    BaseComponent::debugLog("[ConfigManager] ✅ Settings saved to " + _filename);
  }

  file.close();
}

void ConfigManager::reset() {
  BaseComponent::debugLog("[ConfigManager] Resetting config file: " + _filename);
  File configFile = SPIFFS.open(_filename, FILE_WRITE);
  if (!configFile) return;
  configFile.print("{}");
  configFile.close();
}

String ConfigManager::renderHtml(String htmlFilename) {
  BaseComponent::debugLog("[ConfigManager] Rendering HTML from: " + htmlFilename);
  File file = SPIFFS.open(htmlFilename, FILE_READ);
  if (!file || file.isDirectory()) {
    return "<h1>" + htmlFilename + " not found</h1>";
  }

  String html = file.readString();
  file.close();
  return html;
}

void ConfigManager::syncValuesFrom(const JsonObject& patch) {
  for (JsonPair kv : patch) {
    const char* key = kv.key().c_str();
    JsonVariant value = kv.value();

    JsonVariant target = getNested(doc.as<JsonObject>(), key);
    if (!target.isNull() && target.containsKey("value")) {
      target["value"] = value;
      BaseComponent::debugLog("[ConfigManager] Updated key: " + String(key));
    } else {
      Serial.printf("[ConfigManager] ⚠️ Key not found or missing 'value': %s\n", key);
    }
  }

  writeConfig();
}

JsonVariant ConfigManager::getNested(JsonObject root, const char* path) const {
  char buffer[128];
  strncpy(buffer, path, sizeof(buffer));
  buffer[sizeof(buffer) - 1] = '\0';

  JsonVariant current = root;
  char* p = buffer;

  while (*p) {
    while (*p == '.' || *p == '_') ++p;
    char* start = p;
    while (*p && *p != '.' && *p != '_') ++p;

    if (start == p) break;

    char saved = *p;
    *p = '\0';

    if (!current.is<JsonObject>()) return JsonVariant();

    JsonObject obj = current.as<JsonObject>();
    if (!obj.containsKey(start)) return JsonVariant();

    current = obj[start];
    *p = saved;
  }

  return current;
}

DeviceIdentity ConfigManager::getDeviceIdentity() {
  return {
    get("identification.clientId.value"),
    get("identification.locationId.value"),
    get("identification.unitId.value"),
    get("identification.version.value"),
    get("identification.board.value")
  };
}

ApiConfig ConfigManager::getCameraApiConfig(const ConfigNode& node) {
  return ApiConfig{
    node.get<String>("scheme.value"),
    node.get<String>("ip.value"),
    node.get<int>("port.value"),
    node.get<String>("path.value")
  };
}

std::vector<OverlayConfig> ConfigManager::getOverlayConfigList(const ConfigNode& node) {
  std::vector<OverlayConfig> overlays;
  auto overlayNodes = node.getArray("overlays");

  for (const auto& overlayNode : overlayNodes) {
    overlays.push_back(OverlayConfig{
      overlayNode.get<String>("sensor.value"),
      overlayNode.get<int>("identity.value"),
      overlayNode.get<int>("camera.value"),
      overlayNode.get<String>("indicator.value"),
      overlayNode.get<String>("text.value"),
      overlayNode.get<String>("position.value"),
      overlayNode.get<int>("fontSize.value"),
      overlayNode.get<String>("textColor.value")
    });
  }

  return overlays;
}

std::vector<CameraConfig> ConfigManager::getCameraConfigList() {
  std::vector<CameraConfig> configs;
  ConfigNode rootNode(doc.as<JsonVariantConst>());
  auto cameraNodes = rootNode.getArray("cameras");

  for (const auto& node : cameraNodes) {
    configs.push_back(CameraConfig{
      node.get<bool>("enabled.value"),
      getCameraApiConfig(node),
      {
        node.get<String>("username.value"),
        node.get<String>("password.value")
      },
      getOverlayConfigList(node)
    });
  }

  return configs;
}

void ConfigManager::updateSensorList(const std::vector<String>& sensorList) {
  if (doc.containsKey("sensors") &&
      doc["sensors"].containsKey("value") &&
      doc["sensors"]["value"].is<JsonArray>()) {

    JsonArray sensorsArray = doc["sensors"]["value"].as<JsonArray>();
    sensorsArray.clear();

    for (const String& sensor : sensorList) {
      BaseComponent::debugLog("[ConfigManager] Adding sensor: " + sensor);
      sensorsArray.add(sensor);
    }

    writeConfig();
    Serial.println("[ConfigManager] ✅ Sensor list updated.");
  } else {
    Serial.println("[ConfigManager] ⚠️ 'sensors.value' not found or not an array.");
  }
}

void ConfigManager::restoreSettings(const String& filename) {
  File file = SPIFFS.open(filename, "r");
  if (!file || file.isDirectory()) {
    Serial.printf("[ConfigManager] ⚠️ Backup file '%s' not found or invalid\n", filename.c_str());
    return;
  }

  String json = file.readString();
  file.close();

  if (!updateFromJsonString(json)) {
    Serial.printf("[ConfigManager] ❌ Failed to restore config from '%s'\n", filename.c_str());
  } else {
    Serial.printf("[ConfigManager] ✅ Config restored from '%s'\n", filename.c_str());
  }
}

bool ConfigManager::isConfigured() {
  JsonVariant target = getNested(doc.as<JsonObject>(), "isConfigured");
  return target["value"].as<bool>();
}

void ConfigManager::setConfigured(bool value) {
  JsonVariant target = getNested(doc.as<JsonObject>(), "isConfigured");
  if (!target.isNull() && target.containsKey("value")) {
    target["value"] = value;
    writeConfig();
  } else {
    Serial.println("[ConfigManager] ⚠️ Failed to set isConfigured: path not found");
  }
}

IPAddress IPAddressFromString(const String& str) {
  IPAddress ip;
  if (ip.fromString(str)) return ip;
  return IPAddress(0, 0, 0, 0);
}

NetworkConfig ConfigManager::getNetworkConfig() {
  ConfigNode rootNode(doc.as<JsonVariantConst>());
  ConfigNode netNode = rootNode.getChild("network");

  String typeStr = netNode.get<String>("connectionType.value");
  ConnectionType type = ConnectionType::UNKNOWN;
  if (typeStr == "LAN") type = ConnectionType::LAN;
  else if (typeStr == "WIFI") type = ConnectionType::WIFI;

  NetworkConfig config;
  config.connectionType = type;

  if (type == ConnectionType::LAN) {
    ConfigNode lanNode = netNode.getChild("lan");
    config.isStatic = lanNode.get<bool>("isStatic.value");

    ConfigNode ipNode = lanNode.getChild("ipConfig");
    config.ip      = IPAddressFromString(ipNode.get<String>("ip.value"));
    config.subnet  = IPAddressFromString(ipNode.get<String>("subnet.value"));
    config.gateway = IPAddressFromString(ipNode.get<String>("gateway.value"));
    config.dns1    = IPAddressFromString(ipNode.get<String>("dns1.value"));
    config.dns2    = IPAddressFromString(ipNode.get<String>("dns2.value"));
  }

  if (type == ConnectionType::WIFI) {
    ConfigNode wifiNode = netNode.getChild("wifi");
    config.ssid     = wifiNode.get<String>("ssid.value");
    config.password = wifiNode.get<String>("password.value");
  }

  return config;
}