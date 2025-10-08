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
  if (enabled && currentSpeed > 0)
  {
    stepper.setSpeed(currentSpeed);
    stepper.runSpeed();
  }
}

void PumpController::runDosing()
{
  if (mode == PumpMode::DOSING && enabled)
  {
    // Always check actual distance remaining
    long remaining = abs(stepper.distanceToGo());

    if (remaining > 0)
    {
      // Still have steps to move
      moving = true;

      // Ensure motor is enabled
      digitalWrite(enPin, LOW);

      // Run multiple steps per call for faster movement
      for (int i = 0; i < 10; i++)
      {
        if (stepper.distanceToGo() != 0)
        {
          stepper.run();
        }
      }

      if (stepper.currentPosition() != lastPosition)
      {
        lastMoveTime = millis();
        lastPosition = stepper.currentPosition();
      }

      // Print debug info periodically
      if (millis() - lastDebugTime > 1000)
      {
        Serial.printf("Position: %ld/%ld, Remaining: %ld\n",
                      stepper.currentPosition(), stepper.targetPosition(), remaining);
        lastDebugTime = millis();
      }
    }
    else
    {
      // No more steps to move - movement complete
      mode = PumpMode::HOLDING;
      moving = false;
      enabled = false;
      currentSpeed = 0;
      digitalWrite(enPin, HIGH);     // Disable driver
      stepper.setCurrentPosition(0); // Reset position to 0 after move
      Serial.println("Dosing complete - target reached");
    }
  }
  else if (mode == PumpMode::HOLDING)
  {
    if (millis() - lastMoveTime >= holdDelay)
    {
      mode = PumpMode::DOSING; // Ready for next move
    }
  }
}

void PumpController::stop()
{
  enabled = false;
  moving = false;
  currentSpeed = 0;
  digitalWrite(enPin, HIGH);
  mode = PumpMode::HOLDING;
  stepper.stop();
}

void PumpController::moveToPosition(long position)
{
  enabled = true;
  currentSpeed = 0; // Use position mode
  digitalWrite(enPin, LOW);
  stepper.moveTo(position);
  moving = true;
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
  enabled = (currentSpeed > 0);
  digitalWrite(enPin, !enabled); // LOW = enabled
  moving = false;
}

bool PumpController::updateMoving()
{
  if (mode == PumpMode::DOSING)
  {
    moving = stepper.isRunning();
  }

  return moving;
}

bool PumpController::isMovementComplete()
{
  if (moving)
  {
    if (mode == PumpMode::DOSING)
    {
      mode = PumpMode::HOLDING;
      lastMoveTime = millis();
      return true;
    }
  }
  return false;
}

void PumpController::setAcceleration(float accel)
{
  stepper.setAcceleration(accel);
}

void PumpController::setMicrosteps(uint16_t ms)
{
  driver.microsteps(ms);
}
