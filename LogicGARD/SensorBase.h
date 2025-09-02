#pragma once
#include <Arduino.h>
#include "BaseComponent.h"

class SensorBase :  public BaseComponent {
public:
  virtual void begin() = 0;
  virtual String getName() const = 0;
  virtual ~SensorBase() {}
};