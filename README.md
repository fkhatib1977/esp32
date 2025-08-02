# Tommy Guard ESP32 code

# Networks
- Hotspot for configuration
  - When 'prog' button in depressed on boot
- Connection to MQTT logging server
  - store data and push to http://tgdata.earlydetectionsystems.com once per hour
  - Send client ID, location ID, unit ID, temp1, temp2, temp, humidity, pressure
- Connection to NTP time server
  - On boot and once a day get time as http request from http://time.earlydetectionsystems.com
- Connection NVR video server
  - LAN push to NVR(in appsettings.json file) after every data read
- Connection to IP camera
  - LAN push to camera(in appsettings.json file) after every data read
- OTA
  - Code updates from http://ota.earlydetectionsystems.com
  
# appsettings.json file structure
Identification
```
ClientID
LocationID
UnitID
```

LAN
```
isStatic
IP
SubnetMask
DNS1
DNS2
Gateway
```

Camera Info
```
Cam IP
Cam Number
Cam Username
Cam Password
```

NVR Info
```
NVR IP
NVR Username
NVR Password
```


