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

void DisplayManager::updateStatus(bool pumpEnabled, float mlPerMin)
{
  if (displaySleeping)
    return;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(pumpEnabled ? "Pump Enabled" : "Pump Disabled");
  display.print("mL/min: ");
  display.println(mlPerMin, 2);
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