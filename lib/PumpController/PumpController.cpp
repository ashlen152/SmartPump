#include "PumpController.h"

PumpController::PumpController(Stream* serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr)
    : driver(serialPort, rSense, addr),
      stepper(AccelStepper::DRIVER, stepPin, dirPin),
      enPin(enablePin) {}

void PumpController::begin() {
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, HIGH); // Disabled by default (HIGH = off for TMC2209)

  driver.begin();
  driver.toff(5);              // Enable driver
  driver.rms_current(500);     // Set motor current (mA)
  driver.microsteps(256);       // Default microstepping
  driver.pwm_autoscale(true);  // Enable stealthChop

  stepper.setMaxSpeed(4000);
  stepper.setAcceleration(100);
}

void PumpController::run() {
  if (enabled && currentSpeed > 0) {
    stepper.setSpeed(currentSpeed);
    stepper.runSpeed();
  }
}

void PumpController::stop() {
  enabled = false;
  digitalWrite(enPin, HIGH);
  stepper.stop();
}

void PumpController::setSpeed(float speed) {
  currentSpeed = constrain(speed, 0, stepper.maxSpeed());
  enabled = (currentSpeed > 0);
  digitalWrite(enPin, !enabled); // LOW = enabled
}

void PumpController::setAcceleration(float accel) {
  stepper.setAcceleration(accel);
}

void PumpController::setMicrosteps(uint16_t ms) {
  driver.microsteps(ms);
}
