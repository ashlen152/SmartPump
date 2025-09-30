#include <DisplayManager.h>
#include "ButtonHandler.h"
#include <Arduino.h>
#include <Config.h>
#include "PumpController.h"

// Forward declaration or include the correct header for DisplayManager
extern DisplayManager::PumpMode currentMode;
extern DisplayManager &display;
extern PumpController pump;
extern unsigned long lastButtonPressTime;

bool checkButtonPress(uint8_t pin) {
  if (digitalRead(pin) == LOW) {
    lastButtonPressTime = millis();
    if (display.isSleeping()) {
      display.wakeDisplay();
      while (digitalRead(pin) == LOW) {
        pump.runDosing();
      }
      return false;
    }
    while (digitalRead(pin) == LOW) {
      pump.runDosing();
    }
    return true;
  }
  return false;
}

bool checkButtonPressOrHold(uint8_t pin) {
  static unsigned long holdStartTime[4] = {0};
  static unsigned long lastActionTime[4] = {0};
  int index = (pin == BUTTON_SPEED_UP_PIN) ? 0 : (pin == BUTTON_SPEED_DOWN_PIN) ? 1
                                             : (pin == BUTTON_MENU_PIN)         ? 2
                                                                                : 3;
  if (digitalRead(pin) == LOW) {
    if (display.isSleeping()) {
      lastButtonPressTime = millis();
      display.wakeDisplay();
      while (digitalRead(pin) == LOW)
        pump.runDosing();
      return false;
    }
    if (holdStartTime[index] == 0) {
      holdStartTime[index] = millis();
      lastActionTime[index] = millis();
      return true;
    } else {
      unsigned long totalHoldTime = millis() - holdStartTime[index];
      unsigned long interval = (totalHoldTime > 2000) ? 100 : 500;
      if (millis() - lastActionTime[index] >= interval) {
        lastActionTime[index] = millis();
        return true;
      }
    }
  } else {
    holdStartTime[index] = 0;
    lastActionTime[index] = 0;
  }
  return false;
}
