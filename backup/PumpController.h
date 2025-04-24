#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <TMCStepper.h>
#include <AccelStepper.h>

class PumpController {
  private:
    TMC2209Stepper driver;
    AccelStepper stepper;
    uint8_t enPin;

  public:
    PumpController(Stream* serialPort, uint8_t stepPin, uint8_t dirPin, uint8_t enablePin, float rSense, uint8_t addr);

    void begin() ;

    void runForever(float speed) ;

    void stopPump() ;

    void setSpeed(float speed) ;

    void setAcceleration(float accel) ;

    void setMicrosteps(uint16_t ms);
};

#endif
