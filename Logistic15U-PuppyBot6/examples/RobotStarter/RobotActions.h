#pragma once
#include <Logistic15U_PuppyBot6.h>

// ใช้ช่อง มุม และเวลาจากโค้ดเดิมของผู้ใช้
inline void lift_up() {
  servoRun(0, 10);
  delay(300);
  servoStop(0);
}

inline void lift_down(){
   servoRun(0, 90);
}

// เพิ่มฟังก์ชันการทำงานอื่น ๆ ของหุ่นไว้ด้านล่างนี้

inline void show_line_drive_status(const char *mode, const char *detail) {
  tft_.setFont(nullptr); tft_.setTextSize(1); tft_.setTextWrap(false);
  tft_.fillScreen(ST77XX_BLACK);
  const char *lines[] = {mode, detail};
  for (int i = 0; i < 2; ++i) {
    char text[28]; snprintf(text, sizeof(text), "%.*s", (tft_.width() - 8) / 6, lines[i]);
    tft_.setTextColor(i == 0 ? ST77XX_CYAN : ST77XX_YELLOW, ST77XX_BLACK);
    tft_.setCursor((tft_.width() - int(strlen(text)) * 6) / 2, 30 + i * 18);
    tft_.print(text);
  }
}

// ตามเส้นหน้าจนเจอแยก; afterPulses > 0 = ไม่หยุดที่แยก วิ่งต่อจนค่าเฉลี่ยพัลส์ 2 ล้อครบแล้วหยุด
// true = จบตามเงื่อนไข, false = ไม่พร้อม/หมดเวลา/Encoder fault; timeoutMs นับรวมทั้งเที่ยว
// rampMs: ค่อย ๆ เพิ่มความเร็วฐาน 0 -> speed ตอนออกตัว; 0 = ปิด Ramp
// rampDownPulses: ช่วงท้ายก่อนครบ afterPulses ลดความเร็ว speed -> minSpeed; 0 = ปิด Ramp Down
inline bool lineFrontCrossRun(int speed, float kp, float kd, int crossThreshold, long afterPulses,
                              unsigned long timeoutMs, uint32_t rampMs,
                              long rampDownPulses = 0, int minSpeed = 0) {
  ao();
  if (!hasLineCalibration(LineSide::Front) || Sensor_mode_F != 1 || PID_NumPin != 8) {
    Serial.println("Front line: check calibration/settings");
    return false;
  }
  if (speed <= 0 || speed > 100 || crossThreshold <= 0 || crossThreshold > 800 || timeoutMs == 0 ||
      afterPulses < 0 || rampDownPulses < 0 || rampDownPulses > afterPulses ||
      (rampDownPulses > 0 && (minSpeed <= 0 || minSpeed > speed)) || !isfinite(kp) || !isfinite(kd) || kp < 0 || kd < 0) {
    Serial.println("Front line: check calibration/settings");
    return false;
  }

  // เลือกโหมดครั้งเดียวต่อเที่ยว: หาก Encoder มีปัญหาระหว่างวิ่ง จะหยุด ไม่สลับไป PD เดิม
  const bool encoderMode = isWheelSpeedControlConfigured();
  const char *modeName = encoderMode ? "ENCODER + LINE PD" : "LINE PD ONLY";
  if (!isLinePDMonitorActive())
    show_line_drive_status(modeName, encoderMode ? "READY" : "ENCODER NOT SET");
  if (encoderMode && !restartWheelSpeedControl()) {
    return false;
  }
  const uint32_t startedAt = millis();
  previous_error = readline() - ((PID_NumPin - 1) * 100) / 2;
  bool crossed = false;
  int32_t crossPulse1 = 0, crossPulse2 = 0;
  while (millis() - startedAt < timeoutMs) {
    if (!crossed && Read_sumValue_sensor() >= crossThreshold) {
      crossed = true;
      crossPulse1 = PuppyBotIMU::get_pulse_Encoder(1);
      crossPulse2 = PuppyBotIMU::get_pulse_Encoder(2);
      Serial.println(afterPulses > 0 ? "Front crossing: GO PULSES" : "Front crossing: STOP");
    }
    const long traveled = crossed ? PuppyBotEncoderDriveDetail::travelPulses(crossPulse1, crossPulse2) : 0;
    if (crossed && traveled >= afterPulses) {
      if (encoderMode) stopWheelSpeedControl(); else ao();
      if (!isLinePDMonitorActive())
        show_line_drive_status(afterPulses > 0 ? "FRONT PULSES: STOP" : "FRONT CROSS: STOP", modeName);
      if (afterPulses > 0) {
        Serial.print("Front pulses: STOP at ");
        Serial.println(traveled);
      }
      return true;
    }
    const uint32_t elapsed = uint32_t(millis() - startedAt);
    if (elapsed >= timeoutMs) break;
    int runSpeed = rampMs != 0 && elapsed < rampMs
        ? int(uint64_t(speed) * elapsed / rampMs) : speed;
    const long remaining = afterPulses - traveled;
    if (crossed && rampDownPulses > 0 && remaining < rampDownPulses) {
      const int downSpeed = minSpeed + int(int64_t(speed - minSpeed) * remaining / rampDownPulses);
      if (downSpeed < runSpeed) runSpeed = downSpeed;
    }
    if (runSpeed == 0) {
      delay(1);  // รอรอบแรกของ Ramp; ยังตรวจแยกและ timeout ทุกรอบ
      continue;
    }
    if (encoderMode) {
      if (!Run_PID_Encoder(runSpeed, kp, kd)) {
        stopWheelSpeedControl();
        if (!isLinePDMonitorActive())
          show_line_drive_status("ENCODER STOP", PuppyBotDrive::wheelSpeedStatusName(getWheelSpeedTelemetry().status));
        Serial.println("Encoder wheel control fault: STOP");
        return false;
      }
    } else Run_PID(runSpeed, kp, kd);
  }
  if (encoderMode) stopWheelSpeedControl(); else ao();
  if (!isLinePDMonitorActive())
    show_line_drive_status("FRONT TIMEOUT: STOP", modeName);
  Serial.println("Front line timeout: STOP");
  return false;
}

// ตามเส้นหน้าจนเจอแยกแล้วหยุด; true = เจอแยก, false = ไม่พร้อม/หมดเวลา/Encoder fault
inline bool lineFrontCross(int speed, float kp, float kd, int crossThreshold,
                           uint32_t rampMs = 300, unsigned long timeoutMs = 10000) {
  return lineFrontCrossRun(speed, kp, kd, crossThreshold, 0, timeoutMs, rampMs);
}

// ตามเส้นหน้าจนเจอแยก -> ไม่หยุด -> วิ่งต่อ afterPulses (นับจากจุดเจอแยก) -> Ramp Down -> ครบแล้วหยุด
// rampDownPulses = ช่วงลดความเร็วก่อนครบ (0 = ปิด), minSpeed = ความเร็วตอนครบพัลส์
inline bool lineFrontCrossPulses(int speed, float kp, float kd, int crossThreshold, long afterPulses,
                                 uint32_t rampMs = 300, long rampDownPulses = 0, int minSpeed = 10,
                                 unsigned long timeoutMs = 10000) {
  if (afterPulses <= 0) {
    Serial.println("Front line: check calibration/settings");
    return false;
  }
  return lineFrontCrossRun(speed, kp, kd, crossThreshold, afterPulses, timeoutMs, rampMs,
                           rampDownPulses, minSpeed);
}
