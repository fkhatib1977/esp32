#pragma once
#include <Arduino.h>
#include <HTTPClient.h>
#include <memory>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "BaseComponent.h"

class HttpClientWrapper : public BaseComponent {
public:
  HttpClientWrapper();
  HTTPClient& get();
  bool isValid() const;

  int performPost(const String& url, const String& payload, const String& authHeader = "", String* responseOut = nullptr);

private:
  std::unique_ptr<HTTPClient> client;
  SemaphoreHandle_t mutex;
  uint32_t lastUsed = 0;
  int usageCount = 0;
  static constexpr int MAX_USAGE = 10;
};