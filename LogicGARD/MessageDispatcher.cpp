#include "MessageDispatcher.h"

void MessageDispatcher::registerConsumer(MessageConsumer* consumer) {
  if (consumerCount < MAX_CONSUMERS) {
    consumers[consumerCount] = consumer;

    BaseComponent::debugLog("MessageDispatcher::registerConsumer - Registered consumer [" + String(consumerCount) + "]");
    BaseComponent::debugLog("MessageDispatcher::registerConsumer - Consumer [" + String(consumerCount) + "] sensorId: [" + consumer->getSensorId() + "]");
    BaseComponent::debugLog("MessageDispatcher::registerConsumer - Consumer [" + String(consumerCount) + "] pointer: 0x" + String((uintptr_t)consumer, HEX));

    ++consumerCount;
  } else {
    BaseComponent::debugLog("MessageDispatcher::registerConsumer - Error: max consumer limit reached.");
  }
}

void MessageDispatcher::publish(const TemperatureMessage& msg) {
  BaseComponent::debugLog("MessageDispatcher::publish - Publishing message: { temperature: " +
           String(msg.temperature) + ", timestamp: " + String(msg.timestamp) +
           ", sensorId: " + msg.sensorId + " }");

  for (size_t i = 0; i < consumerCount; ++i) {
    if (consumers[i]) {
      const String& consumerSensorId = consumers[i]->getSensorId();

      BaseComponent::debugLog("MessageDispatcher::publish - Checking consumer [" + String(i) + "]");
      BaseComponent::debugLog("Comparing msg.sensorId: [" + msg.sensorId + "] vs consumer.sensorId: [" + consumerSensorId + "]");
      BaseComponent::debugLog("Equality result: " + String(msg.sensorId == consumerSensorId));

      if (msg.sensorId == consumerSensorId || consumerSensorId == "*") {
        BaseComponent::debugLog("MessageDispatcher::publish - Match found. Dispatching to consumer [" + String(i) + "]");
        consumers[i]->enqueue(msg);
      } else {
        BaseComponent::debugLog("MessageDispatcher::publish - Skipped consumer [" + String(i) + "] due to sensorId mismatch.");
      }
    } else {
      BaseComponent::debugLog("MessageDispatcher::publish - Warning: consumer[" + String(i) + "] is null.");
    }
  }
}