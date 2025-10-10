#include "CalibrateHandler.h"
#include <DisplayManager.h>
#include "CalibrateDosingController/CalibrateDosingController.h"

static DisplayManager &display = DisplayManager::getInstance();

void CalibrateHandler()
{
    if (!isInCalibrate())
        return;

    if (isInCalibrateBegin())
    {
        beginCalibrateDosingController(isInCalibrateBegin());
    }
    else if (isInCalibrateProgress())
    {
        progressCalibrateDosingController(isInCalibrateProgress());
    }
    else if (isInCalibrateComplete())
    {
        completeCalibrateDosingController(isInCalibrateComplete());
    }
}

bool isInCalibrate()
{
    if (isInCalibrateBegin() || isInCalibrateProgress() || isInCalibrateComplete())
        return true;
    return false;
}

bool isInCalibrateBegin()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::CALIBRATE_BEGIN)
        return true;
    return false;
}

bool isInCalibrateProgress()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::CALIBRATE_PROGRESS)
        return true;
    return false;
}

bool isInCalibrateComplete()
{
    if (display.getCurrentState() == DisplayManager::DisplayState::CALIBRATE_COMPLETE)
        return true;
    return false;
}