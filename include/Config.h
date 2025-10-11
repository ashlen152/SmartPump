#pragma once
#include <Arduino.h>
// ======================================================
// Configuration Constants (modern constexpr version)
// ======================================================

// ==========================
// Stepper Motor Settings
// ==========================

namespace Config
{
    constexpr int STEPPER_EN_PIN = 26; // Stepper enable pin
    constexpr int DIR_PIN = 2;         // Direction pin
    constexpr int STEP_PIN = 5;        // Step pin

    constexpr float R_SENSE = 0.11f;      // Sense resistor for TMC2209
    constexpr uint8_t DRIVER_ADDR = 0b00; // Default address for TMC2209 driver

    constexpr float MAX_SPEED = 100000.0f;  // steps/sec
    constexpr float ACCELERATION = 1000.0f; // steps/sec²

    // ==========================
    // Display & Timing
    // ==========================
    constexpr unsigned long DISPLAY_TIMEOUT = 100000UL;           // ms
    constexpr unsigned long SETTINGS_DISPLAY_DURATION = 5000UL;   // ms
    constexpr unsigned long CALIBRATION_RESULT_DURATION = 3000UL; // ms

    // ==========================
    // EEPROM Address Map
    // ==========================
    constexpr int EEPROM_PERISTALTIC_STEPS_ADDR = 0;
    constexpr int EEPROM_DOSING_STEPS_ADDR = EEPROM_PERISTALTIC_STEPS_ADDR + sizeof(float);
    constexpr int EEPROM_SAVED_SPEED_ADDR = EEPROM_DOSING_STEPS_ADDR + sizeof(float);
    constexpr int EEPROM_MODE_ADDR = EEPROM_SAVED_SPEED_ADDR + sizeof(float);
    constexpr int EEPROM_AUTO_DOSING_ENABLED_ADDR = EEPROM_MODE_ADDR + sizeof(uint8_t);
    constexpr int EEPROM_DAILY_VOLUME_ADDR = EEPROM_AUTO_DOSING_ENABLED_ADDR + sizeof(bool);
    constexpr int EEPROM_LAST_DOSING_TIME_ADDR = EEPROM_DAILY_VOLUME_ADDR + sizeof(float);
    constexpr int EEPROM_TOTAL_DOSED_ADDR = EEPROM_LAST_DOSING_TIME_ADDR + sizeof(uint32_t);
    constexpr int EEPROM_ADDR = 0; // base EEPROM address (legacy)

    // ==========================
    // Debug Settings
    // ==========================
    constexpr bool DEBUG_AUTO_DOSING = true; // Enable auto-dosing debug logs
    constexpr bool DEBUG_TIMESTAMP = true;   // Include timestamps in debug logs

    // ==========================
    // Calibration Settings
    // ==========================
    constexpr unsigned int CALIBRATE_PERISTALTIC_TIME = 60; // seconds
    constexpr float CALIBRATE_DOSING_VOLUME = 10.0f;        // mL
    constexpr float CALIBRATE_SPEED = 20000.0f;             // steps/sec
    constexpr unsigned int CALIBRATE_TIME = 60;             // seconds (legacy)

    // ==========================
    // Auto Dosing Defaults
    // ==========================
    constexpr float DEFAULT_DAILY_VOLUME = 30.0f; // Default daily volume in mL

    // ==========================
    // Serial / Communication Pins
    // ==========================
    constexpr int RX_PIN = 16;
    constexpr int TX_PIN = 17;
}
