#include "ManualDosingController.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <PumpController.h>

void setupManualDosingController()
{
    DisplayManager &display = DisplayManager::getInstance();
    float initialVolume = 10; // Initial volume for setup
    display.setState(DisplayManager::DisplayState::DOSING_SETUP);
    display.setContextDosingManualSetup(initialVolume);
}
void handleRunningManualDosingController()
{
    DisplayManager &display = DisplayManager::getInstance();

    display.setState(DisplayManager::DisplayState::DOSING_BEGIN);
    display.setContextDosingManualBegin(1); // Example duration

    // float stepsPerML = pump.getDosingStepsPerML();
    // const long totalSteps = targetVolume * stepsPerML;

    // pump.moveML(targetVolume);

    // // Debug logging
    // Serial.printf("Dosing Started: %.2f mL\n", targetVolume);
    // Serial.printf("Steps per mL: %.2f, Total Steps: %ld\n", stepsPerML, totalSteps);

    // display.setContextDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
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