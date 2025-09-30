#include <DisplayManager.h>
#include <Config.h>
#include "ButtonHandler.h"
#include "Calibration.h"
#include "MenuHandler.h"
#include <EEPROM.h>
#include "PumpController.h"
#include <AutoDosingManager.h>

extern int menuIndex;
extern DisplayManager::PumpMode currentMode;
extern DisplayManager &display;
extern PumpController pump;
extern AutoDosingManager autoDosing;
extern bool showingSettings;
extern unsigned long lastSettingsDisplayTime;
extern unsigned long lastCalibrationResultTime;
extern bool showingCalibrationResult;

void runMenuSelection() {
  switch (menuIndex) {
  case 0: // Dosing Calibration
    calibrateDosing();
    break;
  case 1: // Settings Info
    display.showSettingsInfo(pump.getSpeed(),
                             pump.getDosingStepsPerML(),
                             pump.getSpeedStep());
    showingSettings = true;
    lastSettingsDisplayTime = millis();
    break;
  case 2: // Save Speed
    {
      float currentSpeed = pump.getSpeed();
      EEPROM.put(EEPROM_SAVED_SPEED_ADDR, currentSpeed);
      EEPROM.commit();
      display.showText("Speed Saved!");
      delay(1000);
    }
    break;
  case 3: // Change mode
    {
      const char *modeItems[] = {"Dosing"};
      const int modeCount = sizeof(modeItems) / sizeof(modeItems[0]);
      int modeIndex = static_cast<int>(currentMode);
      bool modeChanged = false;
      display.showMenu(modeIndex, modeItems, modeCount);
      unsigned long modeSelectionStart = millis();
      while (millis() - modeSelectionStart < 10000) {
        if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN)) {
          modeIndex = (modeIndex + 1) % modeCount;
          display.showMenu(modeIndex, modeItems, modeCount);
        }
        if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN)) {
          modeIndex = (modeIndex - 1 + modeCount) % modeCount;
          display.showMenu(modeIndex, modeItems, modeCount);
        }
        if (checkButtonPress(BUTTON_ENABLE_PIN)) {
          currentMode = static_cast<DisplayManager::PumpMode>(modeIndex);
          uint8_t modeToSave = static_cast<uint8_t>(currentMode);
          EEPROM.put(EEPROM_MODE_ADDR, modeToSave);
          EEPROM.commit();
          display.showText("Mode Changed!");
          delay(1000);
          modeChanged = true;
          break;
        }
        if (checkButtonPress(BUTTON_MENU_PIN)) {
          break;
        }
        pump.runDosing();
      }
      if (!modeChanged) {
        display.showText("Selection Cancelled");
        delay(1000);
      }
    }
    break;
  case 4: // Auto Dosing
    if (autoDosing.isEnabled()) {
      autoDosing.disable();
      display.showText("Auto Dosing Off");
    } else {
      autoDosing.enable();
      display.showText("Auto Dosing On");
    }
    delay(1000);
    break;
  case 5: // Set Daily Volume
    {
      float volume = 10.0f;
      bool setting = true;
      while (setting) {
        display.showValue("Daily Volume (mL)", volume);
        if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
          volume += 1.0f;
        if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
          volume = max(volume - 1.0f, 1.0f);
        if (checkButtonPress(BUTTON_ENABLE_PIN)) {
          autoDosing.setDailyVolume(volume);
          setting = false;
        }
        if (checkButtonPress(BUTTON_MENU_PIN)) {
          setting = false;
        }
      }
    }
    break;
  }
}
