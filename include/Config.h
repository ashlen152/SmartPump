#ifndef CONFIG_H
#define CONFIG_H

// Stepper Motor Pins
#define DIR_PIN 2
#define STEP_PIN 5
#define STEPPER_EN_PIN 26

// Display Pins and Settings
#define POT_PIN 34
// Display dimensions moved to DisplayManager.h

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
#define DEBUG_AUTO_DOSING 1 // Enable auto-dosing debug logs
#define DEBUG_TIMESTAMP 1   // Include timestamps in debug logs

// Calibration Settings
#define CALIBRATE_PERISTALTIC_TIME 60 // seconds
#define CALIBRATE_DOSING_VOLUME 10    // mL
#define CALIBRATE_SPEED 20000         // steps/sec

// Auto-dosing EEPROM addresses
#define EEPROM_AUTO_DOSING_ENABLED_ADDR (EEPROM_MODE_ADDR + sizeof(uint8_t))
#define EEPROM_DAILY_VOLUME_ADDR (EEPROM_AUTO_DOSING_ENABLED_ADDR + sizeof(bool))
#define EEPROM_LAST_DOSING_TIME_ADDR (EEPROM_DAILY_VOLUME_ADDR + sizeof(float))
#define EEPROM_TOTAL_DOSED_ADDR (EEPROM_LAST_DOSING_TIME_ADDR + sizeof(uint32_t))

// Auto dosing defaults
#define DEFAULT_DAILY_VOLUME 30.0f // Default daily volume in mL

#define EN_PIN 26  // Enable
#define DIR_PIN 2  // Direction
#define STEP_PIN 5 // Step

constexpr const int RX_PIN = 16;
constexpr const int TX_PIN = 17;

#define R_SENSE 0.11f
#define DRIVER_ADDR 0b00 // Default address for TMC2209

#define DISPLAY_TIMEOUT 100000
#define EEPROM_ADDR 0

// Auto dosing defaults
#define CALIBRATE_TIME 60     // seconds
#define CALIBRATE_SPEED 20000 // steps/sec

#define SETTINGS_DISPLAY_DURATION 5000   // ms
#define CALIBRATION_RESULT_DURATION 3000 // ms

// extern char const* ssid;
// extern char const* password;
// extern char* ID_PERISTALTIC_STEPPER;
// extern char* PUMP_SETTINGS_API;
// extern char* PUMP_BY_ID_API;
// extern int WIFI_RETRY_INTERVAL;
// extern int SYNC_INTERVAL;

#endif
