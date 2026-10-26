# Logistic15U-PuppyBot6 Library

Arduino library สำหรับบอร์ด Logistic15U-PuppyBot6 (RP2350B) รวม API มอเตอร์ 2WD, Servo, จอ TFT, เซนเซอร์เส้น, RGB, Encoder และ IMU ดัดแปลงจาก PuppyBot6-New ไม่มีโปรแกรมภารกิจแข่งขันในตัว

- รุ่น: `0.2.0`
- Header: `#include <Logistic15U_PuppyBot6.h>`
- บอร์ด: `Logistic15U-PuppyBot6` (FQBN `logistic15u:rp2040:logistic15u`) บน Arduino-Pico `6.2.0`
- Repository: https://github.com/Amornthep-ra/Logistic15U-PuppyBot6

## ติดตั้ง

1. Arduino IDE → Preferences → Additional Boards Manager URLs เพิ่ม:

   ```text
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json
   ```

2. Boards Manager: ติดตั้ง **Raspberry Pi Pico/RP2040/RP2350 by Earle F. Philhower, III** รุ่น **6.2.0** แล้วติดตั้ง **Logistic15U-PuppyBot6** รุ่น **0.2.0**
3. ดาวน์โหลด [Library ZIP](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.2.0.zip) แล้วเลือก Sketch → Include Library → Add .ZIP Library
4. ติดตั้ง dependency ผ่าน Library Manager ตามรุ่นในตาราง (ZIP ไม่ได้ติดตั้งให้อัตโนมัติ)

| Library Manager name | รุ่นที่ทดสอบ |
| --- | --- |
| Adafruit GFX Library | 1.12.6 |
| Adafruit ST7735 and ST7789 Library | 1.11.0 |
| Adafruit TCS34725 | 1.4.4 |
| Adafruit MCP3008 | 1.3.3 |
| Adafruit BusIO | 1.17.4 |
| rp2040-encoder-library | 0.2.0 |
| SparkFun 9DoF IMU Breakout - ICM 20948 - Arduino Library | 1.3.2 |

## โครงสร้างสเก็ตช์ขั้นต่ำ

core 0 รันงานหลัก เมนู จอ มอเตอร์ และ Encoder; core 1 อ่าน IMU และ RGB (I2C) แล้วเก็บค่าในตัวแปร/cache ให้ core 0 อ่าน

```cpp
#include <Logistic15U_PuppyBot6.h>
#include <atomic>

constexpr bool USE_IMU = true;          // false = ไม่ได้ติดตั้ง IMU
std::atomic<bool> boardReady{ false };

void setup() {
  Serial.begin(115200);
  puppybot_setup();
  boardReady.store(true, std::memory_order_release);
  PuppyBotIMU::init_encoders();
  waitIMUCalibration();   // หน้าสถานะ IMU
  wait_SW1();             // เมนู: Start / ดูค่า / Calibrate RGB / Calibrate Line
  // คำสั่งภารกิจ
}

void loop() {}

void setup1() {
  while (!boardReady.load(std::memory_order_acquire)) delay(1);
  if (initIMU(USE_IMU)) {
    updateIMU_fine_offset_Yaw(300);
    offset_yaw_loop1 = getOffsetYaw();
  }
  beginRGB();
}

void loop1() {
  if (isIMUCalibrated()) {
    updateIMU();
    pitch_loop1 = getPitchCached();
    roll_loop1 = getRollCached();
    yaw_loop1 = getYawCached();
    updateContinuousYawCached();
    Con_yaw_loop1 = continuousYaw + Ref_offset_Con;
    if (status_resetYaw == 1) { status_resetYaw = 0; resetContinuousYaw(); }
    if (status_resetYaw180 == 1) { resetGyro(); status_resetYaw180 = 0; }
  }
  updateRGB();
}
```

include header นี้จาก translation unit หลัก (.ino) เท่านั้น ยังไม่รองรับการ include จากหลายไฟล์ `.cpp`

## กลุ่ม API

