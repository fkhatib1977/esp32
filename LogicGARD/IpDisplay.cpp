#include "IpDisplay.h"

IpDisplay::IpDisplay(Adafruit_GC9A01A& tft, int maxIps, int textScale, int orientation)
  : _tft(tft),
    _maxIps(constrain(maxIps, 1, 10)),
    _textScale(constrain(textScale, 1, 6)),
    _scaledTextScale(_textScale * 0.97f),
    _orientation(constrain(orientation, 0, 3)),
    _activeCount(maxIps) {}

void IpDisplay::begin(int ipCount) {
  _activeCount = (ipCount > 0 && ipCount <= _maxIps) ? ipCount : _maxIps;
  _entries.clear();
  _tft.fillScreen(GC9A01A_BLACK);
  _tft.setRotation(_orientation);
  _tft.setTextWrap(false);
  _tft.setTextSize(_scaledTextScale);
  drawAll();
}

void IpDisplay::setOrientation(int orientation) {
  _orientation = constrain(orientation, 0, 3);
  _tft.setRotation(_orientation);
  drawAll();
}

void IpDisplay::setIp(const String& ipStr, int colorFlag) {
  IPAddress ip;
  if (!parseIp(ipStr, ip)) return;
  colorFlag = constrain(colorFlag, -1, 1);

  for (auto& entry : _entries) {
    if (entry.ip == ip) {
      entry.colorFlag = colorFlag;
      drawAll();
      return;
    }
  }

  if (_entries.size() < _activeCount) {
    _entries.push_back({ip, colorFlag});
  } else {
    _entries[_activeCount - 1] = {ip, colorFlag};
  }

  drawAll();
}

bool IpDisplay::parseIp(const String& str, IPAddress& ip) {
  int parts[4];
  int last = 0, next = 0;
  for (int i = 0; i < 4; ++i) {
    next = str.indexOf('.', last);
    if (next == -1 && i < 3) return false;
    String part = (i < 3) ? str.substring(last, next) : str.substring(last);
    parts[i] = part.toInt();
    if (parts[i] < 0 || parts[i] > 255) return false;
    last = next + 1;
  }
  ip = IPAddress(parts[0], parts[1], parts[2], parts[3]);
  return true;
}

String IpDisplay::formatIp(const IPAddress& ip) {
  return String(ip[0]) + "." + String(ip[1]) + "." + String(ip[2]) + "." + String(ip[3]);
}

std::vector<int> IpDisplay::getRowOrder(int count) {
  std::vector<int> order;
  int mid = count / 2;
  order.push_back(mid);
  for (int offset = 1; order.size() < count; ++offset) {
    if (mid - offset >= 0) order.push_back(mid - offset);
    if (mid + offset < count) order.push_back(mid + offset);
  }
  return order;
}

void IpDisplay::drawAll() {
  const int screenWidth = 240;
  const int screenHeight = 240;
  float compressionFactor = 0.50;
  int verticalShift = screenHeight * 0.20;
  int horizontalShift = screenWidth * 0.10;
  int lineHeight = screenHeight / (_activeCount / compressionFactor);

  std::vector<Entry> sorted = _entries;
  std::sort(sorted.begin(), sorted.end(), [&](const Entry& a, const Entry& b) {
    return formatIp(a.ip).length() > formatIp(b.ip).length();
  });

  std::vector<int> rowOrder = getRowOrder(sorted.size());

  for (int i = 0; i < sorted.size(); ++i) {
    const Entry& entry = sorted[i];
    int row = rowOrder[i];
    String ipStr = formatIp(entry.ip);

    int charWidth = 6 * _scaledTextScale;
    int textWidth = ipStr.length() * charWidth;
    int effectiveWidth = screenWidth * 0.90;
    int x = max((effectiveWidth - textWidth) / 2, 0) + horizontalShift;
    int y = verticalShift + row * lineHeight + (lineHeight - (8 * _scaledTextScale)) / 2;

    _tft.fillRect(0, verticalShift + row * lineHeight, screenWidth, lineHeight, GC9A01A_BLACK);

    uint16_t color = GC9A01A_WHITE;
    if (entry.colorFlag == -1) color = GC9A01A_RED;
    else if (entry.colorFlag == 1) color = GC9A01A_GREEN;

    _tft.setCursor(x, y);
    _tft.setTextColor(color);
    _tft.print(ipStr);
  }
}