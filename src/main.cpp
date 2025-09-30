#include <Arduino.h>
#include <EEPROM.h>
#include <AutoDosingManager.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <Config.h>
#include <ArduinoJson.h>
#include "PumpController.h"
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

DisplayManager::PumpMode currentMode = DisplayManager::PumpMode::PERISTALTIC; // Default mode
DisplayManager::DosingState dosingState = DisplayManager::DosingState::IDLE;

// Manual dosing parameters
float targetVolume = 0.0;    // Target volume in mL
float remainingVolume = 0.0; // Remaining volume to be pumped

#define SETTINGS_DISPLAY_DURATION 5000 // ms

#define CALIBRATION_RESULT_DURATION 3000 // ms

// State
bool inMenu = false;
int menuIndex = 0;
unsigned long lastButtonPressTime = 0;
unsigned long lastTimeDisplayUpdate = 0; // For updating time display
float currentStepsPerML = 0;
int stepsPerSecond = 2000;
const char *menuItems[] = {"Peristaltic Cal", "Dosing Cal", "Settings Info", "Save Speed", "Mode", "Auto Dosing", "Set Daily Vol"};
const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);
unsigned long lastWiFiRetryTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastSettingsDisplayTime = 0;
unsigned long lastCalibrationResultTime = 0;
bool showingSettings = false;
bool showingCalibrationResult = false;

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

// Forward declarations
bool checkButtonPress(uint8_t pin);
bool checkButtonPressOrHold(uint8_t pin);
void calibrateDrop();
void calibrateDosing();
void runMenuSelection();
void syncData();

void setup()
{
  Serial.begin(115200);
  while (!Serial)
    ;
  Serial.println("Starting...");
  EEPROM.begin(512);
  Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);

  // Load pump mode from EEPROM
  uint8_t savedMode;
  EEPROM.get(EEPROM_MODE_ADDR, savedMode);
  if (isnan(savedMode))
  {
    currentMode = static_cast<DisplayManager::PumpMode>(savedMode);
  }
  else
  {
    currentMode = DisplayManager::PumpMode::DOSING;
  }

  pinMode(BUTTON_ENABLE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);

  display.begin();
  pump.begin();

  // Load calibration values and speed from EEPROM
  float peristalticStepsPerML = 0;
  float dosingStepsPerML = 0;
  float savedSpeed = 0;

  EEPROM.get(EEPROM_PERISTALTIC_STEPS_ADDR, peristalticStepsPerML);
  EEPROM.get(EEPROM_DOSING_STEPS_ADDR, dosingStepsPerML);
  EEPROM.get(EEPROM_SAVED_SPEED_ADDR, savedSpeed);

  // Set default values if invalid
  if (isnan(peristalticStepsPerML) || peristalticStepsPerML <= 0)
    peristalticStepsPerML = 709.22f;
  if (isnan(dosingStepsPerML) || dosingStepsPerML <= 0)
    dosingStepsPerML = 709.22f;

  // Set calibration values based on mode
  pump.setPeristalticStepsPerML(peristalticStepsPerML);
  pump.setDosingStepsPerML(dosingStepsPerML);

  // Calculate speed step based on current mode
  stepsPerSecond = pump.getStepsPerML() > 0 ? (int)(pump.getStepsPerML() / 60) : 2000;
  pump.setSpeedStep(stepsPerSecond);

  if (!isnan(savedSpeed) && savedSpeed > 0)
  {
    pump.setSpeed(savedSpeed);
    Serial.print("Loaded saved speed from EEPROM: ");
    Serial.println(savedSpeed);
  }

  lastWiFiRetryTime = millis();
  display.showText("WiFi Connecting...");
}

