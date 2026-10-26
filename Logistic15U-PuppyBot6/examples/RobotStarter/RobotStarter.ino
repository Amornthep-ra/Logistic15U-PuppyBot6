#include <Logistic15U_PuppyBot6.h>
#include <atomic>
#include "RobotSetup.h"
#include "RobotActions.h"

// false = ตั้งใจใช้งานโดยไม่ติดตั้ง IMU
constexpr bool USE_IMU = true;

std::atomic<bool> rgbBoardReady{ false };
int encoder1Pulses = 0;
int encoder2Pulses = 0;

void setup() {
  Serial.begin(115200);
  puppybot_setup();
  rgbBoardReady.store(true, std::memory_order_release);
  RobotSetup::beginEncoders();
  waitIMUCalibration();  // หน้าสถานะ IMU; ถ้ามีปัญหา SW1 = ใช้ต่อโดยไม่มี IMU

  lift_down(); //สั่ง Servo ยกลิฟลง
  wait_SW1();  // BOOT ถัดไป, SW1 เลือก; Calibrate RGB/Line -> Save / Exit; Start ทำงานต่อ

  // RGB: แสดงชื่อสีบนจอหลัง Start ต้องเปิดคู่กับ updateRGBMode() ใน loop()
  // ใช้ค่า Calibrate RGB ใน RAM ถ้าเพิ่งสอบเทียบ ไม่เช่นนั้นโหลดจาก Flash; ห้ามเปิดพร้อม LinePD Monitor
  // อ่านสีในโค้ดภารกิจ: loadRGBCalibration(); แล้ว getRGBColor(RGB0) -> RGBColor::Red ... / Unknown
  // ดู README.md หัวข้อ 7 RGB
  // beginRGBMode(RGB_MODE_SHOW);

  RobotSetup::resetEncoders();  // เริ่มนับใหม่หลังเลือก Start ด้วย SW1
  RobotSetup::beginWheelSpeedControl();  // ตั้งค่าที่วัดจริงใน RobotSetup.h; ยังไม่ครบใช้ PD เดิม

  // PID ตามเส้นหน้า: ต้อง Calibrate Line (Front) และ Save ก่อน ไม่เช่นนั้นไม่วิ่ง
  // คืน true = จบตามเงื่อนไข, false = ตั้งค่าผิด/หมดเวลา/Encoder fault (มอเตอร์หยุดทุกกรณี)
  // เลือกเปิดทีละคำสั่ง; ดู README.md หัวข้อ 6 ตามเส้นด้วย PID (ตารางพารามิเตอร์และแนวทางจูน)
  //
  // แบบ 1 เจอแยกหยุดทันที: speed, Kp, Kd, เกณฑ์แยก (0-800), Ramp ms (0 = ปิด), timeout ms
  // lineFrontCross(30, 0.09f, 0.05f, 400, 300, 10000);
  //
  // แบบ 2 เจอแยกไม่หยุด -> วิ่งต่อตามพัลส์เฉลี่ยสองล้อ -> Ramp Down -> ครบหยุด
  // speed, Kp, Kd, เกณฑ์แยก, พัลส์หลังเจอแยก, Ramp ms, Ramp Down พัลส์ (0 = ปิด), ความเร็วตอนครบ, timeout ms
  // lineFrontCrossPulses(30, 0.085f, 0.05f, 400, 540, 300, 200, 10, 10000); // ทดสอบแล้ว: 200 พัลส์สุดท้ายลด 30 -> 10
  // lineFrontCrossPulses(50, 0.1f, 0.00f, 400, 540, 300, 200, 10, 10000);   // ทดลองความเร็ว 50 ยังไม่ยืนยัน: ลด 50 -> 10
  // ao();  // หยุดมอเตอร์หลังจบคำสั่ง

}

void loop() {
  encoder1Pulses = PuppyBotIMU::get_pulse_Encoder(1);
  encoder2Pulses = PuppyBotIMU::get_pulse_Encoder(2);
  // updateRGBMode();  // วาดหน้าชื่อสี คู่กับ beginRGBMode(...) ใน setup(); ดู README.md หัวข้อ 7.4
}

void setup1() {
  while (!rgbBoardReady.load(std::memory_order_acquire)) delay(1);
  if (initIMU(USE_IMU)) {
    updateIMU_fine_offset_Yaw(300);
    offset_yaw_loop1 = getOffsetYaw();
  }
  beginRGB();  // เริ่ม RGB0/RGB1; เริ่มและอ่าน I2C ต้องอยู่ core 1 เดียวกับ IMU (README.md หัวข้อ 7.1)
}

void loop1() {
  if (isIMUCalibrated()) {
    updateIMU();
    pitch_loop1 = getPitchCached();
    roll_loop1 = getRollCached();
    yaw_loop1 = getYawCached();
    updateContinuousYawCached();
    Con_yaw_loop1 = continuousYaw + Ref_offset_Con;
    if (status_resetYaw == 1) {
      status_resetYaw = 0;
      resetContinuousYaw();
    }
    if (status_resetYaw180 == 1) {
      resetGyro();
      status_resetYaw180 = 0;
    }
  }
  // อ่าน RGB แบบไม่รอ integration แล้วเก็บ cache; core 0 อ่านต่อด้วย getRGB(RGB0) / getRGBColor(RGB0)
  updateRGB();
}
