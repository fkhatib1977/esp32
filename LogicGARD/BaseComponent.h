#pragma once
#include <Arduino.h>

class BaseComponent {
public:
  static void enableDebug(bool enabled) {
    debugEnabled = enabled;
  }

  static void debugLog(const String& msg) {
    if (debugEnabled) {
      Serial.print("[Debug] ");
      Serial.println(msg);
    }
  }

protected:
  static bool debugEnabled;
};