#pragma once

#include <PubSubClient.h>
#include <Client.h>
#include <vector>
#include <Arduino.h>
#include "MessageConsumer.h"
#include "Types.h"
#include "TemperatureMessage.h"
#include "IDisplay.h"

class MqttManager : public MessageConsumer, public IDisplay {
public:
  MqttManager(const String& sensorId, Client& netClient);
  void begin(const MqttConfig& config, const DeviceIdentity& identity);
  void loop();
  void publishMessage(const String& payload);

  // IDisplay interface implementation
  String getText() const override;

protected:
  void process(const TemperatureMessage& msg) override;

private:
  MqttConfig config;
  DeviceIdentity identity;
  PubSubClient mqttClient;
  bool connected = false;

  std::vector<TemperatureMessage> pendingMessages;
  std::vector<String> retryQueue;

  SemaphoreHandle_t msgLock;
  SemaphoreHandle_t retryLock;

  unsigned long lastFlushTime = 0;

  void connectToBroker();
  void flushMessages();
  String buildPayload(const std::vector<TemperatureMessage>& messages);
};