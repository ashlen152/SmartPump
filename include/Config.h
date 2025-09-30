#ifndef CONFIG_H
#define CONFIG_H

// WiFi credentials passed via build flags
#ifndef WIFI_SSID
#define WIFI_SSID "DefaultSSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "DefaultPassword"
#endif

const char *ssid = "SofaHome";
const char *password = "Sofa@123";

// Stepper Motor Pins
#define DIR_PIN 2
#define STEP_PIN 5
#define STEPPER_EN_PIN 26

// Display Pins and Settings
#define POT_PIN 34
// Display dimensions moved to DisplayManager.h

// Button Pins
#define BUTTON_ENABLE_PIN 25
#define BUTTON_SPEED_UP_PIN 35
#define BUTTON_SPEED_DOWN_PIN 34
#define BUTTON_MENU_PIN 14

// Stepper Settings
#define MAX_SPEED 100000
#define ACCELERATION 1000

// Display Timeout (ms)
#define DISPLAY_TIMEOUT 100000
// EEPROM Address
#define EEPROM_PERISTALTIC_STEPS_ADDR 0
#define EEPROM_DOSING_STEPS_ADDR (EEPROM_PERISTALTIC_STEPS_ADDR + sizeof(float))
#define EEPROM_SAVED_SPEED_ADDR (EEPROM_DOSING_STEPS_ADDR + sizeof(float))
#define EEPROM_MODE_ADDR (EEPROM_SAVED_SPEED_ADDR + sizeof(float))
#define EEPROM_AUTO_DOSING_ENABLED_ADDR (EEPROM_MODE_ADDR + sizeof(uint8_t))
#define EEPROM_DAILY_VOLUME_ADDR (EEPROM_AUTO_DOSING_ENABLED_ADDR + sizeof(bool))
#define EEPROM_LAST_DOSING_TIME_ADDR (EEPROM_DAILY_VOLUME_ADDR + sizeof(float))
#define EEPROM_TOTAL_DOSED_ADDR (EEPROM_LAST_DOSING_TIME_ADDR + sizeof(uint32_t))

// Debug Settings
#define DEBUG_AUTO_DOSING 1       // Enable auto-dosing debug logs
#define DEBUG_TIMESTAMP 1         // Include timestamps in debug logs

// Calibration Settings
#define CALIBRATE_PERISTALTIC_TIME 60  // seconds
#define CALIBRATE_DOSING_VOLUME 10.0f  // mL
#define CALIBRATE_SPEED 20000          // steps/sec

#define ID_PERISTALTIC_STEPPER "pump-1"
#define PUMP_SETTINGS_API "/api/pump-settings" // API endpoint for pump settings
#define PUMP_BY_ID_API "/api/pump-settings/getById" // API endpoint for get current settings

#define WIFI_RETRY_INTERVAL 5000         // ms
#define SYNC_INTERVAL 180000             // ms

#endif