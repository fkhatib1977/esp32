#include "HttpClientWrapper.h"

HttpClientWrapper::HttpClientWrapper() {
  client = std::make_unique<HTTPClient>();
  mutex = xSemaphoreCreateMutex();
  if (!mutex) {
    BaseComponent::debugLog("[HttpClientWrapper] ❌ Failed to create mutex");
  }
}

HTTPClient& HttpClientWrapper::get() {
  if (!client) {
    client = std::make_unique<HTTPClient>();
    usageCount = 0;
  }
  return *client;
}

int HttpClientWrapper::performPost(const String& url, const String& payload, const String& authHeader, String* responseOut) {
  if (!client) {
    client = std::make_unique<HTTPClient>();
    usageCount = 0;
  }

  BaseComponent::debugLog("[HttpClientWrapper] ➡️ Performing POST to: " + url);

  client->begin(url);
  client->addHeader("Content-Type", "application/json");
  
  if (authHeader.isEmpty()) {
    const char* headersToCollect[] = { "WWW-Authenticate" };
    client->collectHeaders(headersToCollect, 1);
  } else {
    client->addHeader("Authorization", authHeader);
  }

  int code = client->POST(payload);
  String response = client->getString();
  client->end();

  usageCount++;
  lastUsed = millis();

  if (code <= 0 || code >= 500 || usageCount >= MAX_USAGE) {
    client.reset();
    usageCount = 0;
    BaseComponent::debugLog("[HttpClientWrapper] 🔄 httpClient has been reset");
  }

  if (responseOut) {
    *responseOut = response;
  }

  xSemaphoreGive(mutex);
  return code;
}