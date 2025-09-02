#pragma once
#include <Arduino.h>
#include <vector>
#include <memory>
#include "SensorBase.h"
#include "Types.h"
#include "MessageDispatcher.h"
#include "BaseComponent.h"

class SensorManager : public BaseComponent {
public:
  SensorManager(MessageDispatcher& dispatcher, const std::vector<SensorConfig>& inputConfigs);
  void begin();

private:
  MessageDispatcher& dispatcher;
  std::vector<std::unique_ptr<SensorConfig>> sensorConfigList;
  std::vector<std::unique_ptr<SensorBase>> sensors;
};