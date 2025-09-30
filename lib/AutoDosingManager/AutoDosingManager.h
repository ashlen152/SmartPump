#ifndef AUTO_DOSING_MANAGER_H
#define AUTO_DOSING_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "../PumpController/PumpController.h"
#include "../DisplayManager/DisplayManager.h"
#include <time.h>

#define DEBUG_AUTO_DOSING 1
// Debug logging control
#if DEBUG_AUTO_DOSING
#define AUTO_DOSING_LOG(format, ...) Serial.printf("[AutoDosing] " format "\n", ##__VA_ARGS__)
#else
#define AUTO_DOSING_LOG(format, ...)
#endif

struct DosingSchedule {
    float totalDailyVolume;      // Total volume for the day (e.g. 30ml)
    float dayVolume;             // 70% for day period
    float nightVolume;           // 30% for night period
    uint8_t dayStartHour;        // 11:00 AM
    uint8_t dayEndHour;          // 11:00 PM
    uint32_t lastDosingTime;     // Last dosing timestamp
    uint32_t nextDosingTime;     // Next scheduled dosing
    bool enabled;                // Auto-dosing enabled state
    float lastDoseVolume;        // Volume of last dose for logging
    uint32_t totalDosesDay;      // Count of doses during day period
    uint32_t totalDosesNight;    // Count of doses during night period
};

class AutoDosingManager {
public:
    struct Config {
        uint16_t enabledAddr;     // EEPROM address for enabled state
        uint16_t volumeAddr;      // EEPROM address for daily volume
        uint16_t lastTimeAddr;    // EEPROM address for last dosing time
        uint16_t totalDosedAddr;  // EEPROM address for total dosed volume
        float defaultVolume;      // Default daily volume in mL
    };

    AutoDosingManager(PumpController& pump, DisplayManager& display, const Config& config);
    void begin();
    void enable();
    void disable();
    void setDailyVolume(float volume);
    float getDailyVolume() const { return schedule.totalDailyVolume; }
    void updateSchedule();
    void checkAndDose();
    bool isEnabled() const { return schedule.enabled; }
    uint32_t getNextDosingTime() const { return schedule.nextDosingTime; }
    float getRemainingDailyVolume() const;
    void loadState();
    void saveState();
    
    // Debug functions
    void printStatus() const;
    void printSchedule() const;

private:
    float calculateDoseVolume(uint32_t currentTime);
    bool isInDayPeriod(uint32_t currentTime);
    uint32_t calculateNextDosingTime(uint32_t currentTime);
    bool performDosing(float volume);
    void resetDailyVolume();
    void logDosingEvent(float volume, bool success);
    
    PumpController& pump;
    DisplayManager& display;
    DosingSchedule schedule;
    float totalDosedVolume;      // Track total volume dosed today
    Config eepromConfig;         // EEPROM configuration
    
    static const uint8_t DOSES_PER_HOUR = 2;  // Number of doses per hour
    static const uint32_t DOSE_INTERVAL = (60) / DOSES_PER_HOUR; // Interval in ms
    static const uint8_t DAY_START_HOUR = 11;  // 11:00 AM
    static const uint8_t DAY_END_HOUR = 23;    // 11:00 PM
    static constexpr float DAY_VOLUME_RATIO = 0.7f;
    static constexpr float NIGHT_VOLUME_RATIO = 0.3f;
};

#endif