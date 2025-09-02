#pragma once
#include "AuthStrategy.h"
#include "HttpClientWrapper.h"
#include "Types.h"

class DigestAuthStrategy : public AuthStrategy {
public:
  DigestAuthStrategy(const AuthCredentials& creds);

  bool post(const String& payload, String* responseOut, const ApiConfig& config);

private:
  String getAuthHeader(String method, String uri);
  void parseChallenge(String header);
  String md5(String input);

  String realm, nonce, qop, opaque;
};