#pragma once
#include <DisplayManager.h>
#include <PumpController.h>
#include <Config.h>
#include "ButtonHandler.h"

extern DisplayManager &display;
extern PumpController pump;
extern unsigned long lastCalibrationResultTime;
extern bool showingCalibrationResult;
bool checkButtonPress(uint8_t pin);
bool checkButtonPressOrHold(uint8_t pin);
void calibrateDosing();
