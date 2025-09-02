#pragma once
#include "MessageConsumer.h"
#include "BaseComponent.h"

class MessageDispatcher : public BaseComponent {
public:
  void registerConsumer(MessageConsumer* consumer);
  void publish(const TemperatureMessage& msg);
  void start();

private:
  static constexpr size_t MAX_CONSUMERS = 8;
  MessageConsumer* consumers[MAX_CONSUMERS] = {};
  size_t consumerCount = 0;
};