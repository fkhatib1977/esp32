#include <cstring>
#include "MessageConsumer.h"

MessageConsumer::MessageConsumer(const String& sensorId)
    : sensorId(sensorId) {
        BaseComponent::debugLog("MessageConsumer constructor reached for sensorId: " + sensorId);
      }

void MessageConsumer::begin() {
  BaseComponent::debugLog("MessageConsumer::begin - Creating message queue...");
  queue = xQueueCreate(10, sizeof(TemperatureMessage));
  if (queue == nullptr) {
    BaseComponent::debugLog("MessageConsumer::begin - Error: failed to create queue.");
    return;
  }

  BaseComponent::debugLog("MessageConsumer::begin - Launching task...");
  xTaskCreate(
    taskEntry, "MsgConsumerTask",
    6144, this, 1, nullptr
  );
  BaseComponent::debugLog("MessageConsumer::begin - Task launched.");
}

void MessageConsumer::enqueue(const TemperatureMessage& msg) {
  xQueueSend(queue, &msg, 0);
}

void MessageConsumer::taskEntry(void* param) {
  static_cast<MessageConsumer*>(param)->run();
}

void MessageConsumer::run() {
  BaseComponent::debugLog("MessageConsumer::run - Task started, waiting for messages...");
  TemperatureMessage msg;
  for (;;) {
    if (xQueueReceive(queue, &msg, portMAX_DELAY)) {
      BaseComponent::debugLog("MessageConsumer::run - Message received.");
      BaseComponent::debugLog("MessageConsumer::run - { temperature: " + String(msg.temperature) +
               ", timestamp: " + String(msg.timestamp) +
               ", sensorId: " + msg.sensorId + " }");

      process(msg);
    }
  }
}