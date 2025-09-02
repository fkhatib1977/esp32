#pragma once
#include <Arduino.h>
#include "Types.h"

class OverlayPayloadBuilder {
public:
  static String buildTextPayload(const OverlayConfig& config, int identity, time_t timestamp, int temp);
private:
  static String formatOverlay(String templateStr, time_t timestamp, int temp);
};