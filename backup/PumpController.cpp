#include "PumpController.h"

PumpController::PumpController(Stream* serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr)
  : driver(serialPort, rSense, addr), stepper(AccelStepper::DRIVER, stepPin, dirPin), enPin(enablePin) {}

void PumpController::begin() {
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, LOW); // Enable driver

  driver.begin();
  driver.toff(5);                   // Enable driver
  driver.rms_current(500);         // Set motor current (mA)
  driver.microsteps(16);           // Microstepping (can be set to 256)
  driver.pwm_autoscale(true);      // Needed for stealthChop

  stepper.setMaxSpeed(200);        // Default speed
  stepper.setAcceleration(100);    // Default acceleration
}

void PumpController::runForever(float speed) {
  stepper.setSpeed(speed);  // speed in steps/sec
  stepper.runSpeed();       // continuous movement
}

void PumpController::stopPump() {
  stepper.stop();
}

void PumpController::setSpeed(float speed) {
  stepper.setMaxSpeed(speed);
}

void PumpController::setAcceleration(float accel) {
  stepper.setAcceleration(accel);
}

void PumpController::setMicrosteps(uint16_t ms) {
  driver.microsteps(ms); // e.g., 16 or 256
}
