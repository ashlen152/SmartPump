#include "MenuHandler.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>

int menuIndex = 0;
bool showingSettings = false;
const char *menuItems[] = {"Dosing Cal", "Settings Info", "Auto Dosing", "Set Daily Vol"};

void runMenuSelection()
{
  DisplayManager &display = DisplayManager::getInstance();
  switch (menuIndex)
  {
  case 0: // Dosing Calibration
    // calibrateDosing();
    display.setState(DisplayManager::DisplayState::CALIBRATION_START);
    break;
  case 1:                                   // Settings Info
                                            // display.showSettingsInfo(pump.getSpeed(),
                                            //                          pump.getDosingStepsPerML(),
                                            //                          pump.getSpeedStep());
    display.setContextSettings(0, 0.0f, 0); // Replace with real values
    display.setState(DisplayManager::DisplayState::SETTINGS);
    break;
  case 2: // Auto Dosing
          //   if (autoDosing.isEnabled())
          //   {
          //     autoDosing.disable();
          //     display.showText("Auto Dosing Off");
          //   }
          //   else
          //   {
          //     autoDosing.enable();
          //     display.showText("Auto Dosing On");
          //   }
          //   delay(1000);
    display.setState(DisplayManager::DisplayState::NORMAL);
    break;
  case 3: // Set Daily Volume
    display.setState(DisplayManager::DisplayState::DOSING_SETUP);
    // {
    //   float volume = 10.0f;
    //   bool setting = true;
    //   while (setting)
    //   {
    //     display.showValue("Daily Volume (mL)", volume);
    //     if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
    //       volume += 1.0f;
    //     if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
    //       volume = max(volume - 1.0f, 1.0f);
    //     if (checkButtonPress(BUTTON_ENABLE_PIN))
    //     {
    //       autoDosing.setDailyVolume(volume);
    //       setting = false;
    //     }
    //     if (checkButtonPress(BUTTON_MENU_PIN))
    //     {
    //       setting = false;
    //     }
    //   }
    // }
    break;
  }
}

const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

bool isInMenu()
{
  DisplayManager &display = DisplayManager::getInstance();
  if (display.getCurrentState() == DisplayManager::DisplayState::MENU)
    return true;
  return false;
}

void MenuHandler()
{
  DisplayManager &display = DisplayManager::getInstance();
  // Button/menu handling
  if (pressButtonMenu())
  {
    if (isInMenu())
    {
      printf("Menu selection: %d\n", menuIndex);
      runMenuSelection();
    }
    else
    {
      printf("Entering menu\n");
      menuIndex = 0;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
      display.setState(DisplayManager::DisplayState::MENU);
    }
  }
  if (isInMenu())
  {
    if (holdButtonUp())
    {
      printf("Menu up\n");
      menuIndex = (menuIndex + 1) % menuItemCount;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
    }
    if (holdButtonDown())
    {
      printf("Menu down\n");
      menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
    }
  }
  else
  {
    // Handle DOSING mode buttons
    if (checkButtonPress(BUTTON_ENABLE_PIN))
    {
      switch (dosingState)
      {
      case DisplayManager::DosingState::IDLE:
        dosingState = DisplayManager::DosingState::SETUP_VOLUME;
        targetVolume = 1.0; // Start with 1mL
        display.showDosingSetup(targetVolume, true);
        break;

      case DisplayManager::DosingState::SETUP_VOLUME:
      {
        // Start dosing with exact step calculation
        dosingState = DisplayManager::DosingState::RUNNING;
        remainingVolume = targetVolume;

        // Configure for precise dosing
        pump.setMode(PumpMode::DOSING);

        // Get current calibrated steps per mL
        float stepsPerML = pump.getDosingStepsPerML();
        const long totalSteps = targetVolume * stepsPerML;

        // Move using calibrated values
        pump.moveML(targetVolume);

        // Debug logging
        Serial.printf("Dosing Started: %.2f mL\n", targetVolume);
        Serial.printf("Steps per mL: %.2f, Total Steps: %ld\n", stepsPerML, totalSteps);

        display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
      }
      break;

      case DisplayManager::DosingState::RUNNING:
        // Pause/Resume dosing
        if (pump.isEnabled())
        {
          dosingState = DisplayManager::DosingState::PAUSED;
          pump.stop();
        }
        else
        {
          dosingState = DisplayManager::DosingState::RUNNING;
          pump.setMode(PumpMode::DOSING); // Resume dosing
        }
        display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
        break;

      case DisplayManager::DosingState::COMPLETED:
        // Reset to idle state
        dosingState = DisplayManager::DosingState::IDLE;
        display.updateStatus(false, 0, currentMode, wifi.getCurrentTime());
        break;
      }
    }

    if (dosingState == DisplayManager::DosingState::SETUP_VOLUME)
    {
      if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      {
        targetVolume += 1.f;
        display.showDosingSetup(targetVolume, true);
      }
      if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      {
        targetVolume = max(targetVolume - 1.f, 1.f);
        display.showDosingSetup(targetVolume, true);
      }
    }
  }
}
