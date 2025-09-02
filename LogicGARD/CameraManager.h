#pragma once

#include <vector>
#include "Types.h"
#include "SecureHttpClient.h"
#include "DigestAuthStrategy.h"
#include "OverlayManager.h"
#include "TemperatureMessage.h"
#include "BaseComponent.h"
#include "IDisplay.h"

class CameraManager : public BaseComponent, public IDisplay {
public:
  CameraManager(const CameraConfig& cameraConfig);

  void begin(MessageDispatcher& dispatcher);

  // IDisplay interface
  String getText() const override;
  int getFlag() const override;

private:
  CameraConfig cameraConfig;
  std::vector<std::unique_ptr<OverlayManager>> overlayManagers;
};