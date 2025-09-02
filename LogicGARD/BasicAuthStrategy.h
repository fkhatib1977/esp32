#pragma once
#include "AuthStrategy.h"

class BasicAuthStrategy : public AuthStrategy {
public:
  explicit BasicAuthStrategy(const AuthCredentials& creds);

  bool post(const String& payload, String* responseOut, const ApiConfig& config) override;
  String getAuthHeader(String method, String uri) override;
};