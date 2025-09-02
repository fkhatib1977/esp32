#include "ButtonHandler.h"

static uint8_t buttonPin = 0;
static bool buttonPressed = false;
static bool pendingRestart = false;
static unsigned long pressStart = 0;

static void (*onShortPress)() = nullptr;
static void (*onLongPress)() = nullptr;

void initBootButton(uint8_t pin) {
  buttonPin = pin;
  pinMode(buttonPin, INPUT_PULLUP);
}

void setShortPressCallback(void (*callback)()) {
  onShortPress = callback;
}

void setLongPressCallback(void (*callback)()) {
  onLongPress = callback;
}

void monitorBootButton() {
  if (digitalRead(buttonPin) == LOW && !buttonPressed) {
    pressStart = millis();
    buttonPressed = true;
  }

  if (digitalRead(buttonPin) == HIGH && buttonPressed) {
    unsigned long pressDuration = millis() - pressStart;
    buttonPressed = false;

    if (pressDuration >= 10000) {
      if (onLongPress) onLongPress();
    } else {
      if (onShortPress) onShortPress();
    }
  }
}
