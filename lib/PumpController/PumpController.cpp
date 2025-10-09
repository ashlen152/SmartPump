#include "PumpController.h"

void PumpController::init(Stream *serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr)
{
  // Reinitialize driver using placement new to avoid assignment
  new (&driver) TMC2209Stepper(serialPort, rSense, addr);
  stepper = AccelStepper(AccelStepper::DRIVER, stepPin, dirPin);
  enPin = enablePin;
}

PumpController &PumpController::getInstance()
{
  static PumpController instance;
  return instance;
}

void PumpController::begin()
{
  pinMode(enPin, OUTPUT);
  digitalWrite(enPin, HIGH); // Disabled by default (HIGH = off for TMC2209)

  driver.begin();
  driver.toff(5);             // Enable driver
  driver.rms_current(500);    // Increase motor current (mA)
  driver.pwm_autoscale(true); // Enable stealthChop

  stepper.setMaxSpeed(4000);
  stepper.setAcceleration(2000); // Lower acceleration for smoother movement
  speedStep = 2000;
  currentSpeed = 0;
}

void PumpController::runPeristaltic()
{
  if (isEnable && currentSpeed > 0)
  {
    stepper.setSpeed(currentSpeed);
    stepper.runSpeed();
  }
}

long PumpController::getDistanceToGo()
{
  return stepper.distanceToGo();
}

void PumpController::runDosing()
{
  if (mode == PumpMode::DOSING && isEnable)
  {
    // Always check actual distance remaining
    long remaining = abs(getDistanceToGo());

    if (remaining > 0)
    {
      for (int i = 0; i < 10; i++)
      {
        if (getDistanceToGo() != 0)
        {
          stepper.run();
        }
      }

      if (getCurrentPosition() != lastPosition)
      {
        lastMoveTime = millis();
        lastPosition = getCurrentPosition();
      }

      // Print debug info periodically
      if (millis() - lastDebugTime > 1000)
      {
        Serial.printf("Position: %ld/%ld, Remaining: %ld\n",
                      getCurrentPosition(), getDistanceToGo(), remaining);
        lastDebugTime = millis();
      }
    }
    else
    {
      // No more steps to move - movement complete
      stop();
      Serial.println("Dosing complete - target reached");
    }
  }
}

void PumpController::stop()
{
  currentSpeed = 0;
  mode = PumpMode::HOLDING;
  stepper.setCurrentPosition(0); // Reset position to 0 after move
  stepper.stop();
  disablePump();
}

void PumpController::moveToPosition(long position)
{
  stepper.moveTo(position);
}

void PumpController::moveRelative(long steps)
{
  stepper.move(steps);
}

void PumpController::enablePump()
{
  digitalWrite(enPin, LOW);
  isEnable = true;
}

void PumpController::disablePump()
{
  digitalWrite(enPin, HIGH);
  isEnable = false;
}

void PumpController::moveML(float ml)
{
  mode = PumpMode::DOSING;
  // Reset position to 0 before starting new movement
  long currentPos = stepper.currentPosition();
  stepper.setCurrentPosition(0);

  // Calculate required steps using calibrated value
  long steps = (long)(ml * getStepsPerML());
  Serial.printf("moveML: Moving %.2f mL = %ld steps (stepsPerML: %.2f)\n",
                ml, steps, getStepsPerML());

  enablePump();
  // Move relative to new zero position
  moveRelative(steps);
  Serial.printf("moveML: Target position: %ld, Started from: %ld\n",
                steps, currentPos);
}

long PumpController::getCurrentPosition()
{
  return stepper.currentPosition();
}

void PumpController::setCurrentPosition(int32_t position)
{
  stepper.setCurrentPosition(position);
}

void PumpController::setSpeed(float speed)
{
  currentSpeed = constrain(speed, 0, stepper.maxSpeed());
}

bool PumpController::isRunning()
{
  return stepper.isRunning();
}

void PumpController::setAcceleration(float accel)
{
  stepper.setAcceleration(accel);
}

void PumpController::setMicrosteps(uint16_t ms)
{
  driver.microsteps(ms);
}
