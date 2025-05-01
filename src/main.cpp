#include <Arduino.h>
#include <DisplayManager.h>
#include <WiFiManager.h>
#include <EEPROM.h>
#include <Config.h>
#include <ArduinoJson.h>
#include "PumpController.h"

#define EN_PIN 26  // Enable
#define DIR_PIN 2  // Direction
#define STEP_PIN 5 // Step
#define RX_PIN 16  // ESP32 RX orange line
#define TX_PIN 17  // ESP32 TX blue line

#define R_SENSE 0.11f
#define DRIVER_ADDR 0b00 // Default address for TMC2209

#define DISPLAY_TIMEOUT 100000
#define EEPROM_ADDR 0
#define CALIBRATE_TIME 60     // seconds
#define CALIBRATE_SPEED 20000 // steps/sec

#define SETTINGS_DISPLAY_DURATION 5000 // ms

#define CALIBRATION_RESULT_DURATION 3000 // ms

// State
bool inMenu = false;
int menuIndex = 0;
unsigned long lastButtonPressTime = 0;
float stepsPerML = 0;
int stepsPerSecond = 2000;
const char *menuItems[] = {"Calibrate Drop", "Settings Info", "Save Speed"};
const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);
unsigned long lastWiFiRetryTime = 0;
unsigned long lastSyncTime = 0;
unsigned long lastSettingsDisplayTime = 0;
unsigned long lastCalibrationResultTime = 0;
bool showingSettings = false;
bool showingCalibrationResult = false;

// Create WiFiManager instance
WiFiManager wifi(ssid, password);
DisplayManager &display = DisplayManager::getInstance();
PumpController pump(&Serial2, STEP_PIN, DIR_PIN, EN_PIN, R_SENSE, DRIVER_ADDR);