void loop()
{
  unsigned long currentTime = millis();
  // Initialize lastTimeDisplayUpdate in the first loop
  static bool firstLoop = true;
  if (firstLoop)
  {
    lastTimeDisplayUpdate = currentTime;
    firstLoop = false;
  }

  // WiFi Connection Handling
  if (!wifi.isConnected())
  {
    if (currentTime - lastWiFiRetryTime >= WIFI_RETRY_INTERVAL)
    {
      Serial.println("Attempting WiFi Connect...");
      if (wifi.connect())
      {
        display.setSignalStrength(wifi.getSignalStrength());
        display.showText("Syncing Time...");
        wifi.configureTime("asia.pool.ntp.org", "ICT-7"); // Configure for Indochina Time (UTC+7)        display.showText("WiFi Connected");
        if (wifi.checkApiHealth())
        {
          String response;
          if (wifi.get((String(PUMP_BY_ID_API) + "?pump-id=" + String(ID_PERISTALTIC_STEPPER)).c_str(), response))
          {
            // Parse the JSON response
            JsonDocument doc; // Adjust size as needed
            DeserializationError error = deserializeJson(doc, response);

            if (error)
            {
              Serial.print("Failed to parse JSON: ");
              Serial.println(error.c_str());
              display.showText("Invalid Server Data");
            }
            else
            {
              // Extract current speed from the response
              if (doc["currentSpeed"].is<float>())
              {
                float currentSpeed = doc["currentSpeed"];
                Serial.print("Setting pump speed to: ");
                Serial.println(currentSpeed);

                // Update the pump's speed
                pump.setSpeed(currentSpeed);

                // Only update display if in main screen
                if (!inMenu && !showingSettings && !showingCalibrationResult)
                {
                  float displayValue = currentSpeed > 0 ? currentSpeed / pump.getSpeedStep() : 0;
                   // Format next schedule time for dosing mode
                   char nextScheduleStr[6] = {0}; // HH:MM\0
                   if (currentMode == DisplayManager::PumpMode::DOSING && autoDosing.isEnabled()) {
                     uint32_t nextTime = autoDosing.getNextDosingTime();
                     time_t t = nextTime;
                     struct tm* timeinfo = localtime(&t);
                     snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d", 
                             timeinfo->tm_hour, timeinfo->tm_min);
                   }

                   display.updateStatus(pump.isEnabled(), displayValue, currentMode,
                                      currentMode == DisplayManager::PumpMode::DOSING ? wifi.getCurrentTime() : nullptr,
                                      autoDosing.isEnabled(),
                                      currentMode == DisplayManager::PumpMode::DOSING ? nextScheduleStr : nullptr);
                }
              }
              else
              {
                Serial.println("Response missing 'currentSpeed' field");
                display.showText("Invalid Server Data");
              }
            }

            display.showText("Server OK");

            // Only update display if in main screen
            if (!inMenu && !showingSettings && !showingCalibrationResult)
            {
              float displayValue = pump.getSpeed() > 0 ? pump.getSpeed() / pump.getSpeedStep() : 0;
              display.updateStatus(pump.isEnabled(), displayValue, currentMode,
                                   currentMode == DisplayManager::PumpMode::DOSING ? wifi.getCurrentTime() : nullptr);
            }
          }
        }
        else
        {
          // display.showText("Server Down");
        }
      }
      else
      {
        display.showText("WiFi Failed");
      }
      lastWiFiRetryTime = currentTime;
    }
  }
  else if (!wifi.isConnected() && currentTime - lastWiFiRetryTime >= WIFI_RETRY_INTERVAL)
  {
    Serial.println("Server Health Check Failed");
    display.showText("Server Down");
    lastWiFiRetryTime = currentTime;
  }

  else if (!showingSettings && !showingCalibrationResult)
  {
    // Global MENU button handling
    if (checkButtonPress(BUTTON_MENU_PIN))
    {
      if (inMenu)
      {
        runMenuSelection();
        inMenu = false;
      }
      else if (currentMode == DisplayManager::PumpMode::DOSING &&
               (dosingState == DisplayManager::DosingState::SETUP_VOLUME ||
                dosingState == DisplayManager::DosingState::SETUP_TIME))
      {
        // Cancel dosing setup and return to main screen
        dosingState = DisplayManager::DosingState::IDLE;
        if (!inMenu && !showingSettings && !showingCalibrationResult)
        {
          display.updateStatus(pump.isEnabled(), pump.getSpeed(), currentMode, wifi.getCurrentTime());
        }
      }
      else if (!inMenu)
      {
        inMenu = true;
        menuIndex = 0;
        display.showMenu(menuIndex, menuItems, menuItemCount);
      }
    }

    // Menu navigation
    if (inMenu)
    {
      if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      {
        menuIndex = (menuIndex + 1) % menuItemCount;
        display.showMenu(menuIndex, menuItems, menuItemCount);
      }
      if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      {
        menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
        display.showMenu(menuIndex, menuItems, menuItemCount);
      }
    }
    // Normal operation modes
    else if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
    {
      // Handle PERISTALTIC mode buttons
      if (checkButtonPress(BUTTON_ENABLE_PIN))
      {
        if (pump.isEnabled())
          pump.stop();
        else
        {
          pump.setSpeed(pump.getSpeed() > 0 ? pump.getSpeed() : 0);
          pump.setMode(PumpMode::PERISTALTIC);
        }

         float displayValue = pump.getStepsPerML() > 0 ? pump.getSpeed() / pump.getStepsPerML() * 60 : 0;
         display.updateStatus(pump.isEnabled(), displayValue, currentMode, nullptr,
                          currentMode == DisplayManager::PumpMode::DOSING ? autoDosing.isEnabled() : false,
                          nullptr);
      }

      if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      {
        pump.setSpeed(pump.getSpeed() + pump.getSpeedStep());
        pump.setMode(PumpMode::PERISTALTIC);
        float displayValue = pump.getSpeed() > 0 ? pump.getSpeed() / pump.getStepsPerML() * 60 : 0;
        display.updateStatus(pump.isEnabled(), displayValue, currentMode, nullptr);
      }

      if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      {
        pump.setSpeed(max(pump.getSpeed() - pump.getSpeedStep(), 0.0f));
        pump.setMode(PumpMode::PERISTALTIC);
        float displayValue = pump.getSpeed() > 0 ? pump.getSpeed() / pump.getStepsPerML() * 60 : 0;
        display.updateStatus(pump.isEnabled(), displayValue, currentMode, nullptr);
      }
    }
    else
    {
      // Handle DOSING mode buttons
      if (checkButtonPress(BUTTON_ENABLE_PIN))
      {
        switch (dosingState)
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

  if (!display.isSleeping() && (currentTime - lastButtonPressTime >= DISPLAY_TIMEOUT))
  {
    Serial.print(currentTime - lastButtonPressTime);
    Serial.print(" > ");
    Serial.println("Display Timeout");
    display.sleepDisplay();
  }
  // Sync Data
  if (wifi.isConnected() && currentTime - lastSyncTime >= SYNC_INTERVAL)
  {
    syncData();
    lastSyncTime = currentTime;
  }

  // Handle Settings Display Timeout
  if (showingSettings && currentTime - lastSettingsDisplayTime >= SETTINGS_DISPLAY_DURATION)
  {
    showingSettings = false;
    float displayValue;
    if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
    {
      displayValue = pump.getStepsPerML() > 0 ? pump.getSpeed() / pump.getStepsPerML() * 60 : 0;
    }
    else
    {
      displayValue = pump.getSpeed();
    }
    display.updateStatus(pump.isEnabled(), displayValue, currentMode,
                         currentMode == DisplayManager::PumpMode::DOSING ? wifi.getCurrentTime() : nullptr);
  }

  // Handle Calibration Result Timeout
  if (showingCalibrationResult && currentTime - lastCalibrationResultTime >= CALIBRATION_RESULT_DURATION)
  {
    showingCalibrationResult = false;
    float displayValue;
    if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
    {
      displayValue = pump.getStepsPerML() > 0 ? pump.getSpeed() / pump.getStepsPerML() * 60 : 0;
    }
    else
    {
      displayValue = pump.getSpeed();
    }
    display.updateStatus(pump.isEnabled(), displayValue, currentMode,
                         currentMode == DisplayManager::PumpMode::DOSING ? wifi.getCurrentTime() : nullptr);
  }

  // Update time display and check dosing progress
  if (!display.isSleeping() &&
      !inMenu &&
      !showingSettings &&
      !showingCalibrationResult &&
      currentMode == DisplayManager::PumpMode::DOSING)
  {

    if (currentTime - lastTimeDisplayUpdate >= 1000)
    { // Update every second
      if (dosingState == DisplayManager::DosingState::RUNNING)
      {
        // Use actual position from stepper for accurate tracking
        const long totalStepsNeeded = targetVolume * pump.getStepsPerML();
        const long currentPosition = pump.getCurrentPosition();
        const long elapsedSteps = abs(currentPosition); // Use absolute position

        // Non-blocking position check
        Serial.printf("Dosing Progress: Steps %ld/%ld, Moving: %d\n",
                      elapsedSteps, totalStepsNeeded, pump.isMoving());

        if (elapsedSteps >= totalStepsNeeded || !pump.isMoving())
        {
          // Dosing completed when target reached or motion stopped
          Serial.println("Dosing Complete - Target reached or stopped moving");
          pump.stop();
          dosingState = DisplayManager::DosingState::COMPLETED;
          display.showDosingComplete(targetVolume);
        }
        else
        {
          // Update remaining volume based on actual position
          remainingVolume = (totalStepsNeeded - elapsedSteps) / pump.getStepsPerML();
          Serial.printf("Remaining volume: %.2f mL\n", remainingVolume);
          display.showDosingProgress(targetVolume, remainingVolume, wifi.getCurrentTime());
        }
      }
      else if ((dosingState == DisplayManager::DosingState::IDLE ||
                dosingState == DisplayManager::DosingState::COMPLETED) &&
               !inMenu && !showingSettings && !showingCalibrationResult)
      {
        // Format next schedule time
        char nextScheduleStr[6]; // HH:MM\0
        uint32_t nextTime = autoDosing.getNextDosingTime();
        time_t t = nextTime;
        struct tm* timeinfo = localtime(&t);
        snprintf(nextScheduleStr, sizeof(nextScheduleStr), "%02d:%02d", 
                 timeinfo->tm_hour, timeinfo->tm_min);

        // Only update status in home screen (not in menus or setup)
        display.updateStatus(pump.isEnabled(),
                           autoDosing.getRemainingDailyVolume(),
                           currentMode,
                           wifi.getCurrentTime(),
                           autoDosing.isEnabled(),
                           nextScheduleStr);
      }
      lastTimeDisplayUpdate = currentTime;
    }
  }

  // Run pump in appropriate mode
  if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
  {
    pump.runPeristaltic();
  }
  else
  {
    pump.runDosing();

    // Handle auto-dosing
    if (autoDosing.isEnabled())
    {   
      autoDosing.updateSchedule();
      autoDosing.checkAndDose();
    }
  }
}

void syncData()
{
  JsonDocument doc;

  int rssi = wifi.getSignalStrength();

  doc["pumpId"] = ID_PERISTALTIC_STEPPER;
  doc["stepsPerML"] = pump.getStepsPerML();
  doc["stepsPerSecond"] = pump.getSpeedStep();
  doc["currentSpeed"] = pump.getSpeed();
  doc["rssi"] = rssi;
  display.setSignalStrength(rssi);

  String jsonData;
  serializeJson(doc, jsonData);

  String response;
  if (wifi.post(PUMP_SETTINGS_API, "application/json", jsonData.c_str(), response))
  {
    Serial.println("Sync Ok");
  }
  else
  {
    Serial.println("Sync failed");
  }
}

void calibrateDosing()
{
  const long DOSING_CAL_STEPS = 200000; // Exact number of steps for calibration
  const float DOSING_CAL_SPEED = 2000;  // Fixed speed for calibration

  display.showText("Starting calibration...");
  delay(1000);

  // Stop and reset position
  pump.stop();
  delay(100);
  pump.getCurrentPosition();  // Read current position
  pump.setCurrentPosition(0); // Reset to 0
  delay(100);

  // Configure pump for calibration
  pump.setMode(PumpMode::DOSING);
  pump.setSpeed(DOSING_CAL_SPEED);

  // Move exact number of steps
  pump.moveRelative(DOSING_CAL_STEPS);

  unsigned long startTime = millis();
  unsigned long displayUpdate = millis();

  // Wait for movement to complete
  Serial.println("Starting calibration movement...");
  long lastPosition = 0;
  unsigned long lastMoveTime = millis();
  const unsigned long TIMEOUT = 120000; // 2 minutes timeout

  while (millis() - startTime < TIMEOUT)
  {
    pump.runDosing();

    long currentPosition = pump.getCurrentPosition();

    // Check if we're actually moving
    if (currentPosition != lastPosition)
    {
      lastPosition = currentPosition;
      lastMoveTime = millis();
    }

    // Update display and debug info every second
    if (millis() - displayUpdate >= 1000)
    {
      display.showText("Moving...");
      Serial.printf("Position: %ld of %ld\n", currentPosition, DOSING_CAL_STEPS);
      displayUpdate = millis();
    }

    // Check if we've reached target or if we're stuck
    if (currentPosition >= DOSING_CAL_STEPS ||
        (!pump.isMoving() && millis() - lastMoveTime > 2000))
    {
      break;
    }
  }

  // Stop movement and check status
  pump.stop();

  // Calculate actual moved steps
  long actualSteps = pump.getCurrentPosition();
  bool calibrationComplete = (actualSteps >= DOSING_CAL_STEPS);

  if (!calibrationComplete)
  {
    Serial.printf("Calibration incomplete. Only moved %ld of %ld steps\n",
                  actualSteps, DOSING_CAL_STEPS);
    display.showText("Calibration Failed!");
    delay(2000);
    return;
  }

  // Get measured volume from user
  float ml = CALIBRATE_DOSING_VOLUME;
  bool calibrating = true;
  while (calibrating)
  {
    display.showCalibrationInput(ml);
    if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      ml += 0.1f;
    if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      ml = max(ml - 0.1f, 0.0f);
    if (checkButtonPress(BUTTON_ENABLE_PIN))
      calibrating = false;
  }

  // Calculate steps per mL based on actual moved volume
  float newStepsPerML = ml > 0 ? DOSING_CAL_STEPS / ml : pump.getDosingStepsPerML();
  pump.setDosingStepsPerML(newStepsPerML);
  EEPROM.put(EEPROM_DOSING_STEPS_ADDR, newStepsPerML);
  EEPROM.commit();

  // Debug info
  Serial.printf("Dosing Calibration: %.2f mL moved with %ld steps\n", ml, DOSING_CAL_STEPS);
  Serial.printf("New steps/mL: %.2f\n", newStepsPerML);

  display.showCalibrationResult(newStepsPerML, pump.getSpeedStep());
  showingCalibrationResult = true;
  lastCalibrationResultTime = millis();
}

void runMenuSelection()
{
  switch (menuIndex)
  {
  case 0: // Peristaltic Calibration
    calibrateDrop();
    break;

  case 1: // Dosing Calibration
    calibrateDosing();
    break;

  case 2: // Settings Info
    display.showSettingsInfo(pump.getSpeed(),
                             currentMode == DisplayManager::PumpMode::DOSING ? pump.getDosingStepsPerML() : pump.getPeristalticStepsPerML(),
                             pump.getSpeedStep());
    showingSettings = true;
    lastSettingsDisplayTime = millis();
    break;

  case 3: // Save Speed
  {
    float currentSpeed = pump.getSpeed();
    EEPROM.put(EEPROM_SAVED_SPEED_ADDR, currentSpeed);
    EEPROM.commit();
    Serial.print("Saved speed to EEPROM: ");
    Serial.println(currentSpeed);
    display.showText("Speed Saved!");
    delay(1000);
  }
  break;

  case 4: // Change mode
  {
    const char *modeItems[] = {"Peristaltic", "Dosing"};
    const int modeCount = sizeof(modeItems) / sizeof(modeItems[0]);
    int modeIndex = static_cast<int>(currentMode);
    bool modeChanged = false;

    // Show initial mode selection
    display.showMenu(modeIndex, modeItems, modeCount);

    // Mode selection loop with timeout
    unsigned long modeSelectionStart = millis();
    while (millis() - modeSelectionStart < 10000) // 10 second timeout
    {
      if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      {
        modeIndex = (modeIndex + 1) % modeCount;
        display.showMenu(modeIndex, modeItems, modeCount);
      }

      if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      {
        modeIndex = (modeIndex - 1 + modeCount) % modeCount;
        display.showMenu(modeIndex, modeItems, modeCount);
      }

      if (checkButtonPress(BUTTON_ENABLE_PIN))
      {
        currentMode = static_cast<DisplayManager::PumpMode>(modeIndex);
        uint8_t modeToSave = static_cast<uint8_t>(currentMode);
        EEPROM.put(EEPROM_MODE_ADDR, modeToSave);
        EEPROM.commit();

        display.showText("Mode Changed!");
        delay(1000);
        modeChanged = true;
        break;
      }

      if (checkButtonPress(BUTTON_MENU_PIN))
      {
        break;
      }

      // Keep pump running during mode selection
      if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
      {
        pump.runPeristaltic();
      }
      else
      {
        pump.runDosing();
      }
    }

    // If no mode was selected, show timeout message
    if (!modeChanged)
    {
      display.showText("Selection Cancelled");
      delay(1000);
    }
  }
  break;

  case 5: // Auto Dosing
    if (autoDosing.isEnabled())
    {
      autoDosing.disable();
      display.showText("Auto Dosing Off");
    }
    else
    {
      autoDosing.enable();
      display.showText("Auto Dosing On");
    }
    delay(1000);
    break;

  case 6: // Set Daily Volume
  {
    float volume = 10.0f; // Start with 10mL
    bool setting = true;
    while (setting)
    {
      display.showValue("Daily Volume (mL)", volume);
      if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
        volume += 1.0f;
      if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
        volume = max(volume - 1.0f, 1.0f);
      if (checkButtonPress(BUTTON_ENABLE_PIN))
      {
        autoDosing.setDailyVolume(volume);
        setting = false;
      }
      if (checkButtonPress(BUTTON_MENU_PIN))
      {
        setting = false;
      }
    }
  }
  break;
  }
}

void calibrateDrop()
{
  display.showCalibrationStart(CALIBRATE_PERISTALTIC_TIME);
  pump.setSpeed(CALIBRATE_SPEED);
  unsigned long startTime = millis();
  unsigned long runDuration = CALIBRATE_PERISTALTIC_TIME * 1000UL;
  unsigned long lastUpdate = 0;
  while (millis() - startTime < runDuration)
  {
    if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
    {
      pump.runPeristaltic();
    }
    else
    {
      pump.runDosing();
    }
    if (millis() - lastUpdate > 1000)
    {
      lastUpdate = millis();
      display.showCalibrationStart((runDuration - (millis() - startTime)) / 1000);
    }
  }
  pump.stop();
  float ml = 0.0f;
  bool calibrating = true;
  while (calibrating)
  {
    display.showCalibrationInput(ml);
    if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
      ml += 0.1f;
    if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
      ml = max(ml - 0.1f, 0.0f);
    if (checkButtonPress(BUTTON_ENABLE_PIN))
      calibrating = false;
  }

  float newStepsPerML = ml > 0 ? (float)CALIBRATE_SPEED / ml : 0;
  stepsPerSecond = newStepsPerML > 0 ? (int)(newStepsPerML / 60) : 2000;
  pump.setPeristalticStepsPerML(newStepsPerML);
  pump.setSpeedStep(stepsPerSecond);
  EEPROM.put(EEPROM_PERISTALTIC_STEPS_ADDR, newStepsPerML);
  EEPROM.commit();
  display.showCalibrationResult(newStepsPerML, stepsPerSecond);
  showingCalibrationResult = true;
  lastCalibrationResultTime = millis();
}

bool checkButtonPress(uint8_t pin)
{
  if (digitalRead(pin) == LOW)
  {
    lastButtonPressTime = millis();
    if (display.isSleeping())
    {
      display.wakeDisplay();
      while (digitalRead(pin) == LOW)
      {
        if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
        {
          pump.runPeristaltic();
        }
        else
        {
          pump.runDosing();
        }
      }
      return false;
    }
    while (digitalRead(pin) == LOW)
    {
      if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
      {
        pump.runPeristaltic();
      }
      else
      {
        pump.runDosing();
      }
    }
    return true;
  }
  return false;
}

bool checkButtonPressOrHold(uint8_t pin)
{
  static unsigned long holdStartTime[4] = {0};  // Tracks when the button was first pressed
  static unsigned long lastActionTime[4] = {0}; // Tracks the last time an action was triggered
  int index = (pin == BUTTON_SPEED_UP_PIN) ? 0 : (pin == BUTTON_SPEED_DOWN_PIN) ? 1
                                             : (pin == BUTTON_MENU_PIN)         ? 2
                                                                                : 3;

  if (digitalRead(pin) == LOW)
  {
    if (display.isSleeping())
    {
      lastButtonPressTime = millis();
      display.wakeDisplay();
      while (digitalRead(pin) == LOW)
        if (currentMode == DisplayManager::PumpMode::PERISTALTIC)
        {
          pump.runPeristaltic();
        }
        else
        {
          pump.runDosing();
        }
      return false;
    }

    if (holdStartTime[index] == 0)
    {
      // Button just pressed
      holdStartTime[index] = millis();
      lastActionTime[index] = millis();
      return true;
    }
    else
    {
      unsigned long totalHoldTime = millis() - holdStartTime[index]; // Total time button has been held
      unsigned long interval = (totalHoldTime > 2000) ? 100 : 500;   // Reduce interval to 100ms after 2 seconds

      if (millis() - lastActionTime[index] >= interval)
      {
        lastActionTime[index] = millis(); // Update the last action time
        return true;
      }
    }
  }
  else
  {
    // Button released, reset timers
    holdStartTime[index] = 0;
    lastActionTime[index] = 0;
  }
  return false;
}