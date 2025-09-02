#include "Sensor_I2C.h"
#include "TimeUtils.h"

Sensor_I2C::Sensor_I2C(MessageDispatcher& dispatcher, const SensorConfig& config)
  : dispatcher(dispatcher), config(config) {}

void Sensor_I2C::begin() {
  BaseComponent::debugLog("[I2C] Sensor_I2C::begin - 📟 SensorConfig:");
  BaseComponent::debugLog("[I2C]   SDA Pin: " + String(config.sdaPin));
  BaseComponent::debugLog("[I2C]   SCL Pin: " + String(config.sclPin));
  BaseComponent::debugLog("[I2C]   Sensor ID: " + config.name);

  Wire.begin(config.sdaPin, config.sclPin);

  if (!bme.begin(0x76, &Wire)) {
    BaseComponent::debugLog("[I2C] Sensor_I2C::begin - ❌ Could not find a valid BME280 sensor, check wiring!");
    return;
  }

  BaseComponent::debugLog("[I2C] Sensor_I2C::begin - ✅ BME280 initialized on sensor " + config.name);

  xTaskCreatePinnedToCore(
    task,
    "Sensor_I2C_Task",
    4096,
    this,
    1,
    nullptr,
    1
  );

  BaseComponent::debugLog("[I2C] Sensor_I2C::begin - Task created and pinned to core 1.");
}

void Sensor_I2C::task(void* param) {
  auto* self = static_cast<Sensor_I2C*>(param);
  self->BaseComponent::debugLog("[I2C] Sensor_I2C::task - Task started.");

  vTaskDelay(pdMS_TO_TICKS(5000)); // Initial delay to allow system stabilization

  while (true) {
    int tempF;
    if (self->read(&tempF)) {
      uint32_t timestamp = TimeUtils::getEpochSeconds();
      TemperatureMessage msg{ tempF, timestamp, self->config.name };

      self->BaseComponent::debugLog("[I2C] Sensor_I2C::task - Read temperature: " + String(tempF) +
                     "°F at timestamp: " + String(timestamp));
      self->dispatcher.publish(msg);
    } else {
      self->BaseComponent::debugLog("[I2C] Sensor_I2C::task - ❌ Failed to read temperature.");
    }

    vTaskDelay(pdMS_TO_TICKS(self->config.readIntervalMs));
  }
}

bool Sensor_I2C::read(int* temperature) {
  float tempC = bme.readTemperature();
  if (isnan(tempC)) {
    BaseComponent::debugLog("[I2C] Sensor_I2C::read - Sensor returned NaN.");
    return false;
  }

  float tempF = tempC * 1.8f + 32.0f;
  *temperature = static_cast<int>(roundf(tempF));

  BaseComponent::debugLog("[I2C] Sensor_I2C::read - Converted temperature: " + String(tempC) +
           "°C → " + String(*temperature) + "°F");
  return true;
}

String Sensor_I2C::getName() const {
  return config.name;
}