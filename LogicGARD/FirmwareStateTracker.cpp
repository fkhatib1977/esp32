#include "FirmwareStateTracker.h"
#include <ArduinoJson.h>
#include <MD5Builder.h>

void FirmwareStateTracker::begin() {
  prefs.begin("ota", false);
  currentVersion = prefs.getString("currentVersion", "1.0.0");
}

String FirmwareStateTracker::getLastAppliedVersion() {
  return prefs.getString("lastApplied", "");
}

void FirmwareStateTracker::markVersionApplied(const String& version) {
  prefs.putString("lastApplied", version);
  prefs.putString("currentVersion", version);
  currentVersion = version;
}

bool FirmwareStateTracker::shouldApplyUpdate(const String& manifestVersion, bool mandatory) {
  String lastApplied = getLastAppliedVersion();

  if (mandatory) return true;
  if (manifestVersion > currentVersion) return true;

  return false;
}

bool FirmwareStateTracker::validateChecksum(const uint8_t* data, size_t length, String checksum) const {
  MD5Builder md5;
  md5.begin();
  md5.add(data, length);
  md5.calculate();
  String computed = md5.toString();
  return computed.equalsIgnoreCase(checksum);
}