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
            display.setContextDosingManualBegin(volume);
            display.setState(DisplayManager::DisplayState::DOSING_MANUAL_START);
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
            display.setContextDosingManualStart(duration);
            display.setState(DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS);
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
        // if (pump.getCurrentPosition() >= totalSteps)
        // {
        //     completeManualDosingController();
        // }
        // else {
        //     float remainingVolume = (totalSteps - pump.getCurrentPosition()) / stepsPerML;
        //     display.setContextDosingManualProgress(volume, remainingVolume, wifi.getCurrent
        //   .getCurrentTime());
    }
    else
    {
        display.setContextDosingManualProgress(volume, volume, "00:00:00");
        display.setState(DisplayManager::DisplayState::DOSING_MANUAL_PROGRESS);
    }
    // float stepsPerML = pump.getDosingStepsPerML();
    // const long totalSteps = targetVolume * stepsPerML;

    // pump.moveML(targetVolume);

    // // Debug logging
    // Serial.printf("Dosing Started: %.2f mL\n", targetVolume);
    // Serial.printf("Steps per mL: %.2f, Total Steps: %ld\n", stepsPerML, totalSteps);

    // display.setContextDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
}

void completeManualDosingController()
{
    DisplayManager &display = DisplayManager::getInstance();
    display.setState(DisplayManager::DisplayState::DOSING_MANUAL_COMPLETE);
}

void handleStopManualDosingController(float targetVolume)
{
    DisplayManager &display = DisplayManager::getInstance();
    WiFiManager &wifi = WiFiManager::getInstance();

    // float stepsPerML = pump.getDosingStepsPerML();
    // const long totalSteps = targetVolume * stepsPerML;

    // pump.moveML(targetVolume);

    // // Debug logging
    // Serial.printf("Dosing Started: %.2f mL\n", targetVolume);
    // Serial.printf("Steps per mL: %.2f, Total Steps: %ld\n", stepsPerML, totalSteps);

    // display.setState(DisplayManager::DisplayState::DOSING_PROGRESS);
    // display.setContextDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
}
void handleCompletedDosing(long totalVolume)
{
    DisplayManager &display = DisplayManager::getInstance();

    display.setState(DisplayManager::DisplayState::DOSING_COMPLETE);
    display.setContextDosingManualComplete(totalVolume);
}

// case DisplayManager::DosingState::RUNNING:
//     // Pause/Resume dosing
//     if (pump.isEnabled())
//     {
//         dosingState = DisplayManager::DosingState::PAUSED;
//         pump.stop();
//     }
//     else
//     {
//         dosingState = DisplayManager::DosingState::RUNNING;
//         pump.setMode(PumpMode::DOSING); // Resume dosing
//     }
//     display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
//     break;

// case DisplayManager::DosingState::COMPLETED:
//     // Reset to idle state
//     dosingState = DisplayManager::DosingState::IDLE;
//     display.updateStatus(false, 0, currentMode, wifi.getCurrentTime());
//     break;
// }
// }

// if (dosingState == DisplayManager::DosingState::SETUP_VOLUME)
// {
//     if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
//     {
//         targetVolume += 1.f;
//         display.showDosingSetup(targetVolume, true);
//     }
//     if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
//     {
//         targetVolume = max(targetVolume - 1.f, 1.f);
//         display.showDosingSetup(targetVolume, true);
//     }
// }
// }
// }