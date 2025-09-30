#include <vector>
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



struct DoseSchedule {
    int hour;
    int minute;
    float ml;
    bool completed;
};

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
    float getDailyVolume() const { return scheduleMeta.totalDailyVolume; }
    void updateSchedule();
    void checkAndDose();
    bool isEnabled() const { return scheduleMeta.enabled; }
    uint32_t getNextDosingTime() const { return scheduleMeta.nextDosingTime; }
    float getRemainingDailyVolume() const;
    void loadState();
    void saveState();
    
    // Debug functions
    void printStatus() const;
    void printSchedule() const;

private:
    void generateWeightedSchedule(int slots, float totalMl, int startHour, int endHour, float percent1, float percent2);
    void resetDailyVolume();
    bool performDosing(float volume);
    void logDosingEvent(float volume, bool success);
    PumpController& pump;
    DisplayManager& display;
    DosingSchedule scheduleMeta;
    float totalDosedVolume;
    Config eepromConfig;
    std::vector<DoseSchedule> schedule;
    int slots = 48; // Default slots per day
    int startHour = 11;
    int endHour = 23;
    float percent1 = 0.6f;
    float percent2 = 0.4f;
};

#endif