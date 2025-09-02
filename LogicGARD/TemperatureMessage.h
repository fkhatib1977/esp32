#pragma once
#include <Arduino.h>
#include <stdint.h>

struct TemperatureMessage {
  int temperature;       // Stored as Fahrenheit
  uint32_t timestamp;
  String sensorId;

  String toJson() const {
    return "{\"temperature\":" + String(temperature) +
           ",\"timestamp\":" + String(timestamp) +
           ",\"sensorId\":\"" + sensorId + "\"}";
  }
};