| กลุ่ม | คำสั่งหลัก | เอกสาร |
| --- | --- | --- |
| เมนูและปุ่ม | `wait_SW1()` (BOOT = ถัดไป, SW1 = เลือก) | — |
| IMU | `waitIMUCalibration()`, `Con_yaw_loop1`, `status_resetYaw`, `turnByAngle()`, `turnPID()`, `moveStraightPID*()` | [IMU_STARTUP.md](IMU_STARTUP.md), [DRIVE_2WD.md](DRIVE_2WD.md) |
| มอเตอร์ 2WD | `motor(1/2, -100..100)`, `ao()` | [DRIVE_2WD.md](DRIVE_2WD.md) |
| Encoder | `PuppyBotIMU::get_pulse_Encoder(1/2)`, `resetEncoder(1/2)` | [examples/EncoderDual](examples/EncoderDual/README.md) |
| สอบเทียบเส้น | เมนู `6. Calibrate Line`, `hasLineCalibration()` | [LINE_CALIBRATION.md](LINE_CALIBRATION.md) |
| ตามเส้น PD | `Run_PID()`, `Run_PID_B()`, `readline()`, `Read_sumValue_sensor()`, `setLinePDOptions()` | [LINE_PD_MONITOR.md](LINE_PD_MONITOR.md) |
| PI ความเร็วล้อ | `beginWheelSpeedControl()`, `Run_PID_Encoder()`, `Run_PID_Pulses()`, `stopWheelSpeedControl()` | [WHEEL_SPEED_CONTROL.md](WHEEL_SPEED_CONTROL.md) |
| RGB | `beginRGB()`, `updateRGB()`, `getRGB()`, `getRGBColor()`, `classifyRGB()`, `beginRGBMode()` | [examples/RGBMonitor](examples/RGBMonitor/README.md), [examples/RGBCalibration](examples/RGBCalibration/README.md), [examples/RGBClassify](examples/RGBClassify/README.md) |
| Servo | `servoRun()`, `servoStop()`, `servoSweep()`, `setServoPulseRange()` | [SERVO_API.md](SERVO_API.md) |

## ตัวอย่าง

File → Examples → Logistic15U-PuppyBot6

| ตัวอย่าง | ใช้ทำอะไร |
| --- | --- |
| `OriginalMain` | โครงเดิมจาก PuppyBot6-New สำหรับตรวจว่าคอมไพล์ผ่าน |
| `EncoderDual` | อ่าน Encoder 1/2 ผ่าน Serial และรีเซ็ตแยกตัว |
| `WheelSpeedMeasure` | วัดเครื่องหมายพัลส์และ ticks/s ของแต่ละล้อ เพื่อกรอก `WheelSpeedSettings` |
| `RGBMonitor` | แสดงค่า RGB0/RGB1 บน TFT |
| `RGBCalibration` | สอบเทียบ 6 สีแบบ standalone (SW1 และ Serial) |
| `RGBClassify` | แยกสีจากค่าที่สอบเทียบและแสดงชื่อสี |

## ข้อจำกัด

- ขาอุปกรณ์ ช่วงพัลส์ Servo (300–3000 µs) และค่า PID เริ่มต้นมาจากซอร์สเดิม ยังไม่ได้ยืนยันกับหุ่นจริง ต้องตรวจสายและสอบเทียบก่อนใช้งาน
- ทิศเลี้ยวซ้าย/ขวาของ Line PD และคำสั่ง IMU ขึ้นกับการต่อสายของหุ่นแต่ละตัว ต้องทดสอบก่อนวิ่ง
- การแยกสีทดสอบด้วยข้อมูลจำลองในซอฟต์แวร์ ยังไม่มีผลยืนยันความแม่นยำบนหุ่นจริง
- ยังไม่ได้ลงทะเบียนใน Arduino Library Manager กลาง ต้องติดตั้งจาก ZIP

## ที่มาและสิทธิ์

ดู [NOTICE.md](NOTICE.md) โค้ด API มาจาก PuppyBot6-New ของ KBIDE รุ่น 3.5.8 และการจัดแพ็กเกจนี้ไม่ได้ให้สิทธิ์ใหม่เหนือโค้ดต้นฉบับหรือของบุคคลที่สาม ไม่มีการแจกจ่าย InvenSense SDK เดิม
