/**
 * @file CalibrateDosingController.cpp
 * @brief Calibration flow implementation for determining steps-per-mL ratio.
 *
 * Calibration process:
 *   1. Begin: Show prompt, wait for Enable button to start
 *   2. Progress: Run motor for DOSING_CAL_STEPS (200,000 steps) at 2000 steps/sec
 *      - Shows progress on display with step count
 *      - Detects completion, stall (no movement for 2s), or timeout (2 min)
 *      - Enable button aborts calibration
 *   3. Complete: User measures actual mL dispensed, adjusts with Up/Down (+/- 0.1mL)
 *      - Enable button confirms: calculates newStepsPerML = 200000 / measuredML
 *      - Saves new calibration to EEPROM at EEPROM_DOSING_STEPS_ADDR
 *      - Menu button cancels without saving
 *
 * @note Functions are defined here but NOT declared in the header file.
 *       See AGENTS.md Known Issues #8.
 */

#include "CalibrateDosingController.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <PumpController.h>
#include <EEPROM.h>
#include <Config.h>

// Constants
const long DOSING_CAL_STEPS = 200000;
const float DOSING_CAL_SPEED = 2000.0f;
const unsigned long TIMEOUT = 120000;         // 2 minutes timeout
const float CALIBRATE_DOSING_VOLUME = 100.0f; // Example default

// Internal variables
static unsigned long startTime = 0;
static unsigned long displayUpdate = 0;
static unsigned long lastMoveTime = 0;
static long lastPosition = 0;
static float ml = CALIBRATE_DOSING_VOLUME;
static bool showingCalibrationResult = false;
static unsigned long lastCalibrationResultTime = 0;

static DisplayManager &display = DisplayManager::getInstance();
static PumpController &pump = PumpController::getInstance();
static WiFiManager &wifi = WiFiManager::getInstance();

// --- BEGIN STATE ---
void beginCalibrateDosingController(bool isInBegin)
{
    if (isInBegin)
    {
        // User confirmation before calibration
        if (pressButtonEnable())
        {
            progressCalibrateDosingController(false);
        }
        if (pressButtonMenu())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
    }
    else
    {
        // Prepare system
        pump.stop();
        pump.setCurrentPosition(0);
        pump.setMode(PumpMode::DOSING);
        pump.setSpeed(DOSING_CAL_SPEED);

        display.showText("Ready to calibrate.\nPress Enable to start.");
        display.setState(DisplayManager::DisplayState::CALIBRATE_BEGIN);
    }
}

// --- PROGRESS STATE ---
void progressCalibrateDosingController(bool isInProgress)
{
    if (isInProgress)
    {
        pump.runDosing();

        long currentPosition = pump.getCurrentPosition();

        // Movement check
        if (currentPosition != lastPosition)
        {
            lastPosition = currentPosition;
            lastMoveTime = millis();
        }

        // Periodic display update
        if (millis() - displayUpdate >= 1000)
        {
            char progressText[64];
            snprintf(progressText, sizeof(progressText), "Calibrating...\n%ld / %ld steps\n%d%%", 
                     currentPosition, DOSING_CAL_STEPS, (int)((currentPosition * 100) / DOSING_CAL_STEPS));
            display.showText(progressText);
            displayUpdate = millis();
        }

        // Completion or timeout
        if (currentPosition >= DOSING_CAL_STEPS ||
            (!pump.isRunning() && millis() - lastMoveTime > 2000) ||
            (millis() - startTime > TIMEOUT))
        {
            completeCalibrateDosingController(false);
        }

        // Abort
        if (pressButtonEnable())
        {
            pump.stop();
            display.showText("Calibration aborted.");
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
    }
    else
    {
        // Start calibration movement
        pump.moveRelative(DOSING_CAL_STEPS);
        startTime = millis();
        displayUpdate = millis();
        lastMoveTime = millis();
        lastPosition = 0;

        display.showText("Starting calibration...");
        display.setState(DisplayManager::DisplayState::CALIBRATE_PROGRESS);
    }
}

// --- COMPLETE STATE ---
void completeCalibrateDosingController(bool isInComplete)
{
    if (isInComplete)
    {
        // Adjust measured mL
        if (pressButtonUp())
        {
            ml += 0.1f;
            display.showCalibrationInput(ml);
        }
        if (pressButtonDown())
        {
            ml = max(ml - 0.1f, 0.0f);
            display.showCalibrationInput(ml);
        }

        // Confirm calibration
        if (pressButtonEnable())
        {
            float newStepsPerML = (ml > 0) ? (DOSING_CAL_STEPS / ml) : pump.getDosingStepsPerML();
            pump.setDosingStepsPerML(newStepsPerML);
            EEPROM.put(Config::EEPROM_DOSING_STEPS_ADDR, newStepsPerML);
            EEPROM.commit();

            display.showCalibrationResult(newStepsPerML, pump.getSpeedStep());
            showingCalibrationResult = true;
            lastCalibrationResultTime = millis();
            display.setState(DisplayManager::DisplayState::CALIBRATE_COMPLETE);
        }

        if (pressButtonMenu())
        {
            display.setState(DisplayManager::DisplayState::NORMAL);
        }
    }
    else
    {
        // Calibration movement complete
        pump.stop();
        long actualSteps = pump.getCurrentPosition();
        if (actualSteps < DOSING_CAL_STEPS)
        {
            display.showText("Calibration failed!");
            display.setState(DisplayManager::DisplayState::NORMAL);
            return;
        }

        ml = CALIBRATE_DOSING_VOLUME;
        display.showCalibrationInput(ml);
        display.setState(DisplayManager::DisplayState::CALIBRATE_COMPLETE);
    }
}