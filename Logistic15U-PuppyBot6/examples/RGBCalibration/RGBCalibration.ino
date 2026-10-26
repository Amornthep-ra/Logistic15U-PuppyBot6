#include <Logistic15U_PuppyBot6.h>
#include <atomic>

std::atomic<bool> rgbBoardReady{false};

void printHelp() {
  Serial.println("\nRGB0 / RGB1 six-color calibration (raw R,G,B,C)");
  Serial.println("BOOT click = next; SW1 click = select. Release after each press.");
  Serial.println("Color menu: Red/Green/Blue/Yellow/White/Black/Save all/Sensor menu.");
  Serial.println("0 / 1 = sensor; r/g/b/y/w/k = Red/Green/Blue/Yellow/White/Black");
  Serial.println("Place the selected color at the real operating distance, then c.");
  Serial.println("c = capture 20 fresh frames; x = cancel; s = save to flash");
  Serial.println("p = export CSV; l = reload flash (discards unsaved captures); ? = help");
  Serial.println("Example: 0 r c -> hold RED steady for about 1.1 seconds.");
  Serial.println("50ms/4x: recalibrate old 2.4ms profiles. Keep lighting, LED, distance and angle fixed.");
  Serial.println("RAM* = unsaved; FLASH = saved/loaded. Partial sets can also be saved.");
}

void printSelection() {
  const auto &state = getRGBCalibrationState();
  Serial.print("Selected RGB"); Serial.print(unsigned(state.channel()));
  Serial.print(" / "); Serial.print(rgbColorName(state.color()));
  Serial.print("; completed "); Serial.print(state.completed(state.channel())); Serial.println("/6");
}

void handleCommand(char command) {
  if (command == '\r' || command == '\n' || command == ' ' || command == '\t') return;
  if (command >= 'A' && command <= 'Z') command += 'a' - 'A';
  const auto &state = getRGBCalibrationState();
  RGBChannel channel = state.channel();
  RGBColor color = state.color();
  bool selecting = true;
  switch (command) {
    case '0': channel = RGB0; break;
    case '1': channel = RGB1; break;
    case 'r': color = RGBColor::Red; break;
    case 'g': color = RGBColor::Green; break;
    case 'b': color = RGBColor::Blue; break;
    case 'y': color = RGBColor::Yellow; break;
    case 'w': color = RGBColor::White; break;
    case 'k': color = RGBColor::Black; break;
    default: selecting = false; break;
  }
  if (selecting) {
    if (selectRGBCalibration(channel, color)) printSelection();
    else Serial.println("Capture busy: wait or send x to cancel first.");
    return;
  }
  switch (command) {
    case 'c':
      if (startRGBCalibration(channel, color)) Serial.println("Capturing 20 frames: hold the selected color steady...");
      else Serial.println("Cannot start: another capture is running.");
      break;
    case 'x': cancelRGBCalibration(); Serial.println("Cancelled; previous reference retained."); break;
    case 's':
      Serial.println(saveRGBCalibration() ? "Saved calibration to flash." :
                     "Save failed: finish capture and collect at least one color first.");
      break;
    case 'l':
      Serial.println(loadRGBCalibration() ? "Loaded calibration from flash." :
                     "Load unavailable: busy or no valid flash data. RAM references retained.");
      break;
    case 'p': printRGBCalibration(Serial); break;
    case '?': printHelp(); printSelection(); break;
    default: Serial.println("Unknown command; send ? for help."); break;
  }
}

void setup() {
  Serial.begin(115200);
  puppybot_setup();
  beginRGBCalibration();  // Loads saved references, if present. No automatic flash writes.
  beginRGBCalibrationSW1();  // Uses swpin from the original board library, active LOW.
  rgbBoardReady.store(true, std::memory_order_release);
}

void loop() {
  static bool serialWasOpen = false;
  const bool serialOpen = bool(Serial);
  if (serialOpen && !serialWasOpen) { printHelp(); printSelection(); }
  serialWasOpen = serialOpen;
  while (Serial.available()) handleCommand(char(Serial.read()));
  updateRGBCalibrationSW1();  // Nonblocking debounce/short/long press detection.
  const auto before = getRGBCalibrationState().state();
  updateRGBCalibration();
  const auto after = getRGBCalibrationState().state();
  if (after != before) {
    if (after == RGBCaptureState::Captured) {
      Serial.println("Captured in RAM. Send s to save, p for raw means/min/max CSV.");
      printRGBCalibration(Serial);
    } else if (after == RGBCaptureState::Offline) Serial.println("RGB offline; old reference retained. Check cable, then c to retry.");
    else if (after == RGBCaptureState::InvalidSignal) Serial.println("Too little light or saturation; previous reference retained. Adjust placement and retry.");
    else if (after == RGBCaptureState::TimedOut) Serial.println("No fresh frames; old reference retained. Send c to retry.");
  }
  static uint32_t lastDisplay = 0;
  if (uint32_t(millis() - lastDisplay) >= 100) {
    lastDisplay = millis();
    showRGBCalibration();
  }
}

void setup1() {
  while (!rgbBoardReady.load(std::memory_order_acquire)) delay(1);
  beginRGB();
}

void loop1() {
  updateRGB();  // Nonblocking integration; service on the same core as IMU.
}
