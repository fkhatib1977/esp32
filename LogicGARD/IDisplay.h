#pragma once
#include <Arduino.h>

class IDisplay {
public:
  virtual ~IDisplay() = default;

  // Must be implemented by derived classes to expose display text
  virtual String getText() const = 0;

  // Optional override: returns status flag
  virtual int getFlag() const {
    return flag;
  }

protected:
  String text;  // Display content
  int flag = 0; // -1 = failure, 0 = neutral, 1 = success
};