#include "CameraManager.h"
#include "OverlayManager.h"

CameraManager::CameraManager(const CameraConfig& cameraConfig)
  : cameraConfig(cameraConfig) {}
  
void CameraManager::begin(MessageDispatcher& dispatcher) {
  BaseComponent::debugLog("CameraManager::begin - Starting initialization of overlay managers...");

  size_t index = 0;
  for (const OverlayConfig& overlayConfig : cameraConfig.overlays) {
    BaseComponent::debugLog("OverlayManager[" + String(index) + "] - Creating with config...");
    BaseComponent::debugLog("  sensor: [" + overlayConfig.sensorId + "]");

    overlayManagers.push_back(std::make_unique<OverlayManager>(
      overlayConfig,
      std::make_shared<SecureHttpClient>(
        std::make_unique<DigestAuthStrategy>(cameraConfig.credentials),
        cameraConfig.api
      )
    ));

    overlayManagers.back()->begin(dispatcher);

    BaseComponent::debugLog("OverlayManager[" + String(index) + "] - Initialized.");
    ++index;
  }
  
  BaseComponent::debugLog("CameraManager::begin - Total overlay managers initialized: " + String(overlayManagers.size()));
}

String CameraManager::getText() const {
  return cameraConfig.api.host;
}

int CameraManager::getFlag() const {
  for (const auto& overlay : overlayManagers) {
    if (overlay && overlay->flag != 1) {
      return -1;
    }
  }
  
  return 1;
}