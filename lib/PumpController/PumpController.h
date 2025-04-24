#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <TMCStepper.h>
#include <AccelStepper.h>

class PumpController {
public:
  PumpController(Stream* serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr);
  void begin();
  void run();
  void stop();
  void setSpeed(float speed); // Speed in steps/sec
  void setAcceleration(float accel);
  void setMicrosteps(uint16_t ms);
  bool isEnabled() const { return enabled; }
  float getSpeed() const { return currentSpeed; }
  float getStepsPerML() const { return stepsPerML; }
  int getSpeedStep() const { return speedStep; }
  void setStepsPerML(float steps) { stepsPerML = steps; } // Setter for external calibration
  void setSpeedStep(int step) { speedStep = step; }       // Setter for external calibration
  int getMaxSpeedStep() const { return maxSpeedStep; } // Getter for external calibration

private:
  TMC2209Stepper driver;
  AccelStepper stepper;
  uint8_t enPin;
  bool enabled = false;
  float currentSpeed = 0;
  float stepsPerML = 0;
  int speedStep = 2000;
  int maxSpeedStep = 4000; // Maximum speed step
};

#endif