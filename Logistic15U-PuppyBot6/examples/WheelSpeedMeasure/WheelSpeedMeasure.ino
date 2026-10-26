#include <Logistic15U_PuppyBot6.h>

// Operator test only: lift both drive wheels clear before pressing SW1.
// This measures a no-load reference, not CPR or physical wheel distance.
constexpr int TEST_PWM = 30;
float referenceRate[2] = {};
int measuredSign[2] = {};

void label(int y, const char *text, uint16_t color = ST77XX_WHITE) {
  tft_.setTextColor(color, ST77XX_BLACK); tft_.setCursor(4, y); tft_.print(text);
}
void selectSW1() {
  PuppyBotInput::MenuButtons buttons;
  while (buttons.update(false, digitalRead(swpin) == LOW, millis()) != PuppyBotInput::ButtonEvent::Select) delay(1);
}
bool waitOrCancel(uint32_t start, uint32_t duration) {
  while (uint32_t(millis() - start) < duration) {
    if (digitalRead(swpin) == LOW) { ao(); return false; }
    delay(1);
  }
  return true;
}
void measure(unsigned channel) {
  ao(); tft_.fillScreen(ST77XX_BLACK);
  char text[26]; snprintf(text, sizeof(text), "M%u / Encoder %u", channel, channel);
  label(8, text, ST77XX_CYAN);
  label(28, "LIFT BOTH WHEELS", ST77XX_YELLOW);
  label(48, "SW1: start test");
  label(66, "SW1 during run: stop");
  label(86, "PWM +30; bounded run");
  selectSW1();
  motor(channel, TEST_PWM);
  const bool warmed = waitOrCancel(millis(), 250);
  const int32_t before = PuppyBotIMU::get_pulse_Encoder(channel);
  const uint32_t startedAt = millis();
  const bool completed = warmed && waitOrCancel(startedAt, 1000);
  const int32_t after = PuppyBotIMU::get_pulse_Encoder(channel);
  const uint32_t elapsed = millis() - startedAt;
  ao();
  const uint32_t wrapped = uint32_t(after) - uint32_t(before);
  const int64_t delta = wrapped <= 0x7fffffffu ? int64_t(wrapped) : int64_t(wrapped) - 0x100000000LL;
  if (completed && elapsed > 0 && delta != 0) {
    measuredSign[channel - 1] = delta > 0 ? 1 : -1;
    referenceRate[channel - 1] = float(delta > 0 ? delta : -delta) * 1000.0f / elapsed;
  }
  Serial.print("M"); Serial.print(channel); Serial.print(" sign="); Serial.print(measuredSign[channel - 1]);
  Serial.print(" referencePwm="); Serial.print(TEST_PWM); Serial.print(" ticks/s=");
  Serial.println(referenceRate[channel - 1]);
  tft_.fillScreen(ST77XX_BLACK);
  snprintf(text, sizeof(text), "M%u sign: %d", channel, measuredSign[channel - 1]); label(12, text, ST77XX_CYAN);
  snprintf(text, sizeof(text), "ticks/s: %.1f", double(referenceRate[channel - 1])); label(34, text);
  label(54, completed ? "NO-LOAD REFERENCE" : "TEST CANCELLED", ST77XX_YELLOW);
  label(76, "SW1: next");
  selectSW1();
}

void setup() {
  Serial.begin(115200); puppybot_setup(); ao();
  pinMode(swpin, INPUT_PULLUP); PuppyBotIMU::init_encoders();
  tft_.setFont(nullptr); tft_.setTextSize(1); tft_.setTextWrap(false);
  measure(1); measure(2);
  tft_.fillScreen(ST77XX_BLACK);
  label(6, "RESULT / PWM +30", ST77XX_CYAN);
  for (unsigned i = 0; i < 2; ++i) {
    char text[26]; snprintf(text, sizeof(text), "M%u E%u sign %d", i+1, i+1, measuredSign[i]); label(28 + i*36, text);
    snprintf(text, sizeof(text), "%.1f ticks/s", double(referenceRate[i])); label(40 + i*36, text);
  }
  label(110, "No pulses = check wire", ST77XX_YELLOW);
}
void loop() { delay(10); }
