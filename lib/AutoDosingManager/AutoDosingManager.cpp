#include "AutoDosingManager.h"
#include <EEPROM.h>
#include <time.h>

static time_t toTimeT(uint32_t t) {
    return static_cast<time_t>(t);
}

AutoDosingManager::AutoDosingManager(PumpController& p, DisplayManager& d, const Config& config)
    : pump(p), display(d), totalDosedVolume(0), eepromConfig(config) {
    schedule.enabled = true;
    schedule.totalDailyVolume = config.defaultVolume;
    schedule.dayStartHour = 11;  // 11:00 AM
    schedule.dayEndHour = 23;    // 11:00 PM
    schedule.lastDosingTime = 0;
    schedule.nextDosingTime = 0;
    schedule.lastDoseVolume = 0;
    schedule.totalDosesDay = 0;
    schedule.totalDosesNight = 0;
}

void AutoDosingManager::begin() {
    AUTO_DOSING_LOG("Initializing Auto Dosing Manager");
    loadState();
    
    // Get current time and ensure it's valid (after Jan 1, 2023)
    time_t now = time(nullptr);
    const time_t MIN_VALID_TIME = 1672531200; // Jan 1, 2023
    
    if (now < MIN_VALID_TIME) {
        AUTO_DOSING_LOG("WARNING: Current time not valid, waiting for time sync");
        schedule.enabled = false;
        return;
    }
    
    if (schedule.enabled) {
        // Check if we need to reset daily totals
        struct tm timeInfo;
        localtime_r(&now, &timeInfo);
        time_t lastTime = static_cast<time_t>(schedule.lastDosingTime);
        struct tm lastTimeInfo;
        localtime_r(&lastTime, &lastTimeInfo);
        
        // If first time running, initialize with a last dose time
        if (schedule.lastDosingTime == 0) {
            schedule.lastDosingTime = now - (DOSE_INTERVAL); // Set last dose to 30 minutes ago
            lastTime = schedule.lastDosingTime;
            localtime_r(&lastTime, &lastTimeInfo);
            resetDailyVolume();
            AUTO_DOSING_LOG("First run - Initializing last dose time to: %lu", schedule.lastDosingTime);
        }
        // Check if we need to reset for a new day
        else if (timeInfo.tm_yday != lastTimeInfo.tm_yday || 
                 timeInfo.tm_year != lastTimeInfo.tm_year) {
            AUTO_DOSING_LOG("New day detected - resetting daily volumes");
            resetDailyVolume();
        } else {
            AUTO_DOSING_LOG("Continuing from last state: %.2f ml dosed today", totalDosedVolume);
        }
        
        // Calculate next dosing time based on current time
        schedule.nextDosingTime = calculateNextDosingTime(now);
        printSchedule();
    }
}

void AutoDosingManager::enable() {
    AUTO_DOSING_LOG("Enabling auto dosing");
    schedule.enabled = true;
    resetDailyVolume();
    schedule.nextDosingTime = calculateNextDosingTime(time(nullptr));
    printSchedule();
}

void AutoDosingManager::disable() {
    AUTO_DOSING_LOG("Disabling auto dosing");
    schedule.enabled = false;
}

void AutoDosingManager::setDailyVolume(float volume) {
    AUTO_DOSING_LOG("Setting daily volume to %.2f ml", volume);
    schedule.totalDailyVolume = volume;
    resetDailyVolume();
    saveState();
    printSchedule();
}

float AutoDosingManager::getRemainingDailyVolume() const {
    return schedule.totalDailyVolume - totalDosedVolume;
}

void AutoDosingManager::updateSchedule() {
    if (!schedule.enabled) return;
    
    time_t now = time(nullptr);
    struct tm timeInfo;
    localtime_r(&now, &timeInfo);
    
    // Reset at midnight
    if (timeInfo.tm_hour == 0 && timeInfo.tm_min == 0) {
        AUTO_DOSING_LOG("Midnight reset - Starting new day");
        resetDailyVolume();
        schedule.nextDosingTime = calculateNextDosingTime(now);
        schedule.totalDosesDay = 0;
        schedule.totalDosesNight = 0;
        printSchedule();
    }
}

float AutoDosingManager::calculateDoseVolume(uint32_t currentTime) {
    float baseVolume;
    // Calculate doses per day based on DOSES_PER_HOUR
    float dosesPerDay = DOSES_PER_HOUR * 24.0f;
    
    if (isInDayPeriod(currentTime)) {
        baseVolume = schedule.dayVolume / dosesPerDay;
        AUTO_DOSING_LOG("Day period dose: %.2f ml", baseVolume);
    } else {
        baseVolume = schedule.nightVolume / dosesPerDay;
        AUTO_DOSING_LOG("Night period dose: %.2f ml", baseVolume);
    }
    return baseVolume;
}

