#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>
#include <TMCStepper.h>
#include <AccelStepper.h>


enum class PumpMode {
  PERISTALTIC,  // Continuous speed mode
  DOSING,       // Position control mode
  HOLDING       // Waiting between moves
};

class PumpController {
public:
  PumpController(Stream* serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr);
  void begin();
  
  // Mode specific run functions
  void runPeristaltic();  // Run in continuous speed mode
  void runDosing();       // Run in position control mode
  
  // Movement control
  void stop();
  void moveToPosition(long position);    // Move to absolute position
  void moveRelative(long steps);         // Move relative to current position
  void moveML(float ml);                 // Move by volume in milliliters
  
  // Position and state
  long getCurrentPosition();
  void setCurrentPosition(int32_t position);
  bool isEnabled() const { return enabled; }
  bool isMoving() const { return enabled && moving; }
  float getStepsPerML() const { 
    return mode == PumpMode::DOSING ? dosingStepsPerML : peristalticStepsPerML; 
  }
  void setStepsPerML(float steps) { 
    if (mode == PumpMode::DOSING) {
      dosingStepsPerML = steps;
    } else {
      peristalticStepsPerML = steps;
    }
  }
  float getDosingStepsPerML() const { return dosingStepsPerML; }
  float getPeristalticStepsPerML() const { return peristalticStepsPerML; }
  void setDosingStepsPerML(float steps) { dosingStepsPerML = steps; }
  void setPeristalticStepsPerML(float steps) { peristalticStepsPerML = steps; }
  
  // Speed and acceleration control
  void setAcceleration(float accel);
  void setMicrosteps(uint16_t ms);
  void setSpeed(float speed);
  float getSpeed() const { return currentSpeed; }
  void setSpeedStep(int step) { speedStep = step; }
  int getSpeedStep() const { return speedStep; }
  void setMaxSpeed(float speed) { stepper.setMaxSpeed(speed); }
  
  // Mode control
  void setMode(PumpMode newMode) { mode = newMode; }
  PumpMode getMode() const { return mode; }
  void setHoldDelay(unsigned long delay) { holdDelay = delay; }

private:
  TMC2209Stepper driver;
  AccelStepper stepper;
  uint8_t enPin;
  bool enabled = false;
  float peristalticStepsPerML = 709.22f;
  float dosingStepsPerML = 709.22f;
  float currentSpeed = 0;
  int speedStep = 2000;
  int maxSpeedStep = 4000;
  
  // Movement state
  PumpMode mode = PumpMode::HOLDING;
  bool moving = false;
  unsigned long lastMoveTime = 0;
  unsigned long lastDebugTime = 0;  // For periodic debug output
  unsigned long holdDelay = 0;  // Delay between moves in dosing mode
  long lastPosition = 0;        // Last recorded position for movement detection
  
  // Helper functions
  bool isMovementComplete();
  void updateMoving();
};

#endif