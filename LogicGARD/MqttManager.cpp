#include "MqttManager.h"
#include "TimeUtils.h"

MqttManager::MqttManager(const String& sensorId, Client& netClient)
  : MessageConsumer(sensorId), mqttClient(netClient) {
  msgLock = xSemaphoreCreateMutex();
  retryLock = xSemaphoreCreateMutex();
  BaseComponent::debugLog("[MQTT] MqttManager constructed for sensorId: " + sensorId);
  if (!msgLock || !retryLock) {
    Serial.println("[MQTT] ❌ Mutex creation failed");
  }
}

void MqttManager::begin(const MqttConfig& config, const DeviceIdentity& identity) {
  this->config = config;
  this->identity = identity;

  BaseComponent::debugLog("[MQTT] begin() called with clientId: " + config.clientId +
           ", broker: " + config.broker + ", port: " + String(config.port));

  MessageConsumer::begin();

  if (!config.enabled) {
    Serial.println("[MQTT] Disabled in config");
    return;
  }

  if (!identity.isValid()) {
    Serial.println("[MQTT] ❌ Invalid device identity");
    return;
  }

  mqttClient.setBufferSize(config.bufferSize);
  mqttClient.setServer(config.broker.c_str(), config.port);
  connectToBroker();
  lastFlushTime = millis();
}

void MqttManager::connectToBroker() {
  BaseComponent::debugLog("[MQTT] Connecting to broker: " + config.broker + ":" + String(config.port));

  while (!mqttClient.connected()) {
    BaseComponent::debugLog("[MQTT] Attempting connection as clientId: " + config.clientId);

    if (mqttClient.connect(config.clientId.c_str(), config.username.c_str(), config.password.c_str())) {
      BaseComponent::debugLog("[MQTT] ✅ Connected");
      connected = true;
      flag = 1;  // Success
    } else {
      Serial.println("[MQTT] ❌ Failed, rc=" + String(mqttClient.state()));
      flag = -1; // Failure
      delay(2000);
    }
  }
}

void MqttManager::loop() {
  if (!config.enabled || !connected) {
    BaseComponent::debugLog("[MQTT] Skipping loop—MQTT disabled or not connected.");
    return;
  }

  mqttClient.loop();

  uint32_t now = millis();
  uint32_t elapsed = now - lastFlushTime;
  int32_t remaining = static_cast<int32_t>(config.flushIntervalMs) - static_cast<int32_t>(elapsed);

  BaseComponent::debugLog("[MQTT] Time since last flush: " + String(elapsed) + " ms");
  BaseComponent::debugLog("[MQTT] Time remaining until next flush: " + String(remaining) + " ms");

  bool timeTrigger = config.flushIntervalMs > 0 && elapsed >= config.flushIntervalMs;

  bool sizeTrigger = false;
  size_t pendingSize = 0;

  if (xSemaphoreTake(msgLock, portMAX_DELAY)) {
    pendingSize = pendingMessages.size();
    sizeTrigger = config.batchSize > 0 && pendingSize >= config.batchSize;
    BaseComponent::debugLog("[MQTT] Pending message count: " + String(pendingSize));
    BaseComponent::debugLog("[MQTT] Size trigger threshold: " + String(config.batchSize));
    xSemaphoreGive(msgLock);
  }

  String flushTimeStr = TimeUtils::formatIsoTimestamp(lastFlushTime);
  BaseComponent::debugLog("[MQTT] Last flush timestamp: " + flushTimeStr);

  if (timeTrigger || sizeTrigger) {
    String triggerReason = String(timeTrigger ? "time" : "") +
                           String((timeTrigger && sizeTrigger) ? " & " : "") +
                           String(sizeTrigger ? "size" : "");
    BaseComponent::debugLog("[MQTT] 🚀 Flush triggered by " + triggerReason);
    flushMessages();
    lastFlushTime = now;
    BaseComponent::debugLog("[MQTT] Flush completed. Updated lastFlushTime to: " + String(now));
  }
}

void MqttManager::process(const TemperatureMessage& msg) {
  if (xSemaphoreTake(msgLock, portMAX_DELAY)) {
    BaseComponent::debugLog("[MQTT] Received message: " + msg.toJson());
    pendingMessages.push_back(msg);
    xSemaphoreGive(msgLock);
  }
}

void MqttManager::flushMessages() {
  if (!mqttClient.connected()) {
    Serial.println("[MQTT] ⚠️ Disconnected, reconnecting...");
    connectToBroker();
  }

  std::vector<TemperatureMessage> toPublish;

  if (xSemaphoreTake(msgLock, portMAX_DELAY)) {
    std::swap(toPublish, pendingMessages);
    xSemaphoreGive(msgLock);
  }

  BaseComponent::debugLog("[MQTT] flushMessages() - toPublish size: " + String(toPublish.size()));

  if (xSemaphoreTake(retryLock, portMAX_DELAY)) {
    BaseComponent::debugLog("[MQTT] Processing retry queue, size: " + String(retryQueue.size()));
    for (auto it = retryQueue.begin(); it != retryQueue.end(); ) {
      bool success = mqttClient.publish(config.topic.c_str(), it->c_str());
      if (success) {
        BaseComponent::debugLog("[MQTT] ✅ Retry succeeded");
        it = retryQueue.erase(it);
        flag = 1;
      } else {
        Serial.println("[MQTT] ❌ Retry failed, keeping message");
        Serial.println("[MQTT] Client state: " + String(mqttClient.state()));
        flag = -1;
        break;
      }
    }
    xSemaphoreGive(retryLock);
  }

  if (toPublish.empty()) {
    Serial.println("[MQTT] No new messages to publish");
    return;
  }

  String payload = buildPayload(toPublish);
  BaseComponent::debugLog("[MQTT] Publishing batch payload: " + payload);

  bool success = mqttClient.publish(config.topic.c_str(), payload.c_str());

  if (!success) {
    Serial.println("[MQTT] ❌ Publish failed, queuing batch");
    flag = -1;
    if (xSemaphoreTake(retryLock, portMAX_DELAY)) {
      retryQueue.push_back(payload);
      xSemaphoreGive(retryLock);
    }
  } else {
    BaseComponent::debugLog("[MQTT] ✅ Batch published");
    BaseComponent::debugLog("[MQTT] Client state: " + String(mqttClient.state()));
    flag = 1;
  }
}

String MqttManager::buildPayload(const std::vector<TemperatureMessage>& messages) {
  String payload = "{";
  payload += "\"device\":{";
  payload += "\"clientId\":\"" + identity.clientId + "\",";
  payload += "\"locationId\":\"" + identity.locationId + "\",";
  payload += "\"unitId\":\"" + identity.unitId + "\",";
  payload += "\"version\":\"" + identity.version + "\",";
  payload += "\"board\":\"" + identity.board + "\"";
  payload += "},";

  payload += "\"messages\":[";
  for (size_t i = 0; i < messages.size(); ++i) {
    payload += messages[i].toJson();
    if (i < messages.size() - 1) payload += ",";
  }
  payload += "]}";
  return payload;
}

void MqttManager::publishMessage(const String& payload) {
  if (!connected || !config.enabled) return;
  BaseComponent::debugLog("[MQTT] Direct publish: " + payload);
  bool success = mqttClient.publish(config.topic.c_str(), payload.c_str());
  flag = success ? 1 : -1;
}

String MqttManager::getText() const {
  return config.broker;
}