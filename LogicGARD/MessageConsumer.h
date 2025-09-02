#pragma once
#include <Arduino.h>
#include "TemperatureMessage.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include "BaseComponent.h"

class MessageConsumer : public BaseComponent {
public:
  explicit MessageConsumer(const String& sensorId);
  virtual void begin();
  void enqueue(const TemperatureMessage& msg);
  const String& getSensorId() const { return sensorId; }

protected:
  virtual void process(const TemperatureMessage& msg) = 0;

private:
  void run();
  static void taskEntry(void* param);

  const String sensorId;
  QueueHandle_t queue = nullptr;
};