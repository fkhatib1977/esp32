#include "SecureHttpClient.h"

SecureHttpClient::SecureHttpClient(std::unique_ptr<AuthStrategy> authStrategy, const ApiConfig& cfg)
  : auth(std::move(authStrategy)), config(cfg) {
  BaseComponent::debugLog("SecureHttpClient::constructor - Initialized with API: " + config.fullUrl());
}

int SecureHttpClient::post(String payload) {
  BaseComponent::debugLog("SecureHttpClient::post - Sending payload (no response expected):");
  BaseComponent::debugLog("  " + payload);

  int code = auth->post(payload, nullptr, config);
  BaseComponent::debugLog("SecureHttpClient::post - HTTP result code: " + String(code));

  return code;
}

int SecureHttpClient::postWithResponse(String payload, String& response) {
  BaseComponent::debugLog("SecureHttpClient::postWithResponse - Sending payload:");
  BaseComponent::debugLog("  " + payload);

  int code = auth->post(payload, &response, config);
  BaseComponent::debugLog("SecureHttpClient::postWithResponse - HTTP result code: " + String(code));
  BaseComponent::debugLog("SecureHttpClient::postWithResponse - Response body: " + response);

  return code;
}