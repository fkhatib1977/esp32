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
identification
```
clientId
locationId
unitId
```

lan
```
isStatic
ip
subnet
gateway
dns1
dns2
```

frequencyInSeconds
```
local
public
```

urls
```
mqtt
ntp
ota
```

camera
```
ip
username
password
camera
position
fontSize
textColor
text
```

nvr
```
ip
username
password
port
```


