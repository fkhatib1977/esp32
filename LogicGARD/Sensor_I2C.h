#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#include "SensorBase.h"
#include "MessageDispatcher.h"
#include "TimeUtils.h"
#include "Types.h"

class Sensor_I2C : public SensorBase {
public:
  Sensor_I2C(MessageDispatcher& dispatcher, const SensorConfig& config);
  void begin() override;
  String getName() const override;

private:
  static void task(void* param);       // 👈 Added for FreeRTOS task loop
  bool read(int* temperature);

  MessageDispatcher& dispatcher;
  const SensorConfig& config;
  Adafruit_BME280 bme;
};