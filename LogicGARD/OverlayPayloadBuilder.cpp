#include <ArduinoJson.h>
#include "OverlayPayloadBuilder.h"

String OverlayPayloadBuilder::buildTextPayload(const OverlayConfig& config, int identity, time_t timestamp, int temp) {
  StaticJsonDocument<512> doc;
  doc["apiVersion"] = "1.0";
  JsonObject params = doc.createNestedObject("params");

  String text = OverlayPayloadBuilder::formatOverlay(config.text, timestamp, temp);

  if (identity > 0) {
    doc["method"] = "setText";
    params["identity"] = identity;
    params["text"] = text;
  } else {
    doc["method"] = "addText";
    params["camera"] = config.camera;
    params["indicator"] = config.indicator;
    params["text"] = text;
    params["position"] = config.position;
    params["fontSize"] = config.fontSize;
    params["textColor"] = config.textColor;
  }

  char buffer[512];
  size_t len = serializeJson(doc, buffer, sizeof(buffer));

  return String(buffer, len);
}

String OverlayPayloadBuilder::formatOverlay(String templateStr, time_t timestamp, int temp) {
  struct tm* timeinfo = localtime(&timestamp);
  
  if (!timeinfo) {
    Serial.println("[OverlayPayloadBuilder] Failed to parse timestamp");
    return templateStr;
  }

  int timeStart = templateStr.indexOf("{time:");
  int timeEnd = templateStr.indexOf("}", timeStart);
  if (timeStart != -1 && timeEnd != -1) {
    String formatSpec = templateStr.substring(timeStart + 6, timeEnd);

    char timeBuf[64];
    strftime(timeBuf, sizeof(timeBuf), formatSpec.c_str(), timeinfo);

    String formattedTime = String(timeBuf);
    templateStr.replace(templateStr.substring(timeStart, timeEnd + 1), formattedTime);
  } else {
    Serial.println("[OverlayPayloadBuilder] No valid time token found.");
  }

  templateStr.replace("{temp}", String(temp));

  return templateStr;
}