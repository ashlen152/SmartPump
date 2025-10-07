#include "HomeHandler.h"
#include <DisplayManager.h>
#include "ButtonController/ButtonController.h"
#include "ViewController/Menu/MenuHandler.h"
#include "ViewController/Manual/ManualHandler.h"

void HomeHandler()
{
  DisplayManager &display = DisplayManager::getInstance();
  if (!isInHome())
    return;
  // if any button was pressed in home, switch case to handle it
  if (pressButtonMenu())
  {
    MenuHandler();
  }
  else if (pressButtonEnable())
  {
    ManualHandler();
  }
}

bool isInHome()
{
  DisplayManager &display = DisplayManager::getInstance();
  if (display.getCurrentState() == DisplayManager::DisplayState::NORMAL)
    return true;
  return false;
}