#include "ManualHandler.h"
#include "ViewController/Manual/ManualHandler.h"
#include <DisplayManager.h>
#include "ButtonController/ButtonController.h"
#include "ManualDosingController/ManualDosingController.h"

void ManualHandler()
{
    if (!isInManual())
    {
        beginManualDosingController(isInManualBegin());
    }
    else if (isInManualBegin())
    {
        startManualDosingController(isInManualStart());
    }
    else if (isInManualStart())
    {
        progressManualDosingController(isInManualProgress());
    }
    else if (isInManualProgress())
    {
        completeManualDosingController(isInManualComplete());
    }
    else if (isInManualComplete())
    {
    }
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