#include <Logistic15U_PuppyBot6.h>
#include <atomic>

std::atomic<bool> rgbBoardReady{false};

void setup() {
  Serial.begin(115200);
  puppybot_setup();
  loadRGBCalibration();
  beginRGBColorDisplay();
  rgbBoardReady.store(true, std::memory_order_release);
}

void loop() {
  showRGBColor();  // Both sensors; refresh/dirty drawing are handled by the library.
}

void setup1() {
  while (!rgbBoardReady.load(std::memory_order_acquire)) delay(1);
  beginRGB();
}

void loop1() {
  updateRGB();  // Nonblocking integration; service on the same core as IMU.
}
