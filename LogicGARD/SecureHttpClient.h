#pragma once
#include "AuthStrategy.h"
#include "Types.h"
#include "BaseComponent.h"

class SecureHttpClient : public BaseComponent {
public:
  SecureHttpClient(std::unique_ptr<AuthStrategy> authStrategy, const ApiConfig& cfg);

  int post(String payload);
  int postWithResponse(String payload, String& response);

private:
  std::unique_ptr<AuthStrategy> auth;
  ApiConfig config;
};