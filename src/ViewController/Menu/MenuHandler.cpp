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
}
