#include "Types.h"  // For DeviceIdentity and AuthCredentials
#include "FirmwareStateTracker.h"
#include "BaseComponent.h"
#include "ConfigBase.h"
#include "IDisplay.h"

class OtaManager : public BaseComponent, public IDisplay {
public:
  OtaManager(const OtaConfig& config, const DeviceIdentity& identity);
  void begin();
  void loop();

  // IDisplay interface
  String getText() const override;

private:
  OtaConfig config;
  DeviceIdentity identity;
  FirmwareStateTracker stateTracker;
  unsigned long lastCheckTime = 0;

  bool shouldCheck();
  bool fetchManifest(FirmwareManifest& manifest);
  void applyUpdate(const FirmwareManifest& manifest);
  void performFirmwareUpdate(const FirmwareManifest& manifest);
  void fetchAndApplyConfig(const String& url, ConfigBase& fileConfig, const String& label);
};