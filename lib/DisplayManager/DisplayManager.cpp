#include "DisplayManager.h"

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

DisplayManager &DisplayManager::getInstance()
{
  static DisplayManager instance;
  return instance;
}

void DisplayManager::begin()
{
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("Hello OLED!"));
  displaySignalStrength();
  display.display();
}

void DisplayManager::setSignalStrength(int strength)
{
  rssi = strength;
}

void DisplayManager::updateStatus(bool pumpEnabled, float value, PumpMode mode, const char* currentTime, bool autodosingEnabled, const char* nextSchedule)
{
  if (displaySleeping)
    return;
  display.clearDisplay();
  display.setCursor(0, 0);
  
  // Show mode and pump status (Line 1)
  display.print(mode == PumpMode::PERISTALTIC ? "PERIST" : "DOSING");
  display.print(" | ");
  display.println(pumpEnabled ? "ON" : "OFF");
  
  // Show auto-dosing status and current value (Line 2)
  if (mode == PumpMode::DOSING) {
    display.print("AUTO:");
    display.print(autodosingEnabled ? "ON" : "OFF");
    display.print(" | ");
    display.print(value, 1);
    display.println("mL");
  } else {
    display.print("mL/min: ");
    display.println(value, 1);
  }
  
  // Show current time and next schedule (Line 3-4)
  if (mode == PumpMode::DOSING) {
    if (currentTime) {
      display.print("Time: ");
      display.println(currentTime);
    }
    if (autodosingEnabled && nextSchedule) {
      display.print("Next: ");
      display.println(nextSchedule);
    }
  }
  
  displaySignalStrength();
  display.display();
}

void DisplayManager::showMenu(int menuIndex, const char *menuItems[], int itemCount)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Menu:");
  for (int i = 0; i < itemCount; i++)
  {
    display.print(menuIndex == i ? "> " : "  ");
    display.println(menuItems[i]);
  }
  displaySignalStrength(); // Will be updated by caller if needed
  display.display();
}

void DisplayManager::showSettingsInfo(int currentSpeed, float stepsPerML, int speedStep)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Settings Info:");
  display.print("Speed: ");
  display.println(currentSpeed);
  display.print("Steps/mL: ");
  display.println(stepsPerML, 2);
  display.print("Step Adj: ");
  display.println(speedStep);
  displaySignalStrength();
  display.display();
  delay(5000);
}

void DisplayManager::showCalibrationStart(int timeLeft)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibrating...");
  display.print("Time left: ");
  display.print(timeLeft);
  display.println("s");
  displaySignalStrength();
  display.display();
}

void DisplayManager::showCalibrationInput(float ml)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Enter mL result:");
  display.setCursor(0, 20);
  display.print("mL: ");
  display.print(ml);
  display.print("   ");
  displaySignalStrength();
  display.display();
}

void DisplayManager::showCalibrationResult(float stepsPerML, int speedStep)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibration Done");
  display.print("Steps/mL: ");
  display.println(stepsPerML, 2);
  display.print("Step Adj: ");
  display.println(speedStep);
  displaySignalStrength();
  display.display();
  delay(3000);
}

void DisplayManager::showText(const char *text)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(text);
  displaySignalStrength(); // Caller can update RSSI
  display.display();
}

void DisplayManager::showText(const std::vector<String> &textArray)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  for (size_t i = 0; i < textArray.size(); i++)
  {
    display.println(textArray[i]);
  }
  displaySignalStrength();
  display.display();
}

void DisplayManager::sleepDisplay()
{
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  displaySleeping = true;
}

void DisplayManager::wakeDisplay()
{
  display.ssd1306_command(SSD1306_DISPLAYON);
  displaySleeping = false;
}

void DisplayManager::showDosingSetup(float volume, bool isVolumeSetup)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Manual Dosing Setup");
  display.println();
  
  if (isVolumeSetup) {
    display.println("Set Target Volume:");
    display.print(volume, 2);
    display.println(" mL");
    display.println();
    display.println("UP/DOWN to adjust");
    display.println("ENABLE to confirm");
    display.println("MENU to cancel");
  } else {
    display.println("Set Time Duration:");
    display.print(volume);
    display.println(" min");
    display.println();
    display.println("UP/DOWN to adjust");
    display.println("ENABLE to start");
    display.println("MENU to cancel");
  }
  
  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingProgress(float volume, float remainingVolume, const char* remainingTime)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Dosing in Progress");
  display.println();
  
  display.print("Total: ");
  display.print(volume, 2);
  display.println(" mL");
  
  display.print("Remain: ");
  display.print(remainingVolume, 2);
  display.println(" mL");
  
  display.println();
  display.print("Time Left: ");
  display.println(remainingTime);
  
  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingComplete(float totalVolume)
{
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Dosing Complete!");
  display.println();
  
  display.print("Total Dosed: ");
  display.print(totalVolume, 2);
  display.println(" mL");
  
  displaySignalStrength();
  display.display();
  delay(3000);
}

void DisplayManager::displaySignalStrength()
{
  display.setCursor(0, SCREEN_HEIGHT - 8);
  if (rssi < -50)
  {
    drawWiFiSignal(4);
  }
  else if (rssi < -60)
  {
    drawWiFiSignal(3);
  }
  else if (rssi < -70)
  {
    drawWiFiSignal(2);
  }
  else if (rssi < -80)
  {
    drawWiFiSignal(1);
  }
  else
  {
    drawWiFiSignal(0);
  }
}

void DisplayManager::drawWiFiSignal(int strength)
{
  // strength: 0 to 4
  for (int i = 0; i < 4; i++)
  {
    int barHeight = 2;
    if (i < strength)
    {
      display.fillRect(0 + i * 4, SCREEN_HEIGHT - barHeight, 3, barHeight, WHITE);
    }
    else
    {
      display.drawRect(0 + i * 4, SCREEN_HEIGHT - barHeight, 3, barHeight, WHITE);
    }
  }
}

void DisplayManager::showValue(const char* label, float value) {
    if (displaySleeping) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Show label
    display.setCursor(0, 0);
    display.println(label);
    
    // Show value in larger text
    display.setTextSize(1);
    display.setCursor(0, 16);
    display.print(value, 1);
    
    displaySignalStrength();
    display.display();
}