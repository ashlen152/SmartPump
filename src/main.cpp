#include <Arduino.h>
#include <EEPROM.h>
#include <AutoDosingManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <Config.h>
#include <ArduinoJson.h>
#include "PumpController.h"
#include "ButtonHandler.h"
#include "MenuHandler.h"
#include "Calibration.h"
#include "DisplayUpdater.h"
#include "WiFiSync.h"

// Auto-dosing EEPROM addresses
#define EEPROM_AUTO_DOSING_ENABLED_ADDR (EEPROM_MODE_ADDR + sizeof(uint8_t))
#define EEPROM_DAILY_VOLUME_ADDR (EEPROM_AUTO_DOSING_ENABLED_ADDR + sizeof(bool))
#define EEPROM_LAST_DOSING_TIME_ADDR (EEPROM_DAILY_VOLUME_ADDR + sizeof(float))
#define EEPROM_TOTAL_DOSED_ADDR (EEPROM_LAST_DOSING_TIME_ADDR + sizeof(uint32_t))

// Auto dosing defaults
#define DEFAULT_DAILY_VOLUME 30.0f    // Default daily volume in mL

#define EN_PIN 26  // Enable
#define DIR_PIN 2  // Direction
#define STEP_PIN 5 // Step
#define RX_PIN 16  // ESP32 RX orange line
#define TX_PIN 17  // ESP32 TX blue line

#define R_SENSE 0.11f
#define DRIVER_ADDR 0b00 // Default address for TMC2209

#define DISPLAY_TIMEOUT 100000
#define EEPROM_ADDR 0

// Auto dosing defaults
#define CALIBRATE_TIME 60     // seconds
#define CALIBRATE_SPEED 20000 // steps/sec

#define SETTINGS_DISPLAY_DURATION 5000 // ms
#define CALIBRATION_RESULT_DURATION 3000 // ms

// Only keep global variable declarations needed for modules
DisplayManager::PumpMode currentMode = DisplayManager::PumpMode::DOSING;
DisplayManager::DosingState dosingState = DisplayManager::DosingState::IDLE;
float targetVolume = 0.0;
float remainingVolume = 0.0;
bool inMenu = false;
int menuIndex = 0;
unsigned long lastButtonPressTime = 0;
unsigned long lastTimeDisplayUpdate = 0;
float currentStepsPerML = 0;
int stepsPerSecond = 2000;
unsigned long lastWiFiRetryTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastSettingsDisplayTime = 0;
unsigned long lastCalibrationResultTime = 0;
bool showingSettings = false;
bool showingCalibrationResult = false;

// Menu items for navigation
const char *menuItems[] = {"Dosing Cal", "Settings Info", "Save Speed", "Mode", "Auto Dosing", "Set Daily Vol"};
const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

// Auto-dosing configuration
AutoDosingManager::Config dosingConfig = {
    .enabledAddr = EEPROM_AUTO_DOSING_ENABLED_ADDR,
    .volumeAddr = EEPROM_DAILY_VOLUME_ADDR,
    .lastTimeAddr = EEPROM_LAST_DOSING_TIME_ADDR,
    .totalDosedAddr = EEPROM_TOTAL_DOSED_ADDR,
    .defaultVolume = DEFAULT_DAILY_VOLUME
};

// Create instances
WiFiManager wifi(ssid, password);
DisplayManager &display = DisplayManager::getInstance();
PumpController pump(&Serial2, STEP_PIN, DIR_PIN, EN_PIN, R_SENSE, DRIVER_ADDR);
AutoDosingManager autoDosing(pump, display, dosingConfig);

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting...");
  EEPROM.begin(512);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  display.begin();
  pump.begin();

  lastWiFiRetryTime = millis();
  display.showText("WiFi Connecting...");
}

void loop()
{
  unsigned long currentTime = millis();
  static bool firstLoop = true;
  if (firstLoop) {
    lastTimeDisplayUpdate = currentTime;
    firstLoop = false;
  }

  // WiFi connection and health logic
  handleWiFi(currentTime, lastWiFiRetryTime);

  // Button/menu handling
  if (checkButtonPress(BUTTON_MENU_PIN)) {
    if (inMenu) {
      runMenuSelection();
      inMenu = false;
    } else {
      inMenu = true;
      menuIndex = 0;
      display.showMenu(menuIndex, menuItems, menuItemCount);
    }
  }
  if (inMenu) {
    if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN)) {
      menuIndex = (menuIndex + 1) % menuItemCount;
      display.showMenu(menuIndex, menuItems, menuItemCount);
    }
    if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN)) {
      menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
      display.showMenu(menuIndex, menuItems, menuItemCount);
    }
  }

  // Dosing logic
  pump.runDosing();

  // Display update
  updateDisplayStatus();

  // Data sync
  if (wifi.isConnected() && currentTime - lastSyncTime >= SYNC_INTERVAL) {
    syncData();
    lastSyncTime = currentTime;
  }

  // Settings display timeout
  if (showingSettings && currentTime - lastSettingsDisplayTime >= SETTINGS_DISPLAY_DURATION) {
    showingSettings = false;
    updateDisplayStatus();
  }

  // Calibration result timeout
  if (showingCalibrationResult && currentTime - lastCalibrationResultTime >= CALIBRATION_RESULT_DURATION) {
    showingCalibrationResult = false;
    updateDisplayStatus();
  }

  // Dosing progress update
  if (!display.isSleeping() && !inMenu && !showingSettings && !showingCalibrationResult && currentMode == DisplayManager::PumpMode::DOSING) {
    if (currentTime - lastTimeDisplayUpdate >= 1000) {
      if (dosingState == DisplayManager::DosingState::RUNNING) {
        const long totalStepsNeeded = targetVolume * pump.getStepsPerML();
        const long currentPosition = pump.getCurrentPosition();
        const long elapsedSteps = abs(currentPosition);
        Serial.printf("Dosing Progress: Steps %ld/%ld, Moving: %d\n", elapsedSteps, totalStepsNeeded, pump.isMoving());
        if (elapsedSteps >= totalStepsNeeded || !pump.isMoving()) {
          Serial.println("Dosing Complete - Target reached or stopped moving");
          pump.stop();
          dosingState = DisplayManager::DosingState::COMPLETED;
          display.showDosingComplete(targetVolume);
        } else {
          remainingVolume = (totalStepsNeeded - elapsedSteps) / pump.getStepsPerML();
          Serial.printf("Remaining volume: %.2f mL\n", remainingVolume);
          display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
        }
      } else if ((dosingState == DisplayManager::DosingState::IDLE || dosingState == DisplayManager::DosingState::COMPLETED) && !inMenu && !showingSettings && !showingCalibrationResult) {
        char nextScheduleStr[6];
        uint32_t nextTime = autoDosing.getNextDosingTime();
        time_t t = nextTime;
        struct tm* timeinfo = localtime(&t);
        snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
        display.updateStatus(pump.isEnabled(), autoDosing.getRemainingDailyVolume(), currentMode, wifi.getCurrentTime(), autoDosing.isEnabled(), nextScheduleStr);
      }
      lastTimeDisplayUpdate = currentTime;
    }
  }

  // Auto-dosing
  if (autoDosing.isEnabled()) {
    autoDosing.updateSchedule();
    autoDosing.checkAndDose();
  }
}