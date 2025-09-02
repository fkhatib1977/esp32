#pragma once
#include <Arduino.h>
#include <RTClib.h>
#include "Types.h"
#include "BaseComponent.h"

class TimeProvider : public BaseComponent {
public:
  TimeProvider(TimeProviderType providerType, const NtpConfig& ntpConfig, const RtcConfig& rtcConfig);
  void begin();
  time_t getCurrentTime();

private:
  void setupRtc();
  void setupNtp();

  TimeProviderType providerType;
  NtpConfig ntpConfig;
  RTC_DS3231 rtc;
  RtcConfig rtcConfig;
  bool rtcInitialized = false;
};