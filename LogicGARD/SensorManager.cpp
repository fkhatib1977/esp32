#include "SensorManager.h"
#include "Sensor_I2C.h"
#include "Sensor_1Wire.h"

SensorManager::SensorManager(MessageDispatcher& dispatcher, const std::vector<SensorConfig>& inputConfigs)
  : dispatcher(dispatcher) {
  sensorConfigList.reserve(inputConfigs.size());
  for (const auto& cfg : inputConfigs) {
    sensorConfigList.push_back(std::make_unique<SensorConfig>(cfg));  // deep copy into unique_ptr
  }
}

void SensorManager::begin() {
  if (sensorConfigList.empty()) {
    Serial.println("⚠️ No sensors found in config.");
    return;
  }

  for (const auto& configPtr : sensorConfigList) {
    const SensorConfig& config = *configPtr;

    if (!config.enabled) {
      BaseComponent::debugLog("Sensor " + config.name + " is disabled, skipping...");
      continue;
    }

    if (config.interface == "i2c") {
      auto sensor = std::make_unique<Sensor_I2C>(dispatcher, config);
      sensor->begin();
      sensors.push_back(std::move(sensor));
      Serial.printf("✅ Initialized I2C sensor '%s' (SDA=%d, SCL=%d, Interval=%ums)\r\n",
                    config.name.c_str(), config.sdaPin, config.sclPin, config.readIntervalMs);
    } else if (config.interface == "onewire") {
      auto sensor = std::make_unique<Sensor_1Wire>(dispatcher, config);
      sensor->begin();
      sensors.push_back(std::move(sensor));
      Serial.printf("✅ Initialized 1-Wire sensor '%s' (Pin=%d, Interval=%ums)\r\n",
                    config.name.c_str(), config.onewirePin, config.readIntervalMs);
    } else {
      Serial.printf("❌ Unsupported interface '%s' for sensor '%s'\r\n",
                    config.interface.c_str(), config.name.c_str());
      continue;
    }
  }
}