void AutoDosingManager::checkAndDose() {
    if (!schedule.enabled) return;
    
    time_t now = time(nullptr);
    const time_t MIN_VALID_TIME = 1672531200; // Jan 1, 2023
    
    // Ensure time is valid
    if (now < MIN_VALID_TIME) {
        return;
    }

    
    // If we're still moving from last dose, skip
    if (pump.isMoving()) return;
    
    // Prevent checking too frequently (at least 1 second between checks)
    static uint32_t lastCheckTime = 0;
    if (now - lastCheckTime < 1) return;
    lastCheckTime = now;
    
    // If this is first run after power cycle, initialize with previous last dose time
    if (schedule.lastDosingTime == 0) {
        schedule.lastDosingTime = now - (DOSE_INTERVAL); // Set last dose to 30 minutes ago
        schedule.nextDosingTime = calculateNextDosingTime(now);
        AUTO_DOSING_LOG("Initialized last dose time to: %lu", schedule.lastDosingTime);
    }
    
    // Check if we need to reset daily totals
    struct tm currentTime;
    localtime_r(&now, &currentTime);
    time_t lastTime = static_cast<time_t>(schedule.lastDosingTime);
    struct tm lastDoseTime;
    localtime_r(&lastTime, &lastDoseTime);
    
    // Reset if it's a new day
    if (currentTime.tm_yday != lastDoseTime.tm_yday || 
        currentTime.tm_year != lastDoseTime.tm_year) {
        AUTO_DOSING_LOG("New day detected - resetting daily volumes");
        resetDailyVolume();
        schedule.nextDosingTime = calculateNextDosingTime(now);
        return;
    }
    
    if (static_cast<uint32_t>(now) >= schedule.nextDosingTime) {
        float volume = calculateDoseVolume(static_cast<uint32_t>(now));
        
        // Only proceed if we can dose this volume
        if (totalDosedVolume + volume <= schedule.totalDailyVolume) {
            bool dosed = performDosing(volume);
            if (dosed) {
                schedule.lastDosingTime = now;
                schedule.nextDosingTime = calculateNextDosingTime(now);
                
                AUTO_DOSING_LOG("Next dose scheduled for: %lu", schedule.nextDosingTime);
            }
        }
    }
}

bool AutoDosingManager::performDosing(float volume) {
    // Check if we would exceed daily volume
    if (totalDosedVolume + volume > schedule.totalDailyVolume) {
        AUTO_DOSING_LOG("WARNING: Would exceed daily volume limit - skipping dose");
        return false;
    }
    
    AUTO_DOSING_LOG("Performing dose of %.2f ml", volume);
    AUTO_DOSING_LOG("Total dosed today: %.2f ml of %.2f ml", 
                   totalDosedVolume, schedule.totalDailyVolume);
    
    // Update display
    display.showText("Auto Dosing...");
    
    // Perform the dosing
    pump.moveML(volume);
  
    // Update counters only if dose completed successfully
    totalDosedVolume += volume;
    schedule.lastDoseVolume = volume;
    if (isInDayPeriod(time(nullptr))) {
        schedule.totalDosesDay++;
    } else {
        schedule.totalDosesNight++;
    } 
     
    logDosingEvent(volume, true);
    return true;
}

void AutoDosingManager::resetDailyVolume() {
    AUTO_DOSING_LOG("Resetting daily volumes");
    totalDosedVolume = 0;
    schedule.dayVolume = schedule.totalDailyVolume * DAY_VOLUME_RATIO;
    schedule.nightVolume = schedule.totalDailyVolume * NIGHT_VOLUME_RATIO;
}

bool AutoDosingManager::isInDayPeriod(uint32_t currentTime) {
    struct tm timeInfo;
    time_t t = toTimeT(currentTime);
    localtime_r(&t, &timeInfo);
    return (timeInfo.tm_hour >= schedule.dayStartHour && 
            timeInfo.tm_hour < schedule.dayEndHour);
}

