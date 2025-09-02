#include "Sensor_1Wire.h"
#include "TimeUtils.h"

Sensor_1Wire::Sensor_1Wire(MessageDispatcher& dispatcher, const SensorConfig& config)
  : dispatcher(dispatcher),
    config(config),
    oneWire(config.onewirePin),
    sensors(&oneWire) {
  BaseComponent::debugLog("[1Wire] initializing sensor name " + config.name);
}

void Sensor_1Wire::begin() {
  BaseComponent::debugLog("[1Wire] Sensor_1Wire::begin - Initializing sensor on GPIO " + String(config.onewirePin));

  sensors.begin();
  sensors.setResolution(12);

  DeviceAddress addr;
  if (!sensors.getAddress(addr, 0)) {
    BaseComponent::debugLog("[1Wire] Sensor_1Wire::begin - ❌ No DS18B20 sensor found.");
    return;
  }

  String addrStr;
  for (uint8_t i = 0; i < 8; i++) {
    addrStr += String(addr[i], HEX) + " ";
  }
  BaseComponent::debugLog("[1Wire] Sensor_1Wire::begin - ✅ Found DS18B20 at address: " + addrStr);

  xTaskCreatePinnedToCore(
    task,
    "Sensor_1Wire_Task",
    2048,
    this,
    1,
    nullptr,
    1
  );

  BaseComponent::debugLog("[1Wire] Sensor_1Wire::begin - Task created and pinned to core 1.");
}

void Sensor_1Wire::task(void* param) {
  Sensor_1Wire* self = static_cast<Sensor_1Wire*>(param);
  self->BaseComponent::debugLog("[1Wire] Sensor_1Wire::task - Task started.");

  vTaskDelay(pdMS_TO_TICKS(5000)); // Initial delay to allow system stabilization

  while (true) {
    int tempF;
    if (self->read(&tempF)) {
      uint32_t timestamp = TimeUtils::getEpochSeconds();
      TemperatureMessage msg{ tempF, timestamp, self->config.name };

      String formattedTime = TimeUtils::formatIsoTimestamp(timestamp);
      self->BaseComponent::debugLog("[1Wire] Sensor_1Wire::task - 📤 Publishing temperature: " + String(tempF) +
                     "°F at " + formattedTime + " from sensor: " + self->config.name);
      self->dispatcher.publish(msg);
    } else {
      self->BaseComponent::debugLog("[1Wire] Sensor_1Wire::task - ❌ Failed to read temperature from sensor.");
    }

    self->BaseComponent::debugLog("[1Wire] Sensor " + self->config.name + " will sleep for " + String(self->config.readIntervalMs));
    vTaskDelay(pdMS_TO_TICKS(self->config.readIntervalMs));
  }
}

bool Sensor_1Wire::read(int* tempF) {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (tempC == DEVICE_DISCONNECTED_C) {
    BaseComponent::debugLog("[1Wire] Sensor_1Wire::read - Sensor disconnected.");
    return false;
  }

  float tempF_val = (tempC * 9.0 / 5.0) + 32.0;
  *tempF = static_cast<int>(round(tempF_val));

  BaseComponent::debugLog("[1Wire] Sensor_1Wire::read - Converted temperature: " + String(tempC) +
           "°C → " + String(*tempF) + "°F");
  return true;
}

String Sensor_1Wire::getName() const {
  return config.name;
}