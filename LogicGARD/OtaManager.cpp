#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <base64.h>
#include <esp_ota_ops.h>

#include "OtaManager.h"
#include "ConfigManager.h"
#include "AdminConfigManager.h"
#include "Types.h"

extern ConfigManager userConfig;
extern AdminConfigManager adminConfig;

OtaManager::OtaManager(const OtaConfig& config, const DeviceIdentity& identity)
  : config(config), identity(identity), lastCheckTime(0) {
  text = "OTA Manager";
  flag = 0;
}

void OtaManager::begin() {
  BaseComponent::debugLog("[OTA] Initializing OTA state tracker");
  stateTracker.begin();
}

void OtaManager::loop() {
  if (!config.enabled) {
    BaseComponent::debugLog("[OTA] Skipping loop—OTA disabled");
    flag = -1;
    return;
  }

  if (!shouldCheck()) {
    BaseComponent::debugLog("[OTA] Skipping loop—check interval not reached");
    return;
  }

  BaseComponent::debugLog("[OTA] Starting OTA loop");

  FirmwareManifest manifest;
  if (!fetchManifest(manifest)) {
    BaseComponent::debugLog("[OTA] Manifest fetch failed—skipping update check");
    flag = -1;
    return;
  }

  flag = 1;  // Manifest successfully fetched

  if (stateTracker.shouldApplyUpdate(manifest.version, manifest.mandatory)) {
    BaseComponent::debugLog("[OTA] Update required—applying version " + manifest.version);
    applyUpdate(manifest);
    stateTracker.markVersionApplied(manifest.version);
    BaseComponent::debugLog("[OTA] Update applied and version marked");
  } else {
    BaseComponent::debugLog("[OTA] No update required—version already applied or not mandatory");
  }

  lastCheckTime = millis();
  BaseComponent::debugLog("[OTA] Updated lastCheckTime to " + String(lastCheckTime));
}

bool OtaManager::shouldCheck() {
  uint32_t elapsed = millis() - lastCheckTime;
  BaseComponent::debugLog("[OTA] Time since last check: " + String(elapsed) + " ms");
  return elapsed >= config.checkIntervalMs;
}

bool OtaManager::fetchManifest(FirmwareManifest& manifest) {
  BaseComponent::debugLog("[OTA] Fetching manifest from: " + config.checkInUrl);

  HTTPClient client;
  if (!client.begin(config.checkInUrl)) {
    Serial.println("[OTA] ❌ Failed to initialize HTTPClient with URL: " + config.checkInUrl);
    BaseComponent::debugLog("[OTA] HTTPClient.begin() failed");
    flag = -1;
    return false;
  }

  if (!config.credentials.username.isEmpty()) {
    BaseComponent::debugLog("[OTA] Setting HTTP basic auth");
    client.setAuthorization(
      config.credentials.username.c_str(),
      config.credentials.password.c_str()
    );
  }

  client.addHeader("Content-Type", "application/json");
  String payload = identity.toJson();
  BaseComponent::debugLog("[OTA] Sending identity payload: " + payload);

  int httpCode = client.POST(payload);
  BaseComponent::debugLog("[OTA] HTTP POST returned code: " + String(httpCode));

  if (httpCode == 204) {
    Serial.println("[OTA] ✅ No update available");
    BaseComponent::debugLog("[OTA] Server responded with 204—no update");
    client.end();
    return false;
  }

  if (httpCode != 200) {
    Serial.println("[OTA] ❌ Manifest fetch failed, HTTP code: " + String(httpCode));
    BaseComponent::debugLog("[OTA] Unexpected HTTP code: " + String(httpCode));
    flag = -1;
    client.end();
    return false;
  }

  String response = client.getString();
  BaseComponent::debugLog("[OTA] 🔍 Raw manifest response:");
  BaseComponent::debugLog(response);
  client.end();

  if (!manifest.parse(response)) {
    BaseComponent::debugLog("[OTA] ❌ Failed to parse manifest");
    flag = -1;
    return false;
  }

  BaseComponent::debugLog("[OTA] ✅ Manifest parsed:");
  BaseComponent::debugLog("  version: " + manifest.version);
  BaseComponent::debugLog("  url: " + manifest.url);
  BaseComponent::debugLog("  configUrl: " + manifest.configUrl);
  BaseComponent::debugLog("  checksum: " + manifest.checksum);
  BaseComponent::debugLog("  description: " + manifest.description);
  BaseComponent::debugLog("  mandatory: " + String(manifest.mandatory ? "true" : "false"));
  BaseComponent::debugLog("  updateType: " + String(static_cast<int>(manifest.updateType)));

  return true;
}

