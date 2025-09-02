#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <IPAddress.h>
#include <cstring>
#include <vector>

// ─────────────────────────────────────────────────────────────
// Auth & Credentials
// ─────────────────────────────────────────────────────────────

struct AuthCredentials {
  String username;
  String password;

  bool isValid() const {
    return !username.isEmpty() && !password.isEmpty();
  }
};

// ─────────────────────────────────────────────────────────────
// OTA & Manifest
// ─────────────────────────────────────────────────────────────

enum class UpdateType {
  Binary,
  User,
  Admin,
  All,
  DebugEnable,
  DebugDisable,
  Unknown
};

struct FirmwareManifest {
  String version;
  String url;
  String configUrl;
  String checksum;
  String description;
  bool mandatory;
  UpdateType updateType;

  bool parse(const String& json);
};

struct OtaConfig {
  String checkInUrl;
  String host;
  uint32_t checkIntervalMs;
  bool enabled;
  bool allowDowngrade;
  bool autoApply;
  String currentVersion;
  AuthCredentials credentials;
};

// ─────────────────────────────────────────────────────────────
// Device Identity
// ─────────────────────────────────────────────────────────────

struct DeviceIdentity {
  String clientId;
  String locationId;
  String unitId;
  String version;
  String board;

  bool isValid() const {
    return !clientId.isEmpty() && !unitId.isEmpty();
  }

  String toJson() const {
    StaticJsonDocument<256> doc;
    doc["clientId"]   = clientId;
    doc["locationId"] = locationId;
    doc["unitId"]     = unitId;
    doc["version"]    = version;
    doc["board"]      = board;

    String output;
    serializeJson(doc, output);
    return output;
  }
};

// ─────────────────────────────────────────────────────────────
// API & Camera
// ─────────────────────────────────────────────────────────────

struct ApiConfig {
  String scheme = "http";
  String host;
  uint16_t port = 80;
  String path = "/";

  String fullUrl() const {
    if (host.isEmpty() || path.isEmpty()) return String();

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%s://%s:%u%s",
             scheme.c_str(), host.c_str(), port, path.c_str());
    return String(buffer);
  }
};

struct OverlayConfig {
  String sensorId;
  int identity = 0;
  int camera;
  String indicator;
  String text;
  String position;
  int fontSize;
  String textColor;
};

struct CameraConfig {
  bool enabled = false;
  ApiConfig api;
  AuthCredentials credentials;
  std::vector<OverlayConfig> overlays;
};

// ─────────────────────────────────────────────────────────────
// Sensor & Time
// ─────────────────────────────────────────────────────────────

struct SensorConfig {
  String name;
  String interface;
  bool enabled = false;
  uint32_t readIntervalMs = 0;

  int sdaPin      = -1;
  int sclPin      = -1;
  int mosiPin     = -1;
  int misoPin     = -1;
  int sckPin      = -1;
  int csPin       = -1;
  int analogPin   = -1;
  int onewirePin  = -1;

  bool isValid() const {
    return !name.isEmpty() && readIntervalMs > 0;
  }

  bool validate(bool verbose = true) const {
    bool valid = true;

    if (name.isEmpty()) {
      if (verbose) Serial.println("❌ SensorConfig: missing 'name'");
      valid = false;
    }

    if (interface.isEmpty()) {
      if (verbose) Serial.printf("❌ Sensor '%s': missing 'interface'\n", name.c_str());
      valid = false;
    }

    if (readIntervalMs == 0) {
      if (verbose) Serial.printf("❌ Sensor '%s': readIntervalMs must be > 0\n", name.c_str());
      valid = false;
    }

    if (interface == "i2c") {
      if (sdaPin < 0 || sclPin < 0) {
        if (verbose) Serial.printf("❌ Sensor '%s': missing SDA/SCL pins for I2C\n", name.c_str());
        valid = false;
      }
    } else if (interface == "spi") {
      if (mosiPin < 0 || misoPin < 0 || sckPin < 0 || csPin < 0) {
        if (verbose) Serial.printf("❌ Sensor '%s': missing SPI pins\n", name.c_str());
        valid = false;
      }
    } else if (interface == "analog") {
      if (analogPin < 0) {
        if (verbose) Serial.printf("❌ Sensor '%s': missing analog pin\n", name.c_str());
        valid = false;
      }
    } else if (interface == "onewire") {
      if (onewirePin < 0) {
        if (verbose) Serial.printf("❌ Sensor '%s': missing OneWire pin\n", name.c_str());
        valid = false;
      }
    } else {
      if (verbose) Serial.printf("❌ Sensor '%s': unknown interface '%s'\n", name.c_str(), interface.c_str());
      valid = false;
    }

    return valid;
  }
};

