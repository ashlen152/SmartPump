#ifndef CONFIG_H
#define CONFIG_H

// Stepper Motor Pins
#define DIR_PIN 2
#define STEP_PIN 5
#define STEPPER_EN_PIN 26

// Display Pins and Settings
#define POT_PIN 34
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Button Pins
#define BUTTON_ENABLE_PIN 25
#define BUTTON_SPEED_UP_PIN 34
#define BUTTON_SPEED_DOWN_PIN 35
#define BUTTON_MENU_PIN 14

// Stepper Settings
#define MAX_SPEED 100000
#define ACCELERATION 1000

// Display Timeout (ms)
#define DISPLAY_TIMEOUT 100000

// EEPROM Address
#define EEPROM_ADDR 0

// Calibration Settings
#define CALIBRATE_TIME 60 // in seconds
#define CALIBRATE_SPEED 20000

#define ID_PERISTALTIC_STEPPER "pump-1"
#define PUMP_SETTINGS_API "/api/pump-settings" // API endpoint for pump settings
#define PUMP_BY_ID_API "/api/pump-settings/getById" // API endpoint for get current settings

#define WIFI_RETRY_INTERVAL 5000         // ms
#define SYNC_INTERVAL 180000             // ms

#endif