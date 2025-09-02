#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <time.h>
#include <cstdint>
#include <Adafruit_GC9A01A.h>

// Managers
#include "MessageDispatcher.h"
#include "ConfigManager.h"
#include "AdminConfigManager.h"
#include "WebServerManager.h"
#include "SensorManager.h"
#include "SecureHttpClient.h"
#include "Sensor_I2C.h"
#include "MqttManager.h"
#include "ButtonHandler.h"
#include "TimeProvider.h"
#include "CameraManager.h"
#include "LanManager.h"
#include "OtaManager.h"
#include "IpDisplay.h"

#define BOOT_BUTTON 0

// System State
enum class SystemMode {
  Setup,
  Operation
};

// Global Instances
MessageDispatcher dispatcher;
AdminConfigManager adminConfig;
ConfigManager userConfig;
WebServerManager webServer;
LanManager lanManager;
SystemMode currentMode = SystemMode::Setup;

std::unique_ptr<MqttManager> mqtt;
OtaManager* otaManager = nullptr;
std::vector<CameraConfig> cameraList;
std::vector<std::unique_ptr<CameraManager>> cameraManagers;

std::unique_ptr<TimeProvider> timeProvider;
std::unique_ptr<SensorManager> sensorManager;

void initializeFilesystem() {
  if (!SPIFFS.begin(true)) {
    Serial.println("[ERROR] Failed to mount SPIFFS");
  } else {
    Serial.println("✅ SPIFFS mounted");
  }
}

void initializeConfigManagers() {
  adminConfig.begin();
  userConfig.begin();
  Serial.println("✅ Config managers initialized");
}

SystemMode determineSystemMode() {
  if (!userConfig.isConfigured()) {
    Serial.println("⚙️ Device not configured — entering setup mode.");
    currentMode = SystemMode::Setup;

    AuthCredentials cred = adminConfig.getAccessPointCred();
    WiFi.softAP(cred.username.c_str(), cred.password.c_str());
    Serial.printf("✅ Access Point %s started\r\n", cred.username.c_str());

    webServer.begin(userConfig, adminConfig);
  } else {
    currentMode = SystemMode::Operation;
    Serial.println("🔧 Device in operation mode.");
  }
  return currentMode;
}

bool initializeNetwork(uint32_t timeoutMs = 10000) {
  auto networkConfig = userConfig.getNetworkConfig();
  lanManager.begin(networkConfig);
  return lanManager.isConnected();
}

void initializeTimeProvider() {
  auto timeType = adminConfig.getTimeProviderType();
  auto ntp = adminConfig.getNtpConfig();
  auto rtc = adminConfig.getRtcConfig();

  timeProvider = std::make_unique<TimeProvider>(timeType, ntp, rtc);
  timeProvider->begin();
  Serial.println("✅ TimeProvider initialized");
}

void initializeSensorManager() {
  auto sensors = adminConfig.getSensors();  // returns std::vector<SensorConfig>
  sensorManager = std::make_unique<SensorManager>(dispatcher, sensors);
  sensorManager->begin();
  Serial.println("✅ SensorManager initialized");
}

void initializeCameraManager() {
  cameraList = userConfig.getCameraConfigList();

  for (const auto& cam : cameraList) {
    if (!cam.enabled) {
      Serial.printf("⚠️ Skipping disabled camera '%s'\r\n", cam.api.host.c_str());
      continue;
    }

    Serial.printf("🔌 Setting up Camera with IP '%s'\r\n", cam.api.host.c_str());

    auto manager = std::make_unique<CameraManager>(cam);
    manager->begin(dispatcher);
    cameraManagers.push_back(std::move(manager));
  }

  Serial.printf("✅ Initialized %d camera(s)\r\n", cameraManagers.size());
}

void initializeMqttManager() {
  MqttConfig mqttConfig = adminConfig.getMqttConfig();
  if (!mqttConfig.enabled) {
    Serial.println("⚠️ MQTT is disabled in configuration");
    return;
  }

  DeviceIdentity identity = userConfig.getDeviceIdentity();
  Client* netClient = lanManager.getClient();

  if (!netClient) {
    Serial.println("❌ No transport client available (null pointer)");
    return;
  }

  mqtt = std::make_unique<MqttManager>(mqttConfig.sensorId, *netClient);
  mqtt->begin(mqttConfig, identity);
  dispatcher.registerConsumer(mqtt.get());

  Serial.println("✅ MqttManager initialized");
}

void initializeOtaManager() {
  OtaConfig otaConfig = adminConfig.getOtaConfig();
  DeviceIdentity identity = userConfig.getDeviceIdentity();

  if (!otaConfig.enabled || !identity.isValid()) {
    Serial.println("[OTA] ⚠️ OTA disabled or identity invalid");
    return;
  }

  otaManager = new OtaManager(otaConfig, identity);
  otaManager->begin();
  Serial.println("[OTA] ✅ OTA Manager initialized");
}

Adafruit_GC9A01A* tft = nullptr;  // Global display driver
IpDisplay* ipDisplay = nullptr; // Global pointer to IpDisplay

void initializeDisplay() {
  auto tftConfig = adminConfig.getTftDisplayConfig();

  tft = new Adafruit_GC9A01A(tftConfig.cs, tftConfig.dc, tftConfig.rst);
  tft->begin();

  ipDisplay = new IpDisplay(*tft, 4, 3, tftConfig.orientation);
  ipDisplay->begin(tftConfig.totalLines);
}

void initializeButtons() {
  initBootButton(BOOT_BUTTON);

  setShortPressCallback([] () {
    userConfig.setConfigured(false);
    delay(100);
    ESP.restart();
  });

  setLongPressCallback([] () {
    Serial.println("🛠️ Long press detected!");
    userConfig.restoreSettings();
    delay(100);
    ESP.restart();
  });

  Serial.println("✅ Button handlers initialized");
}

void updateDiagnosticsDisplay() {
  if (!ipDisplay) return;

  if (mqtt) {
    ipDisplay->setIp(mqtt->getText(), mqtt->getFlag());
  }

  if(otaManager) {
    ipDisplay->setIp(otaManager->getText(), otaManager->getFlag());
  }

  for (const auto& cam : cameraManagers) {
    if (cam) {
      ipDisplay->setIp(cam->getText(), cam->getFlag());
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  initializeButtons();
  initializeFilesystem();
  initializeConfigManagers();
  initializeDisplay();
  
  if (!initializeNetwork()) {
    Serial.println("❌ Network initialization failed.");
    return;
  }
  
  initializeTimeProvider();
  
  if (determineSystemMode() == SystemMode::Setup) return;
  
  initializeCameraManager();
  initializeMqttManager();
  initializeSensorManager();
  initializeOtaManager();
}

unsigned long lastDiagUpdate = 0;
const unsigned long diagInterval = 3000;

void loop() {
  unsigned long now = millis();

  if (currentMode == SystemMode::Setup) {
    webServer.loop();
  } else {
    monitorBootButton();
    if (mqtt) mqtt->loop();
    if (otaManager) otaManager->loop();
    delay(3000);
  }
  
  if (now - lastDiagUpdate >= diagInterval) {
    updateDiagnosticsDisplay();
    lastDiagUpdate = now;
  }
}