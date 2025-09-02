#pragma once
#include <Preferences.h>

class FirmwareStateTracker {
public:
  void begin();
  String getLastAppliedVersion();
  void markVersionApplied(const String& version);
  bool shouldApplyUpdate(const String& manifestVersion, bool mandatory);
  bool validateChecksum(const uint8_t* data, size_t length, String checksum) const;

private:
  Preferences prefs;
  String currentVersion;
};