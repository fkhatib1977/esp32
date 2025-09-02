#pragma once
#include <Arduino.h>
#include <Client.h>
#include "Types.h"
#include "BaseComponent.h"
#include "HttpClientWrapper.h"

class LanManager : public BaseComponent {
public:
  void begin(const NetworkConfig& config);
  bool isConnected() const;
  IPAddress getLocalIP() const;
  Client* getClient() const;

private:
  NetworkConfig netConfig;
  bool connected = false;
  Client* transportClient = nullptr;

  void setupLAN();
  void setupWiFi();
  bool checkInternetConnectivity();
};