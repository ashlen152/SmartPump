/**
 * @file main.cpp
 * @brief SmartPump entry point - setup() and loop() for ESP32 Arduino.
 *
 * Initialization sequence (setup):
 *   1. Serial (115200 baud) and EEPROM (512 bytes)
 *   2. Serial2 for TMC2209 UART (RX=16, TX=17)
 *   3. Button GPIO pins with internal pull-ups
 *   4. OLED display initialization
 *   5. PumpController init and begin
 *   6. WiFi connection attempt (shown on display)
 *
 * Main loop priority order:
 *   1. HomeHandler()        - Handle Home screen button inputs
 *   2. ManualHandler()      - Handle manual dosing state machine
 *   3. updateDisplayStatus() - Refresh OLED based on current DisplayState
 *   4. pump.runDosing()     - Execute stepper steps (time-critical)
 *   5. Early return if DOSING mode (skip WiFi for timing accuracy)
 *   6. handleWiFi()         - WiFi reconnect and health check
 *
 * @note Auto-dosing and data sync are currently commented out (WIP).
 */

#include <Arduino.h>
#include <EEPROM.h>
#include <esp_task_wdt.h>  // Phase 3 Sprint 4: Watchdog timer
#include <Config.h>
#include <ButtonConfig.h>
#include <AutoDosingManager.h>
#include <ConfigManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <NetworkTaskManager.h>
#include "WifiController/WiFiSync.h"
#include "DisplayController/DisplayUpdater.h"
#include "PumpController.h"
#include "ViewController/Home/HomeHandler.h"
#include "ViewController/Menu/MenuHandler.h"
#include "ViewController/Manual/ManualHandler.h"

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
  DisplayManager &display = DisplayManager::getInstance();
  PumpController &pump = PumpController::getInstance();
  NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
  ConfigManager &config = ConfigManager::getInstance();

  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting SmartPump...");
  
  // Initialize watchdog timer (10 second timeout)
  esp_task_wdt_init(10, true);  // 10 seconds, panic on timeout
  esp_task_wdt_add(NULL);        // Add current task (Core 1 main loop)
  Serial.println("Watchdog timer initialized (10s timeout)");
  
  EEPROM.begin(512);
  Serial2.begin(115200, SERIAL_8N1, Config::RX_PIN, Config::TX_PIN);

  pinMode(BUTTON_ENABLE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);

  // Initialize ConfigManager (loads pump ID from EEPROM)
  config.begin();
  Serial.print("Pump ID: ");
  Serial.println(config.getPumpId());

  display.begin();
  pump.init(&Serial2, Config::STEP_PIN, Config::DIR_PIN, Config::STEPPER_EN_PIN, Config::R_SENSE, Config::DRIVER_ADDR);
  pump.begin();

  // Initialize AutoDosingManager
  Serial.println("Initializing auto-dosing...");
  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
  AutoDosingManager::Config dosingConfig = {
      .enabledAddr = Config::EEPROM_AUTO_DOSING_ENABLED_ADDR,
      .volumeAddr = Config::EEPROM_DAILY_VOLUME_ADDR,
      .lastTimeAddr = Config::EEPROM_LAST_DOSING_TIME_ADDR,
      .totalDosedAddr = Config::EEPROM_TOTAL_DOSED_ADDR,
      .dayStartHourAddr = Config::EEPROM_DAY_START_HOUR_ADDR,
      .dayEndHourAddr = Config::EEPROM_DAY_END_HOUR_ADDR,
      .dayPercentAddr = Config::EEPROM_DAY_PERCENT_ADDR,
      .defaultVolume = Config::DEFAULT_DAILY_VOLUME
  };
  autoDosing.initialize(pump, display, dosingConfig);
  autoDosing.begin();
  Serial.println("Auto-dosing initialized");

  // Initialize and start network task on Core 0
  Serial.println("Initializing network task...");
  if (networkTask.initialize(10, 10)) {
    Serial.println("Network task initialized successfully");
    if (networkTask.start(8192, 1)) {
      Serial.println("Network task started on Core 0");
      
      // Send initial WiFi connection command to Core 0
      NetworkCommandMessage cmd;
      cmd.command = NetworkCommand::CONNECT_WIFI;
      cmd.param1 = 0;
      cmd.param2 = 0;
      memset(cmd.data, 0, sizeof(cmd.data));
      
      if (networkTask.sendCommand(cmd, 0)) {
        Serial.println("WiFi connection command queued");
        display.showText("WiFi Connecting...");
      } else {
        Serial.println("Failed to queue WiFi connection command");
        display.showText("WiFi Init Failed");
      }
    } else {
      Serial.println("Failed to start network task");
      display.showText("Network Task Failed");
    }
  } else {
    Serial.println("Failed to initialize network task");
    display.showText("Network Init Failed");
  }

  lastWiFiRetryTime = millis();
}

