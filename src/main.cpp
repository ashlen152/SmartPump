#include <Arduino.h>
#include <EEPROM.h>
#include <Config.h>
#include <ButtonConfig.h>
// #include <AutoDosingManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include "WifiController/WiFiSync.h"
#include "DisplayController/DisplayUpdater.h"
#include "PumpController.h"
#include "ViewController/Home/HomeHandler.h"
#include "ViewController/Menu/MenuHandler.h"
#include "ViewController/Manual/ManualHandler.h"
#include "ViewController/Calibrate/CalibrateHandler.h"

// #include <ArduinoJson.h>
// #include "PumpController.h"
// #include "ButtonHandler.h"
// #include "MenuHandler.h"
// #include "Calibration.h"
// #include "DisplayUpdater.h"
// #include "WiFiSync.h"

// // Only keep global variable declarations needed for modules
// DisplayManager::PumpMode currentMode = DisplayManager::PumpMode::DOSING;
// DisplayManager::DosingState dosingState = DisplayManager::DosingState::IDLE;
float targetVolume = 0.0;
float remainingVolume = 0.0;
unsigned long lastTimeDisplayUpdate = 0;
float currentStepsPerML = 0;
int stepsPerSecond = 2000;
unsigned long lastWiFiRetryTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastCalibrationResultTime = 0;
bool showingCalibrationResult = false;

// // Auto-dosing configuration
// AutoDosingManager::Config dosingConfig = {
//     .enabledAddr = EEPROM_AUTO_DOSING_ENABLED_ADDR,
//     .volumeAddr = EEPROM_DAILY_VOLUME_ADDR,
//     .lastTimeAddr = EEPROM_LAST_DOSING_TIME_ADDR,
//     .totalDosedAddr = EEPROM_TOTAL_DOSED_ADDR,
//     .defaultVolume = DEFAULT_DAILY_VOLUME
// };

// // Create instances

// AutoDosingManager autoDosing(pump, display, dosingConfig);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting...");
  EEPROM.begin(512);
  Serial2.begin(115200, SERIAL_8N1, Config::RX_PIN, Config::TX_PIN);

  pinMode(BUTTON_ENABLE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);

  DisplayManager &display = DisplayManager::getInstance();
  PumpController &pump = PumpController::getInstance();

  display.begin();
  pump.begin();

  lastWiFiRetryTime = millis();
  display.showText("WiFi Connecting...");
}

void loop()
{
  PumpController &pump = PumpController::getInstance();
  unsigned long currentTime = millis();
  static bool firstLoop = true;
  if (firstLoop)
  {
    lastTimeDisplayUpdate = currentTime;
    firstLoop = false;
  }

  // Button and menu handling
  HomeHandler();
  ManualHandler();
  CalibrateHandler();
  MenuHandler();

  // Display update
  updateDisplayStatus();

  // Dosing logic
  pump.runDosing();

  // Prevent further processing if in dosing mode
  // for reduced latency and responsiveness
  if (pump.getMode() == PumpMode::DOSING)
    return;

  // WiFi connection and health logic
  handleWiFi(currentTime, lastWiFiRetryTime);

  // // Data sync
  // if (wifi.isConnected() && currentTime - lastSyncTime >= SYNC_INTERVAL) {
  //   syncData();
  //   lastSyncTime = currentTime;
  // }

  // // Settings display timeout
  // if (showingSettings && currentTime - lastSettingsDisplayTime >= SETTINGS_DISPLAY_DURATION) {
  //   showingSettings = false;
  //   updateDisplayStatus();
  // }

  // // Calibration result timeout
  // if (showingCalibrationResult && currentTime - lastCalibrationResultTime >= CALIBRATION_RESULT_DURATION) {
  //   showingCalibrationResult = false;
  //   updateDisplayStatus();
  // }

  // // Dosing progress update
  // if (!display.isSleeping() && !inMenu && !showingSettings && !showingCalibrationResult && currentMode == DisplayManager::PumpMode::DOSING) {
  //   if (currentTime - lastTimeDisplayUpdate >= 1000) {
  //     if (dosingState == DisplayManager::DosingState::RUNNING) {
  //       const long totalStepsNeeded = targetVolume * pump.getStepsPerML();
  //       const long currentPosition = pump.getCurrentPosition();
  //       const long elapsedSteps = abs(currentPosition);
  //       Serial.printf("Dosing Progress: Steps %ld/%ld, Moving: %d\n", elapsedSteps, totalStepsNeeded, pump.isMoving());
  //       if (elapsedSteps >= totalStepsNeeded || !pump.isMoving()) {
  //         Serial.println("Dosing Complete - Target reached or stopped moving");
  //         pump.stop();
  //         dosingState = DisplayManager::DosingState::COMPLETED;
  //         display.showDosingComplete(targetVolume);
  //       } else {
  //         remainingVolume = (totalStepsNeeded - elapsedSteps) / pump.getStepsPerML();
  //         Serial.printf("Remaining volume: %.2f mL\n", remainingVolume);
  //         display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
  //       }
  //     } else if ((dosingState == DisplayManager::DosingState::IDLE || dosingState == DisplayManager::DosingState::COMPLETED) && !inMenu && !showingSettings && !showingCalibrationResult) {
  //       char nextScheduleStr[6];
  //       uint32_t nextTime = autoDosing.getNextDosingTime();
  //       time_t t = nextTime;
  //       struct tm* timeinfo = localtime(&t);
  //       snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
  //       display.updateStatus(pump.isEnabled(), autoDosing.getRemainingDailyVolume(), currentMode, wifi.getCurrentTime(), autoDosing.isEnabled(), nextScheduleStr);
  //     }
  //     lastTimeDisplayUpdate = currentTime;
  //   }
  // }

  // Auto-dosing
  // if (autoDosing.isEnabled()) {
  //   autoDosing.updateSchedule();
  //   autoDosing.checkAndDose();
  // }
}
