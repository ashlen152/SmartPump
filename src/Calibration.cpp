#include "Calibration.h"
#include <EEPROM.h>
#include <DisplayManager.h>
#include "PumpController.h"
#include <Config.h>
extern DisplayManager &display;
extern PumpController pump;
extern unsigned long lastCalibrationResultTime;
extern bool showingCalibrationResult;

void calibrateDosing() {
  const long DOSING_CAL_STEPS = 200000;
  const float DOSING_CAL_SPEED = 2000;
  display.showText("Starting calibration...");
  delay(1000);
  pump.stop();
  delay(100);
  pump.getCurrentPosition();
  pump.setCurrentPosition(0);
  delay(100);
  pump.setMode(PumpMode::DOSING);
  pump.setSpeed(DOSING_CAL_SPEED);
  pump.moveRelative(DOSING_CAL_STEPS);
  unsigned long startTime = millis();
  unsigned long displayUpdate = millis();
  long lastPosition = 0;
  unsigned long lastMoveTime = millis();
  const unsigned long TIMEOUT = 120000;
  while (millis() - startTime < TIMEOUT) {
    pump.runDosing();
    long currentPosition = pump.getCurrentPosition();
    if (currentPosition != lastPosition) {
      lastPosition = currentPosition;
      lastMoveTime = millis();
    }
    if (millis() - displayUpdate >= 1000) {
      display.showText("Moving...");
      Serial.printf("Position: %ld of %ld\n", currentPosition, DOSING_CAL_STEPS);
      displayUpdate = millis();
    }
    if (currentPosition >= DOSING_CAL_STEPS || (!pump.isMoving() && millis() - lastMoveTime > 2000)) {
      break;
    }
  }
  pump.stop();
  long actualSteps = pump.getCurrentPosition();
  bool calibrationComplete = (actualSteps >= DOSING_CAL_STEPS);
  if (!calibrationComplete) {
    Serial.printf("Calibration incomplete. Only moved %ld of %ld steps\n", actualSteps, DOSING_CAL_STEPS);
    display.showText("Calibration Failed!");
    delay(2000);
    return;
  }
  float ml = CALIBRATE_DOSING_VOLUME;
  bool calibrating = true;
  while (calibrating) {
    display.showCalibrationInput(ml);
    if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      ml += 0.1f;
    if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      ml = max(ml - 0.1f, 0.0f);
    if (checkButtonPress(BUTTON_ENABLE_PIN))
      calibrating = false;
  }
  float newStepsPerML = ml > 0 ? DOSING_CAL_STEPS / ml : pump.getDosingStepsPerML();
  pump.setDosingStepsPerML(newStepsPerML);
  EEPROM.put(EEPROM_DOSING_STEPS_ADDR, newStepsPerML);
  EEPROM.commit();
  Serial.printf("Dosing Calibration: %.2f mL moved with %ld steps\n", ml, DOSING_CAL_STEPS);
  Serial.printf("New steps/mL: %.2f\n", newStepsPerML);
  display.showCalibrationResult(newStepsPerML, pump.getSpeedStep());
  showingCalibrationResult = true;
  lastCalibrationResultTime = millis();
}
