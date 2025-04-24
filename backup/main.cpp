 #include "PumpController.h"

 #define EN_PIN           26 // Enable
 #define DIR_PIN          2 // Direction
 #define STEP_PIN         5 // Step
 #define RX_PIN           16   // ESP32 RX orange line
 #define TX_PIN           17   // ESP32 TX blue line
 

 #define R_SENSE 0.11f
 #define DRIVER_ADDR 0b00  // Default address for TMC2209
 
 PumpController pump(&Serial2, STEP_PIN, DIR_PIN, EN_PIN, R_SENSE, DRIVER_ADDR);
 
 void setup() {
   Serial.begin(115200);
   Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN); // ESP32 HardwareSerial with custom pins
 
   pump.begin();
   pump.setSpeed(2000);  // steps/sec
 } 
 
 void loop() {
  pump.runForever(2000);  // run forward forever at 150 steps/sec
 }
 