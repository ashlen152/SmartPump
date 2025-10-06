#include "ViewController/Manual/ManualHandler.h"
#include <DisplayManager.h>
#include "ButtonController/ButtonController.h"
#include "ManualHandler.h"

void ManualHandler()
{
}

bool isInManual()
{
    DisplayManager &display = DisplayManager::getInstance();
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_BEGIN || display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_START || display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS || display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE)
        return true;
    return false;
}

bool isInManualBegin()
{
    DisplayManager &display = DisplayManager::getInstance();
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_BEGIN)
        return true;
    return false;
}

bool isInManualProgress()
{
    DisplayManager &display = DisplayManager::getInstance();
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS)
        return true;
    return false;
}

bool isInManualStart()
{
    DisplayManager &display = DisplayManager::getInstance();
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_START)
        return true;
    return false;
}

bool isInManualComplete()
{
    DisplayManager &display = DisplayManager::getInstance();
    if (display.getCurrentState() == DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE)
        return true;
    return false;
}