// Forward declarations
bool checkButtonPress(uint8_t pin);
bool checkButtonPressOrHold(uint8_t pin);
void calibrateDrop();
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

  pinMode(BUTTON_ENABLE_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_UP_PIN, INPUT_PULLUP);
  pinMode(BUTTON_SPEED_DOWN_PIN, INPUT_PULLUP);
  pinMode(BUTTON_MENU_PIN, INPUT_PULLUP);

  display.begin();
  pump.begin();

  // Load stepsPerML and speed from EEPROM
  EEPROM.get(EEPROM_ADDR, stepsPerML);
  float savedSpeed = 0;
  EEPROM.get(EEPROM_ADDR + sizeof(stepsPerML), savedSpeed);

  if (isnan(stepsPerML) || stepsPerML <= 0)
    stepsPerML = 0;
  stepsPerSecond = stepsPerML > 0 ? (int)(stepsPerML / 60) : 2000;
  pump.setStepsPerML(stepsPerML);
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
  // WiFi Connection Handling
  if (!wifi.isConnected())
  {
    if (currentTime - lastWiFiRetryTime >= WIFI_RETRY_INTERVAL)
    {
      Serial.println("Attempting WiFi Connect...");
      if (wifi.connect())
      {
        display.setSignalStrength(wifi.getSignalStrength());
        display.showText("WiFi Connected");
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
                display.updateStatus(pump.isEnabled(), currentSpeed > 0 ? currentSpeed / pump.getSpeedStep() : 0);
              }
              else
              {
                Serial.println("Response missing 'currentSpeed' field");
                display.showText("Invalid Server Data");
              }
            }

            display.showText("Server OK");
            display.updateStatus(pump.isEnabled(), pump.getSpeed() > 0 ? pump.getSpeed() / pump.getSpeedStep() : 0);
          }
        }
        else
        {
          display.showText("Server Down");
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

  if (checkButtonPress(BUTTON_MENU_PIN))
  {
    if (!inMenu)
    {
      inMenu = true;
      menuIndex = 0;
      display.showMenu(menuIndex, menuItems, menuItemCount);
    }
    else
    {
      runMenuSelection();
    }
  }

  if (inMenu)
  {
    if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
    {
      menuIndex = (menuIndex + 1) % menuItemCount;
    }
    if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
    {
      menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
    }
    display.showMenu(menuIndex, menuItems, menuItemCount);
  }
  else if (!showingSettings && !showingCalibrationResult)
  {
    if (checkButtonPress(BUTTON_ENABLE_PIN))
    {
      if (pump.isEnabled())
        pump.stop();
      else
        pump.setSpeed(pump.getSpeed() > 0 ? pump.getSpeed() : 0);
      display.updateStatus(pump.isEnabled(), pump.getStepsPerML() > 0 ? pump.getSpeed() / pump.getStepsPerML() * 60 : 0);
    }

    if (checkButtonPressOrHold(BUTTON_SPEED_UP_PIN))
    {
      Serial.println("getSpeedStep: " + String(pump.getSpeedStep()));
      Serial.println("getMaxSpeedStep: " + String(pump.getMaxSpeedStep()));
      Serial.println("getSpeed: " + String(pump.getSpeed()));
      Serial.println("getStepsPerML: " + String(pump.getStepsPerML()));
      pump.setSpeed(pump.getSpeed() + pump.getSpeedStep());
      display.updateStatus(pump.isEnabled(), pump.getSpeed() > 0 ? pump.getSpeed() / pump.getSpeedStep() : 0);
    }

    if (checkButtonPressOrHold(BUTTON_SPEED_DOWN_PIN))
    {
      pump.setSpeed(max(pump.getSpeed() - pump.getSpeedStep(), 0.0f));
      display.updateStatus(pump.isEnabled(), pump.getSpeed() > 0 ? pump.getSpeed() / pump.getSpeedStep() : 0);
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
    display.updateStatus(pump.isEnabled(), stepsPerML > 0 ? pump.getSpeed() / stepsPerML * 60 : 0);
  }

  // Handle Calibration Result Timeout
  if (showingCalibrationResult && currentTime - lastCalibrationResultTime >= CALIBRATION_RESULT_DURATION)
  {
    showingCalibrationResult = false;
    display.updateStatus(pump.isEnabled(), stepsPerML > 0 ? pump.getSpeed() / stepsPerML * 60 : 0);
  }

  pump.run();
  display.updateStatus(pump.isEnabled(), stepsPerML > 0 ? pump.getSpeed() / stepsPerML * 60 : 0);
}

void syncData()
{
  JsonDocument doc;

  int rssi = wifi.getSignalStrength();

  doc["pumpId"] = ID_PERISTALTIC_STEPPER;
  doc["stepsPerML"] = stepsPerML;
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

void runMenuSelection()
{
  if (menuIndex == 0)
  {
    calibrateDrop();
  }
  else if (menuIndex == 1)
  {
    display.showSettingsInfo(pump.getSpeed(), pump.getStepsPerML(), pump.getSpeedStep());
    showingSettings = true;
    lastSettingsDisplayTime = millis();
  }
  else if (menuIndex == 2) // Save Speed
  {
    float currentSpeed = pump.getSpeed();
    EEPROM.put(EEPROM_ADDR + sizeof(stepsPerML), currentSpeed); // Store speed after stepsPerML
    EEPROM.commit();
    Serial.print("Saved speed to EEPROM: ");
    Serial.println(currentSpeed);
    display.showText("Speed Saved!");
    delay(1000); // Show confirmation message briefly
  }
  inMenu = false;
}

void calibrateDrop()
{
  display.showCalibrationStart(CALIBRATE_TIME);
  pump.setSpeed(CALIBRATE_SPEED);
  unsigned long startTime = millis();
  unsigned long runDuration = CALIBRATE_TIME * 1000UL;
  unsigned long lastUpdate = 0;
  while (millis() - startTime < runDuration)
  {
    pump.run();
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

  stepsPerML = ml > 0 ? (float)CALIBRATE_SPEED / ml : 0;
  stepsPerSecond = stepsPerML > 0 ? (int)(stepsPerML / 60) : 2000;
  pump.setStepsPerML(stepsPerML);
  pump.setSpeedStep(stepsPerSecond);
  EEPROM.put(EEPROM_ADDR, stepsPerML);
  EEPROM.commit();
  display.showCalibrationResult(stepsPerML, stepsPerSecond);
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
        pump.run();
      }
      return false;
    }
    while (digitalRead(pin) == LOW)
    {
      pump.run();
    }
    return true;
  }
  return false;
}

bool checkButtonPressOrHold(uint8_t pin)
{
  static unsigned long holdStartTime[4] = {0}; // Tracks when the button was first pressed
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
        pump.run();
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