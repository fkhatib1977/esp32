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
  - LAN push to NVR(in user.json file) after every data read
- Connection to IP camera
  - LAN push to camera(in user.json file) after every data read
- OTA
  - Code updates from http://ota.earlydetectionsystems.com

<br><br>

# user.json file terms definitions
- identification
  - clientId - Client name or ID  *(Burger King, Subway, Food Holdings LLC, Client #2, 45, 897)*
  - locationId - Location of monitored facility *(Woodstock IL, store #23, 3465, 87344)*
  - unitId - ID of monitored equipment *(Freezer #2, cooler#4, unit 3, 2, 4)*

- lan
  - isStatic - is the LAN IP address static or DHCP *(true, false)*
  - ip - static IP address *(10.10.2.15)*
  - subnet
  - gateway
  - dns1
  - dns2

- frequencyInSeconds
  - local
  - public - number of minutes between data pushes to MQTT server *(1, 5, 60, 240, etc)*

- urls
  - mqtt - url for MQTT server *(tgdata.earlydetectionsystems.com)*
  - ntp - url for NPT server *(time.earlydetectionsystems.com)*
  - ota - url for OTA server *(ota.earlydetectionsystems.com)*

- camera
  - ip
  - username
  - password
  - camera - for multi camera enclosures *(1, 2, 3, 4)*
  - position - where to display text *(topLeft, topRight, bottomLeft, bottomRight)*
  - fontSize - size of text *(6, 8, 14, 26, etc)*
  - textColor - color of text *(red, blue, green, etc)*
  - text - Text to display with token for tempature - *( Temp in meat cooler is (temp)°F degrees. )*

- nvr
  - ip
  - username
  - password
  - port



