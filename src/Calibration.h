#pragma once
extern DisplayManager &display;
extern PumpController pump;
extern unsigned long lastCalibrationResultTime;
extern bool showingCalibrationResult;
extern float CALIBRATE_DOSING_VOLUME;
bool checkButtonPress(uint8_t pin);
bool checkButtonPressOrHold(uint8_t pin);
void calibrateDosing();
