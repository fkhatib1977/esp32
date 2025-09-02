#define ETH_PHY_TYPE        ETH_PHY_LAN8720
#define ETH_PHY_ADDR        0
#define ETH_PHY_MDC         23
#define ETH_PHY_MDIO        18
#define ETH_PHY_POWER       -1
#define ETH_CLK_MODE        ETH_CLOCK_GPIO0_IN

#include "LanManager.h"
#include <ETH.h>
#include <WiFi.h>

void LanManager::begin(const NetworkConfig& config) {
  netConfig = config;
  BaseComponent::debugLog("LanManager::begin - Connection type: " + config.connectionTypeName());

  if (transportClient) {
    delete transportClient;
    transportClient = nullptr;
  }

  if (config.connectionType == ConnectionType::LAN) {
    setupLAN();
  } else if (config.connectionType == ConnectionType::WIFI) {
    setupWiFi();
  } else {
    BaseComponent::debugLog("LanManager::begin - ❌ Unknown connection type.");
  }

  IPAddress ip = getLocalIP();
  BaseComponent::debugLog("LanManager::begin - Final IP: " + ip.toString());

  int waitAttempts = 0;
  while ((ip == IPAddress(0, 0, 0, 0) || ip == IPAddress(255, 255, 255, 255)) && waitAttempts < 20) {
    delay(250);
    ip = getLocalIP();
    ++waitAttempts;
  }

  delay(3000);  // Allow LWIP internals to settle

  if (!checkInternetConnectivity()) {
    BaseComponent::debugLog("[LanManager] ❌ No internet. Restarting...");
    delay(1000);
    ESP.restart();
  }

  BaseComponent::debugLog("[LanManager] ✅ Internet connectivity verified.");
}

bool LanManager::isConnected() const {
  return connected;
}

IPAddress LanManager::getLocalIP() const {
  if (netConfig.connectionType == ConnectionType::LAN) {
    return ETH.localIP();
  } else if (netConfig.connectionType == ConnectionType::WIFI) {
    return WiFi.localIP();
  }
  return IPAddress();
}

Client* LanManager::getClient() const {
  return (connected && transportClient) ? transportClient : nullptr;
}

void LanManager::setupLAN() {
  BaseComponent::debugLog("LanManager::setupLAN - Initializing native Ethernet (LAN8720)");

  ETH.begin();  // Uses default PHY config from defines

  int attempts = 0;
  while (!ETH.linkUp() && attempts < 20) {
    delay(250);
    ++attempts;
  }

  IPAddress ip = ETH.localIP();
  connected = ETH.linkUp() && ip != IPAddress(0, 0, 0, 0) && ip != IPAddress(255, 255, 255, 255);

  if (connected) {
    transportClient = new WiFiClient();  // Works for both WiFi and native ETH
    BaseComponent::debugLog("LanManager::setupLAN - ✅ Connected. IP: " + ip.toString());
  } else {
    BaseComponent::debugLog("LanManager::setupLAN - ❌ Failed to connect via LAN. IP: " + ip.toString());
  }
}

void LanManager::setupWiFi() {
  BaseComponent::debugLog("LanManager::setupWiFi - Connecting to SSID: " + netConfig.ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(netConfig.ssid.c_str(), netConfig.password.c_str());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    ++attempts;
  }

  connected = WiFi.status() == WL_CONNECTED;

  if (connected) {
    transportClient = new WiFiClient();
    BaseComponent::debugLog("LanManager::setupWiFi - ✅ Connected. IP: " + WiFi.localIP().toString());
  } else {
    BaseComponent::debugLog("LanManager::setupWiFi - ❌ Failed to connect to WiFi.");
  }
}

bool LanManager::checkInternetConnectivity() {
  const char* host = "example.com";
  const uint16_t port = 80;
  const int maxAttempts = 3;
  const int delayMs = 500;

  BaseComponent::debugLog("[Network] 🌐 Checking internet connectivity via TCP to example.com");

  for (int i = 0; i < maxAttempts; ++i) {
    transportClient->setTimeout(5000);

    if (transportClient->connect(host, port)) {
      transportClient->stop();
      BaseComponent::debugLog("[Network] ✅ TCP connection succeeded");
      return true;
    }

    BaseComponent::debugLog("[Network] ❌ TCP connection to example.com failed (attempt " + String(i + 1) + ")");
    delay(delayMs);
  }

  BaseComponent::debugLog("[Network] ❌ All attempts to connect to example.com failed");
  return false;
}