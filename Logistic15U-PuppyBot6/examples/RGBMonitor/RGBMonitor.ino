#include <Logistic15U_PuppyBot6.h>

void setup() {
  puppybot_setup();
  beginRGB();
  beginRGBDisplay();
}

void loop() {
  updateRGB();  // Services both sensors without waiting for integration.
  static unsigned long lastRead = 0;
  if (millis() - lastRead >= 50) {
    lastRead = millis();
    showRGB();  // Or showRGB(RGB0) / showRGB(RGB1) for a single sensor.
  }
}
