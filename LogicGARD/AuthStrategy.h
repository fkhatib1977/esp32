#pragma once

#include <Arduino.h>
#include "Types.h"
#include "HttpClientWrapper.h"
#include "BaseComponent.h"

class AuthStrategy : public BaseComponent {
public:
  explicit AuthStrategy(const AuthCredentials& creds)
    : credentials(creds),
      httpClient(std::make_unique<HttpClientWrapper>()) {}

  virtual ~AuthStrategy() {}

  // Must be implemented by derived classes
  virtual String getAuthHeader(String method, String uri) = 0;

  // Shared POST logic for all auth strategies
  virtual bool post(const String& payload, String* responseOut, const ApiConfig& config) {
    String url = config.fullUrl();
    String authHeader = getAuthHeader("POST", config.path);

    int code = httpClient->performPost(url, payload, authHeader, responseOut);
    return code == 200 || code == 1;
  }

protected:
  AuthCredentials credentials;
  std::unique_ptr<HttpClientWrapper> httpClient;
};