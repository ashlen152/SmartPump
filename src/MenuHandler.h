#pragma once
extern int menuIndex;
extern DisplayManager::PumpMode currentMode;
extern DisplayManager &display;
extern PumpController pump;
extern AutoDosingManager autoDosing;
extern bool showingSettings;
extern unsigned long lastSettingsDisplayTime;
extern unsigned long lastCalibrationResultTime;
extern bool showingCalibrationResult;
void runMenuSelection();
