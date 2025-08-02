# Tommy Guard ESP32 code

# Networks
- Hotspot for configuration
  - When 'prog' button in depressed on boot
- Connection to MQTT logging server
  - store data and push to tgdata.earlydetectionsystems.com once per hour
- Connection to NTP time server
  - On boot and once a day get time as http request from time.earlydetectionsystems.com
- Connection NVR video server
  - LAN push to NVR(in appsettings.json file) after every data read
- Connection to IP camera
  - LAN push to camera(in appsettings.json file) after every data read
- OTA
  - Code updates from ota.earlydetectionsystems.com
  
