#include "TimeProvider.h"
#include <Wire.h>
#include <time.h>
#include "TimeUtils.h"

TimeProvider::TimeProvider(TimeProviderType providerType, const NtpConfig& ntpConfig, const RtcConfig& rtcConfig)
  : providerType(providerType), ntpConfig(ntpConfig), rtcConfig(rtcConfig), rtcInitialized(false) {
  BaseComponent::debugLog("[TimeProvider] Constructor called");
  BaseComponent::debugLog("[TimeProvider] Provider type: " + String(providerType == TimeProviderType::RTC ? "RTC" : "NTP"));

  // Debug NTP config
  BaseComponent::debugLog("[TimeProvider] NtpConfig:");
  BaseComponent::debugLog("  enabled: " + String(ntpConfig.enabled));
  BaseComponent::debugLog("  url: " + ntpConfig.url);
  BaseComponent::debugLog("  gmtOffset: " + String(ntpConfig.gmtOffset));
  BaseComponent::debugLog("  dstOffset: " + String(ntpConfig.dstOffset));

  // Debug RTC config
  BaseComponent::debugLog("[TimeProvider] RtcConfig:");
  BaseComponent::debugLog("  sdaPin: " + String(rtcConfig.sdaPin));
  BaseComponent::debugLog("  sclPin: " + String(rtcConfig.sclPin));
  BaseComponent::debugLog("  timeAdjust.enabled: " + String(rtcConfig.timeAdjust.enabled));
  if (rtcConfig.timeAdjust.enabled) {
    BaseComponent::debugLog("  timeAdjust.year: " + String(rtcConfig.timeAdjust.year));
    BaseComponent::debugLog("  timeAdjust.month: " + String(rtcConfig.timeAdjust.month));
    BaseComponent::debugLog("  timeAdjust.day: " + String(rtcConfig.timeAdjust.day));
    BaseComponent::debugLog("  timeAdjust.hour: " + String(rtcConfig.timeAdjust.hour));
    BaseComponent::debugLog("  timeAdjust.minute: " + String(rtcConfig.timeAdjust.minute));
    BaseComponent::debugLog("  timeAdjust.second: " + String(rtcConfig.timeAdjust.second));
  }
}

void TimeProvider::begin() {
  BaseComponent::debugLog("[TimeProvider] begin() called");

  if (providerType == TimeProviderType::RTC) {
    BaseComponent::debugLog("[TimeProvider] Initializing RTC...");
    setupRtc();
  } else if (providerType == TimeProviderType::NTP) {
    BaseComponent::debugLog("[TimeProvider] Initializing NTP...");
    setupNtp();
  } else {
    BaseComponent::debugLog("[TimeProvider] ❌ Unknown provider type");
  }

  time_t currentTime = getCurrentTime();
  String formattedTime = TimeUtils::formatIsoTimestamp(currentTime);
  BaseComponent::debugLog("[TimeProvider] Current time: " + formattedTime);
}

void TimeProvider::setupRtc() {
  BaseComponent::debugLog("[RTC] setupRtc() called");

  Wire.begin(rtcConfig.sdaPin, rtcConfig.sclPin);

  if (!rtc.begin()) {
    BaseComponent::debugLog("[RTC] ❌ RTC not found");
    return;
  }

  if (rtc.lostPower()) {
    BaseComponent::debugLog("[RTC] ⚠️ RTC lost power");

    if (rtcConfig.timeAdjust.enabled) {
      BaseComponent::debugLog("[RTC] ⏱️ Setting RTC time from config");
      rtc.adjust(DateTime(
        rtcConfig.timeAdjust.year,
        rtcConfig.timeAdjust.month,
        rtcConfig.timeAdjust.day,
        rtcConfig.timeAdjust.hour,
        rtcConfig.timeAdjust.minute,
        rtcConfig.timeAdjust.second
      ));
    } else {
      BaseComponent::debugLog("[RTC] ⏱️ Setting RTC time from compile time");
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    DateTime now = rtc.now();
    BaseComponent::debugLog("[RTC] ✅ Time set to: " + String(now.timestamp(DateTime::TIMESTAMP_FULL)));
  } else {
    BaseComponent::debugLog("[RTC] Power status OK");
    DateTime now = rtc.now();
    BaseComponent::debugLog("[RTC] Current RTC time: " + String(now.timestamp(DateTime::TIMESTAMP_FULL)));
  }

  rtcInitialized = true;
  BaseComponent::debugLog("[RTC] ✅ RTC initialized");
}

void TimeProvider::setupNtp() {
  BaseComponent::debugLog("[NTP] setupNtp() called");

  if (!ntpConfig.enabled) {
    BaseComponent::debugLog("[NTP] ❌ NTP is disabled in config");
    return;
  }

  setenv("TZ", "CST6CDT,M3.2.0,M11.1.0", 1);
  tzset();

  configTime(ntpConfig.gmtOffset, ntpConfig.dstOffset, ntpConfig.url.c_str());
  BaseComponent::debugLog("[NTP] ✅ NTP initialized");

  BaseComponent::debugLog("[NTP] Waiting for time sync");
  int attempts = 0;
  time_t now;
  String dots = "";
  do {
    now = time(nullptr);
    delay(500);
    dots += ".";
    attempts++;
  } while (now < 1000000000 && attempts < 40);

  BaseComponent::debugLog("[NTP] Sync attempt result: " + dots);

  if (now >= 1000000000) {
    BaseComponent::debugLog("[NTP] ✅ Time synced: " + String(now));
  } else {
    BaseComponent::debugLog("[NTP] ❌ Time sync failed after timeout");
  }
}

time_t TimeProvider::getCurrentTime() {
  BaseComponent::debugLog("[TimeProvider] getCurrentTime() called");

  if (providerType == TimeProviderType::RTC && rtcInitialized) {
    time_t rtcTime = rtc.now().unixtime();
    BaseComponent::debugLog("[TimeProvider] RTC time: " + String(rtcTime));
    return rtcTime;
  }

  time_t now = time(nullptr);
  BaseComponent::debugLog("[TimeProvider] NTP time: " + String(now));

  if (providerType == TimeProviderType::NTP && now < 1000000000) {
    BaseComponent::debugLog("[TimeProvider] ⚠️ NTP time not yet synced");

    if (rtcInitialized) {
      time_t rtcTime = rtc.now().unixtime();
      BaseComponent::debugLog("[TimeProvider] ⚠️ Falling back to RTC time: " + String(rtcTime));
      return rtcTime;
    }
  }

  return now;
}