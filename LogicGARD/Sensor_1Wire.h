#pragma once
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "SensorBase.h"
#include "MessageDispatcher.h"
#include "Types.h"

class Sensor_1Wire : public SensorBase {
public:
  Sensor_1Wire(MessageDispatcher& dispatcher, const SensorConfig& config);

  void begin() override;
  String getName() const override;

private:
  static void task(void* param);       // 👈 Added for FreeRTOS task loop
  bool read(int* temperature);

  MessageDispatcher& dispatcher;
  const SensorConfig& config;
  OneWire oneWire;
  DallasTemperature sensors;
};