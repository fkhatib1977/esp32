#include <ArduinoJson.h>
#include <cstring>
#include <cstdio>

#include "OverlayManager.h"
#include "OverlayPayloadBuilder.h"
#include "TimeUtils.h"

OverlayManager::OverlayManager(const OverlayConfig& config, std::shared_ptr<SecureHttpClient> client)
  : MessageConsumer(config.sensorId),
    config(config),
    client(client) {
  BaseComponent::debugLog("OverlayManager::constructor - 🛠 Creating OverlayManager instance...");

  BaseComponent::debugLog("  sensor: [" + config.sensorId + "]");
  BaseComponent::debugLog("  identity: [" + String(config.identity) + "]");
  BaseComponent::debugLog("  camera: [" + String(config.camera) + "]");
  BaseComponent::debugLog("  indicator: [" + config.indicator + "]");
  BaseComponent::debugLog("  text: [" + config.text + "]");
  BaseComponent::debugLog("  position: [" + config.position + "]");
  BaseComponent::debugLog("  fontSize: [" + String(config.fontSize) + "]");
  BaseComponent::debugLog("  textColor: [" + config.textColor + "]");

  flag = 0;  // neutral until first update
}

void OverlayManager::begin(MessageDispatcher& dispatcher) {
  BaseComponent::debugLog("OverlayManager::begin - Registering with dispatcher...");
  MessageConsumer::begin();
  dispatcher.registerConsumer(this);
  BaseComponent::debugLog("OverlayManager::begin - Registration complete.");
}

void OverlayManager::process(const TemperatureMessage& msg) {
  BaseComponent::debugLog("OverlayManager::process - Received temperature message.");

  auto identity = config.identity;

  if (identity == 0) {
    identity = resolveIdentity(config.indicator);
    BaseComponent::debugLog("OverlayManager::process - Resolved identity: " + String(identity));
  }

  if (identity < 0) {
    BaseComponent::debugLog("OverlayManager::process - Error: invalid identity, aborting overlay update.");
    flag = -1;
    return;
  }

  String payload = OverlayPayloadBuilder::buildTextPayload(
    config, identity, msg.timestamp, msg.temperature);

  if (payload.isEmpty()) {
    BaseComponent::debugLog("OverlayManager::process - Error: failed to build payload for overlay '" + config.indicator + "'");
    flag = -1;
    return;
  }

  BaseComponent::debugLog("OverlayManager::process - Posting payload: " + payload);
  bool success = client->post(payload.c_str());

  flag = success ? 1 : -1;
}

int OverlayManager::resolveIdentity(String indicator) {
  BaseComponent::debugLog("OverlayManager::resolveIdentity - Resolving identity for indicator: " + indicator);

  String listPayload = R"({
    "apiVersion": "1.0",
    "method": "list",
    "params": {}
  })";

  String response;
  int code = client->postWithResponse(listPayload, response);

  if (code <= 0) {
    BaseComponent::debugLog("OverlayManager::resolveIdentity - Error: failed to fetch overlay list");
    flag = -1;
    return -1;
  }

  BaseComponent::debugLog("OverlayManager::resolveIdentity - Received response: " + response);

  DynamicJsonDocument doc(8192);
  DeserializationError err = deserializeJson(doc, response);
  if (err) {
    BaseComponent::debugLog("OverlayManager::resolveIdentity - Error: JSON parse failed: " + String(err.c_str()));
    flag = -1;
    return -2;
  }

  JsonArray overlays = doc["data"]["textOverlays"].as<JsonArray>();
  for (JsonObject overlay : overlays) {
    String found = overlay["indicator"];
    int id = overlay["identity"];
    if (found == indicator) {
      BaseComponent::debugLog("OverlayManager::resolveIdentity - Match found: " + found + " → ID " + String(id));
      return id;
    }
  }

  BaseComponent::debugLog("OverlayManager::resolveIdentity - Error: no matching indicator found");
  flag = -1;
  return 0;
}