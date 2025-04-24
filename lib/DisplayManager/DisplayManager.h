#ifndef DISPLAY_MANAGER_H
#define DISPLAY_MANAGER_H

#include <Adafruit_SSD1306.h>
#include <Wire.h>
#include <vector>

class DisplayManager
{
public:
    static DisplayManager &getInstance();
    void begin();
    void setSignalStrength(int strength);
    void updateStatus(bool pumpEnabled, float mlPerMin);
    void sleepDisplay();
    void wakeDisplay();
    void showMenu(int menuIndex, const char *menuItems[], int itemCount);
    void showSettingsInfo(int currentSpeed, float stepsPerML, int speedStep);
    void showCalibrationStart(int timeLeft);
    void showCalibrationInput(float ml);
    void showCalibrationResult(float stepsPerML, int speedStep);
    void showText(const char *text);
    void showText(const std::vector<String> &textArray);

    bool isSleeping() const { return displaySleeping; }

    // Display Pins and Settings
    static const int SCREEN_WIDTH = 128;
    static const int SCREEN_HEIGHT = 64;
    static const int OLED_RESET = -1;

private:
    DisplayManager();
    DisplayManager(const DisplayManager &) = delete;            // Prevent copying
    DisplayManager &operator=(const DisplayManager &) = delete; // Prevent assignment

    Adafruit_SSD1306 display;

    bool displaySleeping = false;
    int rssi = 0; 
    void displaySignalStrength();
    void drawWiFiSignal(int strength);
};

#endif