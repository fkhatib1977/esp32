#pragma once

#include <Arduino.h>
#include <ctime>
#include <cstdio>

class TimeUtils {
public:
  //––––––––––––––––––––––––––––––––––––––––––––
  // Existing String-based APIs (unchanged)
  //––––––––––––––––––––––––––––––––––––––––––––

  // Returns current UTC time in ISO 8601 format
  static String getIsoTimestamp() {
    return formatIsoTimestamp(time(nullptr));
  }

  // Returns ISO 8601 timestamp for a given time_t value
  static String formatIsoTimestamp(time_t rawTime) {
    if (rawTime == 0) {
      return String("1970-01-01T00:00:00Z");
    }
    char buffer[32];
    struct tm* utc = gmtime(&rawTime);
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", utc);
    return String(buffer);
  }

  // Returns raw epoch time in seconds
  static uint32_t getEpochSeconds() {
    return static_cast<uint32_t>(time(nullptr));
  }

  // Returns timestamp formatted using a user-defined strftime pattern
  static String formatCustomTimestamp(time_t rawTime, const String& formatStr) {
    if (rawTime == 0 || formatStr.isEmpty()) {
      return formatIsoTimestamp(0);
    }
    char buffer[64];
    struct tm timeinfo;
    gmtime_r(&rawTime, &timeinfo);
    size_t written = strftime(buffer, sizeof(buffer), formatStr.c_str(), &timeinfo);
    if (written == 0) {
      return formatIsoTimestamp(rawTime);
    }
    return String(buffer);
  }

  //––––––––––––––––––––––––––––––––––––––––––––
  // New buffer-based overloads (static inline)
  //––––––––––––––––––––––––––––––––––––––––––––

  // ISO 8601 into your own buffer: "YYYY-MM-DDTHH:MM:SSZ"
  static inline bool formatIsoTimestamp(time_t ts,
                                        char* buf,
                                        size_t bufSize) {
    struct tm tm{};
    gmtime_r(&ts, &tm);
    int len = std::snprintf(
      buf, bufSize,
      "%04d-%02d-%02dT%02d:%02d:%02dZ",
      tm.tm_year + 1900,
      tm.tm_mon  + 1,
      tm.tm_mday,
      tm.tm_hour,
      tm.tm_min,
      tm.tm_sec
    );
    return (len > 0 && static_cast<size_t>(len) < bufSize);
  }

  // Custom fmt into your buffer, using strftime patterns
  static inline bool formatCustomTimestamp(time_t ts,
                                           String fmt,
                                           char* buf,
                                           size_t bufSize) {
    struct tm tm{};
    gmtime_r(&ts, &tm);
    size_t written = std::strftime(buf, bufSize, fmt.c_str(), &tm);
    return (written > 0 && written < bufSize);
  }
};