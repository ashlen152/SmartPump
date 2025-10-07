#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <vector>

// Context struct for passing display state
struct DisplayContext
{
    bool pumpEnabled = false;
    float value = 0.0f;
    const char *currentTime = nullptr;
    bool autodosingEnabled = false;
    const char *nextSchedule = nullptr;
    int menuIndex = 0;
    const char **menuItems = nullptr;
    int itemCount = 0;
    int currentSpeed = 0;
    float stepsPerML = 0.0f;
    int speedStep = 0;
    int timeLeft = 0;
    float ml = 0.0f;
    float remainingVolume = 0.0f;
    const char *remainingTime = nullptr;
    float totalVolume = 0.0f;
    int duration = 1;
};

class DisplayManager
{
public:
    // Enums
    enum class PumpMode
    {
        PERISTALTIC,
        DOSING
    };

    enum class DisplayState
    {
        NORMAL,
        MENU,
        SETTINGS,
        CALIBRATION_START,
        CALIBRATION_INPUT,
        CALIBRATION_RESULT,
        DOSING_SETUP,
        DOSING_BEGIN,
        DOSING_PROGRESS,
        DOSING_COMPLETE,
        DOSING_MANUAL_START,
        DOSING_MANUAL_BEGIN,
        DOSING_MANUAL_PROGRESS,
        DOSING_MANUAL_COMPLETE,
        STATUS,
        INFO,
        ERROR,
        WARNING,
        SUCCESS,
    };

    enum class DosingState
    {
        IDLE,
        SETUP_VOLUME,
        SETUP_TIME,
        RUNNING,
        PAUSED,
        COMPLETED
    };

    // Singleton access
    static DisplayManager &getInstance();

    // Initialization
    void begin();

    // State management
    void setState(DisplayManager::DisplayState state) { currentState = state; }
    DisplayState getCurrentState() const { return currentState; }
    bool isSleeping() const { return displaySleeping; }

    // Status & Info
    void updateStatus(bool pumpEnabled, float value, const char *currentTime = nullptr, bool autodosingEnabled = false, const char *nextSchedule = nullptr);
    void setSignalStrength(int strength);

    // Sleep/Wake
    void sleepDisplay();
    void wakeDisplay();

    // Menu/UI
    void showMenu(int menuIndex, const char *menuItems[], int itemCount);
    void showSettingsInfo(int currentSpeed, float stepsPerML, int speedStep);

    // Calibration UI
    void showCalibrationStart(int timeLeft);
    void showCalibrationInput(float ml);
    void showCalibrationResult(float stepsPerML, int speedStep);

    // Text display
    void showText(const char *text);
    void showText(const std::vector<String> &textArray);
    void showValue(const char *label, float value);

    // Main loop display updater
    void updateDisplayState();

    // Manual dosing UI
    void showDosingManualSetup(float volume);
    void showDosingManualBegin(int du);
    void showDosingManualProgress(float volume, float remainingVolume, const char *remainingTime);
    void showDosingManualComplete(float totalVolume);

    // Display constants
    static const int SCREEN_WIDTH = 128;
    static const int SCREEN_HEIGHT = 64;
    static const int OLED_RESET = -1;

    // Context setter functions for each state
    void setContextNormal(bool pumpEnabled, float value, const char *currentTime, bool autodosingEnabled, const char *nextSchedule)
    {
        m_ctx.pumpEnabled = pumpEnabled;
        m_ctx.value = value;
        m_ctx.currentTime = currentTime;
        m_ctx.autodosingEnabled = autodosingEnabled;
        m_ctx.nextSchedule = nextSchedule;
    }
    void setContextMenu(int menuIndex, const char **menuItems, int itemCount)
    {
        m_ctx.menuIndex = menuIndex;
        m_ctx.menuItems = menuItems;
        m_ctx.itemCount = itemCount;
    }
    void setContextSettings(int currentSpeed, float stepsPerML, int speedStep)
    {
        m_ctx.currentSpeed = currentSpeed;
        m_ctx.stepsPerML = stepsPerML;
        m_ctx.speedStep = speedStep;
    }
    void setContextCalibrationStart(int timeLeft)
    {
        m_ctx.timeLeft = timeLeft;
    }
    void setContextCalibrationInput(float ml)
    {
        m_ctx.ml = ml;
    }
    void setContextCalibrationResult(float stepsPerML, int speedStep)
    {
        m_ctx.stepsPerML = stepsPerML;
        m_ctx.speedStep = speedStep;
    }
    void setContextDosingManualBegin(float volume)
    {
        m_ctx.value = volume;
    }
    void setContextDosingManualStart(int duration)
    {
        m_ctx.duration = duration;
    }
    void setContextDosingManualProgress(float volume, float remainingVolume, const char *remainingTime)
    {
        m_ctx.value = volume;
        m_ctx.remainingVolume = remainingVolume;
        m_ctx.remainingTime = remainingTime;
    }
    void setContextDosingManualComplete(float totalVolume)
    {
        m_ctx.totalVolume = totalVolume;
    }

private:
    // Singleton pattern
    DisplayManager();
    DisplayManager(const DisplayManager &) = delete;
    DisplayManager &operator=(const DisplayManager &) = delete;

    // Internal helpers
    bool isDisplayInUse(DisplayState state);
    void displaySignalStrength();
    void drawWiFiSignal(int strength);

    // Members
    Adafruit_SSD1306 display;
    bool displaySleeping = false;

    unsigned long lastUpdate = 0;

    DisplayState currentState = DisplayState::NORMAL;
    DisplayState lastState = DisplayState::NORMAL;
    unsigned long stateChangeTime = 0;

    int rssi = 0;
    DisplayContext m_ctx;
};

#endif