uint32_t AutoDosingManager::calculateNextDosingTime(uint32_t currentTime) {
    // Round up to next 30-minute interval
    const uint32_t INTERVAL = DOSE_INTERVAL; // 30 minutes in seconds
    uint32_t nextTime = ((currentTime + INTERVAL - 1) / INTERVAL) * INTERVAL;
    
    struct tm timeInfo;
    time_t t = toTimeT(nextTime);
    localtime_r(&t, &timeInfo);
    
    // Check if next time would go into tomorrow
    // Only move to next day if we're past the last dosing time of the current day
    if (timeInfo.tm_hour > schedule.dayEndHour || 
        (timeInfo.tm_hour == schedule.dayEndHour && 
         timeInfo.tm_min >= 60 - (60 / DOSES_PER_HOUR))) {
        // Move to next day's start
        timeInfo.tm_hour = 0;
        timeInfo.tm_min = 0;
        timeInfo.tm_mday += 1;  // Move to next day
        nextTime = mktime(&timeInfo);
    }
    
    AUTO_DOSING_LOG("Next dosing time calculated: %lu", nextTime);
    return nextTime;
}

void AutoDosingManager::loadState() {
    AUTO_DOSING_LOG("Loading state from EEPROM");
    
    bool enabled = false;
    float storedDailyVolume = eepromConfig.defaultVolume;
    uint32_t lastDoseTime = 0;
    float storedTotalDosed = 0;
    
    EEPROM.get(eepromConfig.enabledAddr, enabled);
    EEPROM.get(eepromConfig.volumeAddr, storedDailyVolume);
    EEPROM.get(eepromConfig.lastTimeAddr, lastDoseTime);
    EEPROM.get(eepromConfig.totalDosedAddr, storedTotalDosed);
    
    // Set values with validation
    schedule.enabled = enabled;
    schedule.totalDailyVolume = (!isnan(storedDailyVolume) && storedDailyVolume > 0) ? storedDailyVolume : eepromConfig.defaultVolume;
    schedule.lastDosingTime = lastDoseTime;
    totalDosedVolume = (!isnan(storedTotalDosed) && storedTotalDosed >= 0) ? storedTotalDosed : 0;
    
    AUTO_DOSING_LOG("Loaded: enabled=%d, daily=%.2f, last=%lu, total=%.2f",
                    schedule.enabled, schedule.totalDailyVolume,
                    schedule.lastDosingTime, totalDosedVolume);
}

void AutoDosingManager::saveState() {
    AUTO_DOSING_LOG("Saving state to EEPROM");
    
    EEPROM.put(eepromConfig.enabledAddr, schedule.enabled);
    EEPROM.put(eepromConfig.volumeAddr, schedule.totalDailyVolume);
    EEPROM.put(eepromConfig.lastTimeAddr, schedule.lastDosingTime);
    EEPROM.put(eepromConfig.totalDosedAddr, totalDosedVolume);
    EEPROM.commit();
}

void AutoDosingManager::printStatus() const {
    AUTO_DOSING_LOG("=== Auto Dosing Status ===");
    AUTO_DOSING_LOG("Enabled: %d", schedule.enabled);
    AUTO_DOSING_LOG("Daily Volume: %.2f ml", schedule.totalDailyVolume);
    AUTO_DOSING_LOG("Total Dosed Today: %.2f ml", totalDosedVolume);
    AUTO_DOSING_LOG("Last Dose: %.2f ml", schedule.lastDoseVolume);
    AUTO_DOSING_LOG("Day Doses: %lu", schedule.totalDosesDay);
    AUTO_DOSING_LOG("Night Doses: %lu", schedule.totalDosesNight);
    AUTO_DOSING_LOG("Next Dose Time: %lu", schedule.nextDosingTime);
}

void AutoDosingManager::printSchedule() const {
    AUTO_DOSING_LOG("=== Dosing Schedule ===");
    AUTO_DOSING_LOG("Total Daily: %.2f ml", schedule.totalDailyVolume);
    AUTO_DOSING_LOG("Day Period (%.2f%%): %.2f ml", DAY_VOLUME_RATIO * 100, schedule.dayVolume);
    AUTO_DOSING_LOG("Night Period (%.2f%%): %.2f ml", NIGHT_VOLUME_RATIO * 100, schedule.nightVolume);
    AUTO_DOSING_LOG("Individual Day Dose: %.2f ml", schedule.dayVolume / 24.0f);
    AUTO_DOSING_LOG("Individual Night Dose: %.2f ml", schedule.nightVolume / 24.0f);
}

void AutoDosingManager::logDosingEvent(float volume, bool success) {
    AUTO_DOSING_LOG("=== Dosing Event ===");
    AUTO_DOSING_LOG("Time: %lu", time(nullptr));
    AUTO_DOSING_LOG("Volume: %.2f ml", volume);
    AUTO_DOSING_LOG("Success: %d", success);
    AUTO_DOSING_LOG("Total Today: %.2f ml", totalDosedVolume);
    AUTO_DOSING_LOG("Remaining: %.2f ml", getRemainingDailyVolume());
    AUTO_DOSING_LOG("Period: %s", isInDayPeriod(time(nullptr)) ? "Day" : "Night");
}