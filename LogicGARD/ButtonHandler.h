#pragma once
#include <Arduino.h>

// ─── Public API ─────────────────────────────────
void initBootButton(uint8_t pin);
void monitorBootButton();
void setShortPressCallback(void (*callback)());
void setLongPressCallback(void (*callback)());
bool isRestartPending();
void clearRestartFlag();