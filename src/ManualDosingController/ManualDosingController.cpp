#include "ManualDosingController.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <PumpController.h>

float volume = 10.0; // Default volume for manual dosing
int duration = 1;

void beginManualDosingController(bool isInManualBegin)
{
    DisplayManager &display = DisplayManager::getInstance();
    if (isInManualBegin)
    {
        if (pressButtonDown())
        {
            volume -= 0.1;
            display.setContextDosingManualBegin(volume);
        }
        if (pressButtonUp())
        {
            volume += 0.1;
            display.setContextDosingManualBegin(volume);
        }
        if (pressButtonEnable())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
        if (pressButtonMenu())
        {
            startManualDosingController(false);
        }
    }
    else
    {
        volume = 10; // Initial volume for setup
        display.setContextDosingManualBegin(volume);
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_BEGIN);
    }
}

void startManualDosingController(bool isInManualStart)
{
    DisplayManager &display = DisplayManager::getInstance();
    if (isInManualStart)
    {
        if (pressButtonDown())
        {
            duration = max(duration - 1.0, 1.0);
            display.setContextDosingManualStart(duration);
        }
        if (pressButtonUp())
        {
            duration += 1.0;
            display.setContextDosingManualStart(duration);
        }
        if (pressButtonEnable())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
        if (pressButtonMenu())
        {
            progressManualDosingController(false);
        }
    }
    else
    {
        duration = 1;
        display.setContextDosingManualStart(duration);
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_START);
    }
}

void progressManualDosingController(bool isInManualProgress)
{
    DisplayManager &display = DisplayManager::getInstance();
    PumpController &pump = PumpController::getInstance();
    WiFiManager &wifi = WiFiManager::getInstance();

    if (isInManualProgress)
    {
        if (pressButtonEnable())
        {
            // TODO: implement stop dosing
            // cancel dosing
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
        // TODO:
        // check elapsed steps of motor > total steps
        // if yes, then complete dosing
        // else recalculate remaining volume and time
        // update display
        float stepsPerML = pump.getDosingStepsPerML();
        const long totalSteps = volume * stepsPerML;
        if (pump.getCurrentPosition() >= totalSteps)
        {
            completeManualDosingController(false);
        }
        else
        {
            float remainingVolume = (totalSteps - pump.getCurrentPosition()) / stepsPerML;
            display.setContextDosingManualProgress(volume, remainingVolume, wifi.getCurrentTime());
        }
    }
    else
    {

        float stepsPerML = pump.getDosingStepsPerML();
        const long targetSteps = volume * stepsPerML;

        pump.moveML(volume);

        display.setContextDosingManualProgress(volume, volume, "00:00:00");
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS);
    }

    // display.setContextDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
}

void completeManualDosingController(bool isInManualComplete)
{
    DisplayManager &display = DisplayManager::getInstance();
    display.setState(DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE);
}