void loop()
{
  // Reset watchdog timer every loop iteration
  esp_task_wdt_reset();
  
  PumpController &pump = PumpController::getInstance();
  NetworkTaskManager &networkTask = NetworkTaskManager::getInstance();
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

  // Display update
  updateDisplayStatus();

  // Dosing logic (time-critical, runs every loop)
  pump.runDosing();

  // Prevent further processing if in dosing mode
  // for reduced latency and responsiveness
  if (pump.getMode() == PumpMode::DOSING)
    return;

  // Check for network responses from Core 0 (non-blocking)
  NetworkResponseMessage response;
  while (networkTask.getResponse(response, 0)) {
    // Handle network responses
    switch (response.command) {
      case NetworkCommand::CONNECT_WIFI:
        if (response.status == NetworkStatus::WIFI_CONNECTED) {
          Serial.printf("[Main] WiFi connected: %s\n", response.data);
          DisplayManager::getInstance().setSignalStrength(response.value);
          
          // Request time sync after WiFi connection
          NetworkCommandMessage syncCmd;
          syncCmd.command = NetworkCommand::SYNC_TIME;
          syncCmd.param1 = 0;
          syncCmd.param2 = 0;
          memset(syncCmd.data, 0, sizeof(syncCmd.data));
          networkTask.sendCommand(syncCmd, 0);
        } else {
          Serial.printf("[Main] WiFi connection failed: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::SYNC_TIME:
        if (response.status == NetworkStatus::TIME_SYNCED) {
          Serial.printf("[Main] Time synced: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::HTTP_GET_SETTINGS:
      case NetworkCommand::HTTP_POST_SETTINGS:
        if (response.status == NetworkStatus::HTTP_OK) {
          Serial.printf("[Main] HTTP success: %s\n", response.data);
        } else {
          Serial.printf("[Main] HTTP failed: %s\n", response.data);
        }
        break;
        
      case NetworkCommand::HEALTH_CHECK:
        if (response.status == NetworkStatus::SUCCESS) {
          Serial.println("[Main] Server health check OK");
        } else {
          Serial.println("[Main] Server health check failed");
        }
        break;
        
      case NetworkCommand::AUTO_DOSING_RESET:
        if (response.status == NetworkStatus::SUCCESS) {
          Serial.println("[Main] ⏰ Midnight reset detected - resetting auto-dosing");
          AutoDosingManager::getInstance().resetDailyVolume();
        }
        break;
        
      default:
        break;
    }
  }

  // Auto-dosing check and execution (non-blocking)
  AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
  if (autoDosing.isEnabled()) {
    static unsigned long lastScheduleUpdate = 0;
    static unsigned long lastDoseCheck = 0;
    
    // Update schedule every 5 minutes to recalculate next dose
    if (currentTime - lastScheduleUpdate >= 300000) {
      autoDosing.updateSchedule();
      lastScheduleUpdate = currentTime;
    }
    
    // Check for scheduled doses every second
    if (currentTime - lastDoseCheck >= 1000) {
      autoDosing.checkAndDose();
      lastDoseCheck = currentTime;
    }
  }

  // WiFi connection is now handled by Core 0 background tasks
  // No need for handleWiFi() - Core 0 auto-reconnects every 5 seconds

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
