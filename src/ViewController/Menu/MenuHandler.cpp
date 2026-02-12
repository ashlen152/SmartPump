#include "MenuHandler.h"
#include <ButtonConfig.h>
#include "ButtonController/ButtonController.h"
#include <DisplayManager.h>
#include <PumpController.h>
#include <AutoDosingManager.h>
#include <ConfigManager.h>
#include <time.h>

static int menuIndex = 0;
static bool showingSettings = false;
static const char *menuItems[] = {"Dosing Cal", "Settings Info", "Auto Dosing", "Set Daily Vol", "Day Period", "Day/Night %", "Set Pump ID", "Reset Config", "Pause Dosing", "Resume Dosing", "Dose History", "Speed Profile", "Edit Profiles"};

void runMenuSelection()
{
  DisplayManager &display = DisplayManager::getInstance();
  PumpController &pump = PumpController::getInstance();

  switch (menuIndex)
  {
  case 0: // Dosing Calibration
    // calibrateDosing();
    display.setState(DisplayManager::DisplayState::CALIBRATE_BEGIN);
    break;
  case 1: // Settings Info
    display.setContextSettings(pump.getSpeed(), pump.getDosingStepsPerML(), pump.getSpeedStep());
    display.setState(DisplayManager::DisplayState::SETTINGS);
    break;
  case 2: // Auto Dosing
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    if (autoDosing.isEnabled()) {
      autoDosing.disable();
      display.showText("Auto Dosing\nDisabled");
    } else {
      autoDosing.enable();
      display.showText("Auto Dosing\nEnabled");
    }
    delay(1500);
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 3: // Set Daily Volume
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    float volume = autoDosing.getDailyVolume();
    bool setting = true;
    
    while (setting) {
      display.showValue("Daily Vol (mL)", volume);
      
      if (pressButtonUp()) {
        volume += 1.0f;
        if (volume > 200.0f) volume = 200.0f;  // Max 200mL per day
      }
      if (pressButtonDown()) {
        volume = max(volume - 1.0f, 1.0f);  // Min 1mL
      }
      if (pressButtonEnable()) {
        autoDosing.setDailyVolume(volume);
        char msg[32];
        snprintf(msg, sizeof(msg), "Volume Saved\n%.0f mL", volume);
        display.showText(msg);
        delay(1000);
        setting = false;
      }
      if (pressButtonMenu()) {
        display.showText("Cancelled");
        delay(500);
        setting = false;  // Cancel without saving
      }
      
      delay(100);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 4: // Day Period (start/end hour)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    uint8_t startH = autoDosing.getDayStartHour();
    uint8_t endH = autoDosing.getDayEndHour();
    bool settingStart = true;  // true = setting start, false = setting end
    bool setting = true;
    
    while (setting) {
      char label[32];
      if (settingStart) {
        snprintf(label, sizeof(label), "Day Start Hour");
        display.showValue(label, (float)startH);
      } else {
        snprintf(label, sizeof(label), "Day End Hour");
        display.showValue(label, (float)endH);
      }
      
      if (pressButtonUp()) {
        if (settingStart) {
          startH = (startH + 1) % 24;
        } else {
          endH = (endH + 1) % 24;
        }
      }
      if (pressButtonDown()) {
        if (settingStart) {
          startH = (startH == 0) ? 23 : startH - 1;
        } else {
          endH = (endH == 0) ? 23 : endH - 1;
        }
      }
      if (pressButtonMenu()) {
        if (settingStart) {
          settingStart = false;  // Switch to setting end hour
          display.showText("Now set end hour");
          delay(800);
        } else {
          // Both set, now save
          autoDosing.setDayPeriod(startH, endH);
          char msg[32];
          snprintf(msg, sizeof(msg), "Period Saved\n%02d:00-%02d:00", startH, endH);
          display.showText(msg);
          delay(1500);
          setting = false;
        }
      }
      if (pressButtonEnable()) {
        display.showText("Cancelled");
        delay(500);
        setting = false;  // Cancel without saving
      }
      
      delay(100);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 5: // Day/Night Split %
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    uint8_t dayPercent = autoDosing.getDayPercent();
    bool setting = true;
    
    while (setting) {
      char label[32];
      uint8_t nightPercent = 100 - dayPercent;
      snprintf(label, sizeof(label), "Day:%d%% Night:%d%%", dayPercent, nightPercent);
      display.showText(label);
      
      if (pressButtonUp()) {
        dayPercent += 5;
        if (dayPercent > 100) dayPercent = 100;
      }
      if (pressButtonDown()) {
        if (dayPercent >= 5) dayPercent -= 5;
        else dayPercent = 0;
      }
      if (pressButtonEnable()) {
        autoDosing.setDayNightSplit(dayPercent);
        char msg[32];
        snprintf(msg, sizeof(msg), "Split Saved\n%d%% / %d%%", dayPercent, 100 - dayPercent);
        display.showText(msg);
        delay(1000);
        setting = false;
      }
      if (pressButtonMenu()) {
        display.showText("Cancelled");
        delay(500);
        setting = false;  // Cancel without saving
      }
      
      delay(100);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 6: // Set Pump ID
  {
    ConfigManager &config = ConfigManager::getInstance();
    char newId[16];
    strncpy(newId, config.getPumpId(), sizeof(newId) - 1);
    newId[15] = '\0';
    
    int cursorPos = 0;
    int idLen = strlen(newId);
    bool editing = true;
    const char validChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    
    while (editing) {
      // Show current ID with cursor
      char displayBuf[32];
      snprintf(displayBuf, sizeof(displayBuf), "ID:%s\nPos:%d", newId, cursorPos + 1);
      display.showText(displayBuf);
      
      if (pressButtonUp()) {
        // Cycle through valid characters
        if (cursorPos < idLen) {
          char current = newId[cursorPos];
          const char *pos = strchr(validChars, current);
          if (pos && *(pos + 1) != '\0') {
            newId[cursorPos] = *(pos + 1);
          } else {
            newId[cursorPos] = validChars[0];  // Wrap to 'A'
          }
        } else if (idLen < 15) {
          // Add new character
          newId[idLen] = 'A';
          newId[idLen + 1] = '\0';
          idLen++;
        }
      }
      
      if (pressButtonDown()) {
        // Move cursor or delete character
        if (idLen > 0 && cursorPos == idLen - 1) {
          // Delete last character
          newId[idLen - 1] = '\0';
          idLen--;
          if (cursorPos > 0) cursorPos--;
        }
      }
      
      if (pressButtonMenu()) {
        // Move cursor right (or save if at end)
        if (cursorPos < idLen - 1) {
          cursorPos++;
        } else {
          // Save and exit
          if (config.setPumpId(newId)) {
            display.showText("Pump ID Saved");
          } else {
            display.showText("Invalid ID");
          }
          delay(1000);
          editing = false;
        }
      }
      
      if (pressButtonEnable()) {
        display.showText("Cancelled");
        delay(500);
        editing = false;
      }
      
      delay(150);  // Debounce
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 7: // Reset Config (Factory Reset)
  {
    display.showText("Factory Reset?\nMenu=YES\nEnable=NO");
    delay(100);
    
    bool waiting = true;
    while (waiting) {
      if (pressButtonMenu()) {
        display.showText("Resetting...");
        delay(500);
        
        ConfigManager &config = ConfigManager::getInstance();
        config.resetToDefaults();
        
        display.showText("Reset Complete\nRestarting...");
        delay(2000);
        ESP.restart();  // Restart ESP32
        waiting = false;
      }
      if (pressButtonEnable()) {
        display.showText("Cancelled");
        delay(500);
        waiting = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 8: // Pause Dosing (Phase 3 Sprint 5)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    
    const char* pauseOptions[] = {"1 Hour", "6 Hours", "12 Hours", "24 Hours", "Indefinite"};
    const uint32_t pauseDurations[] = {3600, 21600, 43200, 86400, 0}; // seconds (0 = indefinite)
    int pauseIndex = 0;
    const int pauseOptionCount = 5;
    
    bool selecting = true;
    while (selecting) {
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Pause:\n%s\nMenu=OK Enable=Cancel", pauseOptions[pauseIndex]);
      display.showText(buffer);
      delay(100);
      
      if (pressButtonUp()) {
        pauseIndex = (pauseIndex == 0) ? pauseOptionCount - 1 : pauseIndex - 1;
      }
      if (pressButtonDown()) {
        pauseIndex = (pauseIndex + 1) % pauseOptionCount;
      }
      if (pressButtonMenu()) {
        autoDosing.pause(pauseDurations[pauseIndex]);
        display.showText("Paused");
        delay(1000);
        selecting = false;
      }
      if (pressButtonEnable()) {
        display.showText("Cancelled");
        delay(500);
        selecting = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 9: // Resume Dosing (Phase 3 Sprint 5)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    
    if (!autoDosing.isPaused()) {
      display.showText("Not Paused");
      delay(1000);
    } else {
      display.showText("Resume?\nMenu=YES\nEnable=NO");
      delay(100);
      
      bool waiting = true;
      while (waiting) {
        if (pressButtonMenu()) {
          autoDosing.resume();
          display.showText("Resumed");
          delay(1000);
          waiting = false;
        }
        if (pressButtonEnable()) {
          display.showText("Cancelled");
          delay(500);
          waiting = false;
        }
        delay(100);
      }
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 10: // Dose History (Phase 3 Sprint 6)
  {
    AutoDosingManager &autoDosing = AutoDosingManager::getInstance();
    uint8_t count = 0;
    const DoseHistoryEntry* history = autoDosing.getDoseHistory(count);
    
    // Format history for display
    char historyText[256];
    int offset = snprintf(historyText, sizeof(historyText), "=Dose History=\n");
    
    if (count == 0) {
      offset += snprintf(historyText + offset, sizeof(historyText) - offset, 
                         "\nNo doses yet");
    } else {
      for (uint8_t i = 0; i < count && i < 5; i++) {
        time_t t = history[i].timestamp;
        struct tm* timeinfo = localtime(&t);
        
        // Format: MM/DD HH:MM 0.5mL OK
        offset += snprintf(historyText + offset, sizeof(historyText) - offset,
                           "\n%02d/%02d %02d:%02d %.1fmL %s",
                           timeinfo->tm_mon + 1, timeinfo->tm_mday,
                           timeinfo->tm_hour, timeinfo->tm_min,
                           history[i].volume,
                           history[i].success ? "OK" : "X");
      }
    }
    
    display.showText(historyText);
    delay(100);
    
    bool viewing = true;
    while (viewing) {
      if (pressButtonEnable() || pressButtonMenu()) {
        viewing = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 11: // Speed Profile (Phase 3 Sprint 7)
  {
    PumpController &pump = PumpController::getInstance();
    uint8_t profile = pump.getActiveProfile();
    const char* profileNames[] = {"Slow", "Medium", "Fast"};
    
    bool selecting = true;
    while (selecting) {
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "Profile: %s\n%.0f steps/sec\nMenu=OK Enable=Cancel", 
               profileNames[profile], pump.getProfileSpeed(profile));
      display.showText(buffer);
      delay(100);
      
      if (pressButtonUp()) {
        profile = (profile + 1) % 3;
      }
      if (pressButtonDown()) {
        profile = (profile == 0) ? 2 : profile - 1;
      }
      if (pressButtonMenu()) {
        pump.setSpeedProfile(profile);
        display.showText("Profile Set");
        delay(1000);
        selecting = false;
      }
      if (pressButtonEnable()) {
        display.showText("Cancelled");
        delay(500);
        selecting = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  
  case 12: // Edit Profiles (Phase 3 Sprint 7)
  {
    PumpController &pump = PumpController::getInstance();
    uint8_t profile = 0;
    const char* profileNames[] = {"Slow", "Medium", "Fast"};
    
    bool editing = true;
    while (editing) {
      float speed = pump.getProfileSpeed(profile);
      
      char buffer[64];
      snprintf(buffer, sizeof(buffer), "Edit %s:\n%.0f steps/sec\nMenu=Next Enable=Done", 
               profileNames[profile], speed);
      display.showText(buffer);
      delay(100);
      
      if (pressButtonUp()) {
        speed += 1000.0f;
        if (speed > 50000.0f) speed = 50000.0f;
        pump.setProfileSpeed(profile, speed);
      }
      if (pressButtonDown()) {
        speed = max(speed - 1000.0f, 1000.0f);
        pump.setProfileSpeed(profile, speed);
      }
      if (pressButtonMenu()) {
        // Move to next profile
        profile = (profile + 1) % 3;
        if (profile == 0) {
          // Wrapped around, exit
          display.showText("Profiles Saved");
          delay(1000);
          editing = false;
        }
      }
      if (pressButtonEnable()) {
        display.showText("Profiles Saved");
        delay(1000);
        editing = false;
      }
      delay(100);
    }
    display.setState(DisplayManager::DisplayState::NORMAL);
  }
  break;
  }
}

const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

bool isInMenu()
{
  DisplayManager &display = DisplayManager::getInstance();
  if (display.getCurrentState() == DisplayManager::DisplayState::MENU)
    return true;
  return false;
}

void MenuHandler()
{
  DisplayManager &display = DisplayManager::getInstance();
  // Button/menu handling
  if (pressButtonMenu())
  {
    if (isInMenu())
    {
      printf("Menu selection: %d\n", menuIndex);
      runMenuSelection();
    }
    else
    {
      printf("Entering menu\n");
      menuIndex = 0;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
      display.setState(DisplayManager::DisplayState::MENU);
    }
  }
  if (isInMenu())
  {
    if (holdButtonUp())
    {
      printf("Menu up\n");
      menuIndex = (menuIndex + 1) % menuItemCount;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
    }
    if (holdButtonDown())
    {
      printf("Menu down\n");
      menuIndex = (menuIndex - 1 + menuItemCount) % menuItemCount;
      display.setContextMenu(menuIndex, menuItems, menuItemCount);
    }
  }
}
