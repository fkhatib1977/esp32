#pragma once

#include <Adafruit_GC9A01A.h>
#include <IPAddress.h>
#include <vector>

class IpDisplay {
public:
  IpDisplay(Adafruit_GC9A01A& tft, int maxIps = 6, int textScale = 3, int orientation = 0);

  void begin(int ipCount = -1);
  void setOrientation(int orientation);
  void setIp(const String& ipStr, int colorFlag);

private:
  Adafruit_GC9A01A& _tft;
  int _maxIps;
  int _textScale;
  float _scaledTextScale;
  int _orientation;
  int _activeCount;

  struct Entry {
    IPAddress ip;
    int colorFlag;
  };

  std::vector<Entry> _entries;

  bool parseIp(const String& str, IPAddress& ip);
  String formatIp(const IPAddress& ip);
  std::vector<int> getRowOrder(int count);
  void drawAll();
};