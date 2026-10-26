#include <PuppyBotEncoders.h>
using namespace PuppyBotIMU;

// Encoder-only test: no motor, servo, IMU or color initialization.
// Uses the original library pins: encoder 1 = 17/18, encoder 2 = 19/20.
// Verify those connections against the actual board before connecting sensors.
void setup() {
  Serial.begin(115200);
  init_encoders();
  resetEncoder(1);
  resetEncoder(2);
}

void loop() {
  while (Serial.available()) {
    const char command = Serial.read();
    if (command == '1') resetEncoder(1);
    else if (command == '2') resetEncoder(2);
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 100) {
    lastPrint = millis();
    Serial.print("E1=");
    Serial.print(get_pulse_Encoder(1));
    Serial.print("\tE2=");
    Serial.println(get_pulse_Encoder(2));
  }
}
