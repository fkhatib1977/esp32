#include "DigestAuthStrategy.h"
#include <MD5Builder.h>

DigestAuthStrategy::DigestAuthStrategy(const AuthCredentials& creds)
  : AuthStrategy(creds) {}

bool DigestAuthStrategy::post(const String& payload, String* responseOut, const ApiConfig& config) {
  String url = config.fullUrl();
  String challengeResponse;

  int challengeCode = httpClient->performPost(url, "", "", &challengeResponse);
  String authHeader = httpClient->get().header("WWW-Authenticate");

  if (authHeader.isEmpty()) {
    BaseComponent::debugLog("Error: Challenge failed or header missing.");
    return false;
  }

  parseChallenge(authHeader);

  if (realm.isEmpty() || nonce.isEmpty() || qop.isEmpty()) {
    BaseComponent::debugLog("Error: Missing required digest fields.");
    return false;
  }

  String digestHeader = getAuthHeader("POST", config.path);
  int finalCode = httpClient->performPost(url, payload, digestHeader, responseOut);

  if (finalCode != 200) {
    BaseComponent::debugLog("Error: Final response code = " + String(finalCode));
  }

  return finalCode == 200;
}

String DigestAuthStrategy::getAuthHeader(String method, String uri) {
  String nc = "00000001";
  String cnonce = String(random(100000, 999999), HEX);

  String ha1 = md5(credentials.username + ":" + realm + ":" + credentials.password);
  String ha2 = md5(method + ":" + uri);
  String response = md5(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2);

  String header = "Digest username=\"" + credentials.username +
                  "\", realm=\"" + realm +
                  "\", nonce=\"" + nonce +
                  "\", uri=\"" + uri +
                  "\", qop=\"" + qop + "\"" +
                  ", nc=" + nc +
                  ", cnonce=\"" + cnonce +
                  "\", response=\"" + response + "\"";

  if (!opaque.isEmpty()) {
    header += ", opaque=\"" + opaque + "\"";
  }

  BaseComponent::debugLog("Generated Digest header: " + header);
  return header;
}

void DigestAuthStrategy::parseChallenge(String header) {
  int idx = header.indexOf("Digest ");
  if (idx == -1) {
    BaseComponent::debugLog("Error: No Digest prefix found.");
    return;
  }

  String digestPart = header.substring(idx + 7);
  digestPart.trim();

  int pos = 0;
  while (pos < digestPart.length()) {
    int eq = digestPart.indexOf('=', pos);
    if (eq == -1) break;

    String key = digestPart.substring(pos, eq); key.trim();
    int valStart = eq + 1;
    String value;

    if (digestPart.charAt(valStart) == '"') {
      valStart++;
      int valEnd = digestPart.indexOf('"', valStart);
      value = digestPart.substring(valStart, valEnd);
      pos = valEnd + 1;
    } else {
      int valEnd = digestPart.indexOf(',', valStart);
      if (valEnd == -1) valEnd = digestPart.length();
      value = digestPart.substring(valStart, valEnd);
      pos = valEnd;
    }

    key.toLowerCase();

    if (key == "realm") realm = value;
    else if (key == "nonce") nonce = value;
    else if (key == "qop") qop = value;
    else if (key == "opaque") opaque = value;

    while (pos < digestPart.length() &&
           (digestPart.charAt(pos) == ',' || digestPart.charAt(pos) == ' ')) {
      pos++;
    }
  }

  BaseComponent::debugLog("Parsed challenge: realm=" + realm + ", nonce=" + nonce + ", qop=" + qop + ", opaque=" + opaque);
}

String DigestAuthStrategy::md5(String input) {
  MD5Builder md5;
  md5.begin();
  md5.add(input);
  md5.calculate();
  return md5.toString();
}