struct TimeAdjust {
  bool enabled;
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
};

struct RtcConfig {
  int sdaPin;
  int sclPin;
  TimeAdjust timeAdjust;
};

enum class TimeProviderType {
  UNKNOWN,
  RTC,
  NTP
};

// ─────────────────────────────────────────────────────────────
// Networking, MQTT & NTP
// ─────────────────────────────────────────────────────────────

enum class ConnectionType {
  LAN,
  WIFI,
  UNKNOWN
};

struct NetworkConfig {
  ConnectionType connectionType = ConnectionType::UNKNOWN;

  bool isStatic = false;
  IPAddress ip;
  IPAddress subnet;
  IPAddress gateway;
  IPAddress dns1;
  IPAddress dns2;

  String ssid;
  String password;

  bool isValid() const {
    if (connectionType == ConnectionType::LAN) {
      return isStatic || (ip == IPAddress(0, 0, 0, 0) && gateway == IPAddress(0, 0, 0, 0));
    }
    if (connectionType == ConnectionType::WIFI) {
      return !ssid.isEmpty() && !password.isEmpty();
    }
    return false;
  }

  String connectionTypeName() const {
    switch (connectionType) {
      case ConnectionType::LAN: return "LAN";
      case ConnectionType::WIFI: return "WIFI";
      default: return "UNKNOWN";
    }
  }
};

struct MqttConfig {
  bool enabled;
  String sensorId;
  String broker;
  int port;
  String clientId;
  String topic;
  String username;
  String password;

  uint16_t batchSize;
  uint32_t flushIntervalMs;
  int bufferSize;
};

struct NtpConfig {
  bool enabled = false;
  String url;
  String username;
  String password;
  int gmtOffset = 0;
  int dstOffset = 0;
};

struct TftDisplayConfig {
  uint8_t cs;          // Chip select pin
  uint8_t dc;          // Data/command pin
  uint8_t rst;         // Reset pin
  uint8_t orientation; // Display rotation (0–3)
  uint8_t totalLines;
};

// ─────────────────────────────────────────────────────────────
// FirmwareManifest Implementation
// ─────────────────────────────────────────────────────────────

inline bool FirmwareManifest::parse(const String& json) {
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) return false;

  JsonObject root = doc.as<JsonObject>();
  version     = root["version"].as<String>();
  url         = root["url"].as<String>();
  configUrl   = root["configUrl"].as<String>();
  checksum    = root["checksum"].as<String>();
  description = root["description"].as<String>();
  mandatory   = root["mandatory"] | false;

  String typeStr = root["updateType"].as<String>();
  typeStr.toLowerCase();
  if (typeStr == "binary")       updateType = UpdateType::Binary;
  else if (typeStr == "user")    updateType = UpdateType::User;
  else if (typeStr == "admin")   updateType = UpdateType::Admin;
  else if (typeStr == "all")     updateType = UpdateType::All;
  else if (typeStr == "debugon") updateType = UpdateType::DebugEnable;
  else if (typeStr == "debugoff")updateType = UpdateType::DebugDisable;
  else                           updateType = UpdateType::Unknown;

  return true;
}