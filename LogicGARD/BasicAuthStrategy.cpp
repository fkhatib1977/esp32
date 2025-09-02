#include "BasicAuthStrategy.h"
#include <base64.h>

BasicAuthStrategy::BasicAuthStrategy(const AuthCredentials& creds)
  : AuthStrategy(creds) {}

bool BasicAuthStrategy::post(const String& payload, String* responseOut, const ApiConfig& config) {
  if (!httpClient || !httpClient->isValid()) {
    Serial.println("[BasicAuth] Invalid HTTP client");
    return false;
  }

  String url = config.fullUrl();
  String authHeader = getAuthHeader("POST", config.path);

  if (authHeader.isEmpty()) {
    Serial.println("[BasicAuth] Failed to generate Authorization header");
    return false;
  }

  int code = httpClient->performPost(url, payload, authHeader, responseOut);
  return code == 200 || code == 1;
}

String BasicAuthStrategy::getAuthHeader(String method, String uri) {
  if (credentials.username.isEmpty() || credentials.password.isEmpty()) {
    return String();
  }

  char raw[128];
  size_t len = snprintf(raw, sizeof(raw), "%s:%s",
                        credentials.username.c_str(),
                        credentials.password.c_str());

  if (len >= sizeof(raw)) {
    return String();
  }

  String encoded = base64::encode(raw);

  char header[160];
  len = snprintf(header, sizeof(header), "Basic %s", encoded.c_str());

  if (len >= sizeof(header)) {
    return String();
  }

  return String(header);
}