#pragma once
#include <stdint.h>

extern unsigned long lastButtonPressTime;
extern DisplayManager &display;
extern PumpController pump;
extern DisplayManager::PumpMode currentMode;

bool checkButtonPress(uint8_t pin);
bool checkButtonPressOrHold(uint8_t pin);
