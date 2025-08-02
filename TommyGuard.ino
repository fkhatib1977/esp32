#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>

const char* ssid = "ESP32_Hotspot";
const char* password = "12345678";
WebServer server(80);

void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");
  if (!file) {
    server.send(404, "text/plain", "File not found");
    return;
  }
  String html = file.readString();
  file.close();
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  WiFi.softAP(ssid, password);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return;
  }

  server.on("/", handleRoot);
  server.begin();
}

void loop() {
  server.handleClient();
}