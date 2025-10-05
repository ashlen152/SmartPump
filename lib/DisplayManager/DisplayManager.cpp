#include "DisplayManager.h"

DisplayManager::DisplayManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

DisplayManager &DisplayManager::getInstance()
{
  static DisplayManager instance;
  return instance;
}

// Call this in your main loop to update display every second
void DisplayManager::updateDisplayState()
{
  lastUpdate = 0;
  unsigned long now = millis();
  if (now - lastUpdate < 200)
    return;
  lastUpdate = now;

  display.clearDisplay();

  // States that are handled manually and should not auto-revert
  bool isManualState = (currentState == DisplayState::CALIBRATION_START ||
                        currentState == DisplayState::CALIBRATION_INPUT ||
                        currentState == DisplayState::CALIBRATION_RESULT ||
                        currentState == DisplayState::DOSING_SETUP ||
                        currentState == DisplayState::DOSING_PROGRESS ||
                        currentState == DisplayState::DOSING_COMPLETE);

  // Timing logic for STATUS state
  if ((currentState != DisplayState::NORMAL || currentState != DisplayState::MENU) && now - stateChangeTime > 3000)
  {
    currentState = DisplayState::NORMAL;
  }
  // Single switch-case for all states
  switch (currentState)
  {
  case DisplayState::NORMAL:
    updateStatus(m_ctx.pumpEnabled, m_ctx.value, m_ctx.currentTime, m_ctx.autodosingEnabled, m_ctx.nextSchedule);
    stateChangeTime = now;
    break;
  case DisplayState::MENU:
    stateChangeTime = now;
    showMenu(m_ctx.menuIndex, m_ctx.menuItems, m_ctx.itemCount);
    break;
  case DisplayState::SETTINGS:
    showSettingsInfo(m_ctx.currentSpeed, m_ctx.stepsPerML, m_ctx.speedStep);
    break;
  case DisplayState::CALIBRATION_START:
    showCalibrationStart(m_ctx.timeLeft);
    break;
  case DisplayState::CALIBRATION_INPUT:
    showCalibrationInput(m_ctx.ml);
    break;
  case DisplayState::CALIBRATION_RESULT:
    showCalibrationResult(m_ctx.stepsPerML, m_ctx.speedStep);
    break;
  case DisplayState::DOSING_SETUP:
    showDosingSetup(m_ctx.value);
    break;
  case DisplayState::DOSING_BEGIN:
    showDosingBegin(m_ctx.duration);
    break;
  case DisplayState::DOSING_PROGRESS:
    showDosingProgress(m_ctx.value, m_ctx.remainingVolume, m_ctx.remainingTime);
    break;
  case DisplayState::DOSING_COMPLETE:
    showDosingComplete(m_ctx.totalVolume);
    break;
  case DisplayState::STATUS:
    updateStatus(m_ctx.pumpEnabled, m_ctx.value, m_ctx.currentTime, m_ctx.autodosingEnabled, m_ctx.nextSchedule);
    break;
  default:
    updateStatus(m_ctx.pumpEnabled, m_ctx.value, m_ctx.currentTime, m_ctx.autodosingEnabled, m_ctx.nextSchedule);
    break;
  }
  lastState = currentState;
  display.display();
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

void DisplayManager::updateStatus(bool pumpEnabled, float value, const char *currentTime, bool autodosingEnabled, const char *nextSchedule)
{
  if (isDisplayInUse(DisplayManager::DisplayState::NORMAL))
    return;
  display.clearDisplay();
  display.setCursor(0, 0);

  // Show mode and pump status (Line 1)
  display.print("DOSING");
  display.print(" | ");
  display.println("Pump: " + pumpEnabled ? "ON" : "OFF");

  display.print("AUTO:");
  display.print(autodosingEnabled ? "ON" : "OFF");
  display.print(" | ");
  display.print(value, 1);
  display.println("mL");

  if (currentTime)
  {
    display.print("Time: ");
    display.println(currentTime);
  }
  if (autodosingEnabled && nextSchedule)
  {
    display.print("Next: ");
    display.println(nextSchedule);
  }

  displaySignalStrength();
  display.display();
}

bool DisplayManager::isDisplayInUse(DisplayManager::DisplayState state)
{
  if (currentState == state && !displaySleeping)
    return false;
  return true;
}

void DisplayManager::showMenu(int menuIndex, const char *menuItems[], int itemCount)
{
  if (isDisplayInUse(DisplayManager::DisplayState::MENU))
    return;

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
  if (isDisplayInUse(DisplayManager::DisplayState::SETTINGS))
    return;

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
}

void DisplayManager::showCalibrationStart(int timeLeft)
{
  if (isDisplayInUse(DisplayManager::DisplayState::CALIBRATION_START))
    return;

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
  if (isDisplayInUse(DisplayManager::DisplayState::CALIBRATION_INPUT))
    return;
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
  if (isDisplayInUse(DisplayManager::DisplayState::CALIBRATION_RESULT))
    return;
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Calibration Done");
  display.print("Steps/mL: ");
  display.println(stepsPerML, 2);
  display.print("Step Adj: ");
  display.println(speedStep);
  displaySignalStrength();
  display.display();
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

void DisplayManager::showDosingSetup(float volume)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_SETUP))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Manual Dosing Setup");
  display.println();

  display.println("Set Target Volume:");
  display.print(volume, 2);
  display.println(" mL");
  display.println();
  display.println("UP/DOWN to adjust");
  display.println("ENABLE to confirm");
  display.println("MENU to cancel");

  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingBegin(int duration)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_BEGIN))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Manual Dosing Setup");
  display.println();

  display.println("Set Time Duration:");
  display.print(duration);
  display.println(" min");
  display.println();
  display.println("UP/DOWN to adjust");
  display.println("ENABLE to start");
  display.println("MENU to cancel");

  displaySignalStrength();
  display.display();
}

void DisplayManager::showDosingProgress(float volume, float remainingVolume, const char *remainingTime)
{
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_PROGRESS))
    return;

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
  if (isDisplayInUse(DisplayManager::DisplayState::DOSING_COMPLETE))
    return;

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Dosing Complete!");
  display.println();

  display.print("Total Dosed: ");
  display.print(totalVolume, 2);
  display.println(" mL");

  displaySignalStrength();
  display.display();
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

void DisplayManager::showValue(const char *label, float value)
{
  if (isDisplayInUse(DisplayManager::DisplayState::NORMAL))
    return;

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