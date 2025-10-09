#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>

enum class PumpMode
{
  PERISTALTIC, // Continuous speed mode
  DOSING,      // Position control mode
  HOLDING      // Waiting between moves
};

class PumpController
{
public:
  // Singleton access
  static PumpController &getInstance();
  // Constructor & Initialization
  void init(Stream *serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr);
  void begin();

  // Mode Control
  void setMode(PumpMode newMode) { mode = newMode; }
  PumpMode getMode() const { return mode; }
  void setHoldDelay(unsigned long delay) { holdDelay = delay; }

  // Movement Control
  void runPeristaltic(); // Continuous speed mode
  void runDosing();      // Position control mode
  void stop();
  void moveToPosition(long position); // Absolute position
  void moveRelative(long steps);      // Relative movement
  void moveML(float ml); // Move by volume (ml)

  // Position & State
  long getCurrentPosition();
  long getDistanceToGo();
  void setCurrentPosition(int32_t position);
  bool getIsEnable() const { return isEnable; }

  // Calibration & Steps
  float getStepsPerML() const
  {
    return mode == PumpMode::DOSING ? dosingStepsPerML : peristalticStepsPerML;
  }
  void setStepsPerML(float steps)
  {
    if (mode == PumpMode::DOSING)
    {
      dosingStepsPerML = steps;
    }
    else
    {
      peristalticStepsPerML = steps;
    }
  }
  float getDosingStepsPerML() const { return dosingStepsPerML; }
  float getPeristalticStepsPerML() const { return peristalticStepsPerML; }
  void setDosingStepsPerML(float steps) { dosingStepsPerML = steps; }
  void setPeristalticStepsPerML(float steps) { peristalticStepsPerML = steps; }

  // Speed & Acceleration
  bool isRunning();
  void setAcceleration(float accel);
  void setMicrosteps(uint16_t ms);
  void setSpeed(float speed);
  float getSpeed() const { return currentSpeed; }
  void setSpeedStep(int step) { speedStep = step; }
  int getSpeedStep() const { return speedStep; }
  void setMaxSpeed(float speed) { stepper.setMaxSpeed(speed); }

private:
  // Singleton pattern
  PumpController();
  PumpController(const PumpController &) = delete;
  PumpController &operator=(const PumpController &) = delete;

  // Hardware
  TMC2209Stepper driver;
  AccelStepper stepper;
  uint8_t enPin;

  // State
  bool isEnable = false;
  PumpMode mode = PumpMode::HOLDING;
  unsigned long lastMoveTime = 0;
  unsigned long lastDebugTime = 0;
  unsigned long holdDelay = 0;
  long lastPosition = 0;

  // Calibration
  float peristalticStepsPerML = 709.22f;
  float dosingStepsPerML = 709.22f;

  // Speed
  float currentSpeed = 0;
  int speedStep = 2000;
  int maxSpeedStep = 4000;

  void enablePump();
  void disablePump();
  void updateCurrentPosition();
};

#endif