void OtaManager::applyUpdate(const FirmwareManifest& manifest) {
  BaseComponent::debugLog("[OTA] Applying update for version: " + manifest.version);

  if ((manifest.updateType == UpdateType::Binary || manifest.updateType == UpdateType::All) &&
      !manifest.url.isEmpty()) {
    performFirmwareUpdate(manifest);
  }

  if ((manifest.updateType == UpdateType::User || manifest.updateType == UpdateType::All) &&
      !manifest.configUrl.isEmpty()) {
    BaseComponent::debugLog("[OTA] Fetching user config from: " + manifest.configUrl);
    fetchAndApplyConfig(manifest.configUrl, userConfig, "User");
  }

  if ((manifest.updateType == UpdateType::Admin || manifest.updateType == UpdateType::All) &&
      !manifest.configUrl.isEmpty()) {
    BaseComponent::debugLog("[OTA] Fetching admin config from: " + manifest.configUrl);
    fetchAndApplyConfig(manifest.configUrl, adminConfig, "Admin");
  }

  if (manifest.updateType == UpdateType::DebugEnable) {
    Serial.println("[OTA] 🐞 Enabling debug logging as per manifest");
    BaseComponent::debugLog("[OTA] Enabling debug logging");
    BaseComponent::enableDebug(true);
  }

  if (manifest.updateType == UpdateType::DebugDisable) {
    Serial.println("[OTA] 🐞 Disabling debug logging as per manifest");
    BaseComponent::debugLog("[OTA] Disabling debug logging");
    BaseComponent::enableDebug(false);
  }
}

void OtaManager::performFirmwareUpdate(const FirmwareManifest& manifest) {
  Serial.println("[OTA] Starting OTA update from: " + manifest.url);

  HTTPClient client;
  if (!client.begin(manifest.url)) {
    Serial.println("[OTA] ❌ Failed to initialize HTTPClient with firmware URL");
    BaseComponent::debugLog("[OTA] HTTPClient.begin() failed for URL: " + manifest.url);
    flag = -1;
    return;
  }

  client.setTimeout(20000);

  if (!config.credentials.username.isEmpty()) {
    BaseComponent::debugLog("[OTA] Setting HTTP basic auth");
    client.setAuthorization(
      config.credentials.username.c_str(),
      config.credentials.password.c_str()
    );
  }

  httpUpdate.rebootOnUpdate(false);
  t_httpUpdate_return ret = httpUpdate.update(client);

  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.println("[OTA] ❌ Update failed");
      Serial.println("[OTA] Error code: " + String(httpUpdate.getLastError()));
      Serial.println("[OTA] Error message: " + httpUpdate.getLastErrorString());
      BaseComponent::debugLog("[OTA] Update failed with error: " + httpUpdate.getLastErrorString());
      flag = -1;
      break;

    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("[OTA] ⚠️ No updates available");
      BaseComponent::debugLog("[OTA] Server responded but no update was provided");
      flag = -1;
      break;

    case HTTP_UPDATE_OK:
      Serial.println("[OTA] ✅ Update successful. Rebooting ...");
      BaseComponent::debugLog("[OTA] Firmware update completed. Triggering manual reboot.");
      flag = 1;
      delay(100);
      ESP.restart();
      break;
  }
}

void OtaManager::fetchAndApplyConfig(const String& url, ConfigBase& fileConfig, const String& label) {
  BaseComponent::debugLog("[OTA] Fetching " + label + " config from: " + url);

  HTTPClient client;
  if (!client.begin(url)) {
    Serial.println("[OTA] ❌ Failed to initialize HTTPClient for " + label + " config");
    BaseComponent::debugLog("[OTA] HTTPClient.begin() failed for " + label + " config");
    flag = -1;
    return;
  }

  if (!config.credentials.username.isEmpty()) {
    BaseComponent::debugLog("[OTA] Setting HTTP basic auth for " + label + " config");
    client.setAuthorization(
      config.credentials.username.c_str(),
      config.credentials.password.c_str()
    );
  }

  int httpCode = client.GET();
  BaseComponent::debugLog("[OTA] HTTP GET returned code: " + String(httpCode));

  if (httpCode != 200) {
    Serial.println("[OTA] ❌ " + label + " config fetch failed, HTTP code: " + String(httpCode));
    BaseComponent::debugLog("[OTA] Failed to fetch " + label + " config—HTTP code: " + String(httpCode));
    flag = -1;
    client.end();
    return;
  }

  String configJson = client.getString();
  BaseComponent::debugLog("[OTA] Received " + label + " config JSON:");
  BaseComponent::debugLog(configJson);
  client.end();

  if (!fileConfig.updateFromJsonString(configJson)) {
    Serial.println("[OTA] ❌ Failed to apply " + label + " config");
    BaseComponent::debugLog("[OTA] Failed to apply " + label + " config");
    flag = -1;
  } else {
    Serial.println("[OTA] ✅ " + label + " config applied successfully");
    BaseComponent::debugLog("[OTA] " + label + " config applied successfully, restarting ...");
    flag = 1;
    delay(3000);
    ESP.restart();
  }
}

String OtaManager::getText() const {
  return config.host;
}