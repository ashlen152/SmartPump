#include "HomeHandler.h"
#include <DisplayManager.h>
#include "ButtonController/ButtonController.h"
#include "ViewController/Menu/MenuHandler.h"
#include "ViewController/Manual/ManualHandler.h"

static DisplayManager &display = DisplayManager::getInstance();

void HomeHandler()
{
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
  if (display.getCurrentState() == DisplayManager::DisplayState::NORMAL)
    return true;
  return false;
}