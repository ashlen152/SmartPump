#include "ManualDosingController.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>

void setupManualDosingController()
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