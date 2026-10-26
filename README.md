# Logistic15U-PuppyBot6

Arduino library และแพ็กเกจบอร์ดรุ่น **0.2.0** ใช้ชื่อบอร์ด **Logistic15U-PuppyBot6**
พร้อม `main/main.ino` สำหรับตรวจความเข้ากันได้ ไม่มีโปรแกรมภารกิจ

บอร์ดใช้ Arduino-Pico 6.2.0 อ้าง RP2350B / ARM / 150 MHz / Flash 4 MB จาก
`reference/board/context.json` ของ KBIDE ไม่ใช่ผลตรวจ flash บนฮาร์ดแวร์จริง
FQBN: `logistic15u:rp2040:logistic15u`

วิธีใช้ API แต่ละกลุ่มและตัวอย่างอยู่ใน [README ของ Library](Logistic15U-PuppyBot6/README.md)

## มีอะไรใหม่ใน 0.2.0

รวมงานรุ่นพัฒนาหลัง 0.1.0 จนถึง 0.1.24-dev:

- **2WD เท่านั้น:** คำสั่งขับทั้งหมดใช้ M1/M2 เอาโหมด 4WD/Mecanum และ `setModeGyro()` ออก — [DRIVE_2WD.md](Logistic15U-PuppyBot6/DRIVE_2WD.md), [ผลตรวจ](DRIVE_2WD_VERIFICATION.md)
- **หน้าสถานะ IMU ตอนเปิดเครื่อง:** `waitIMUCalibration()` บน core 0 และ `initIMU(USE_IMU)` บน core 1 พร้อม timeout และ SW1 ข้าม — [IMU_STARTUP.md](Logistic15U-PuppyBot6/IMU_STARTUP.md), [ผลตรวจ](IMU_STARTUP_VERIFICATION.md)
- **เมนู `wait_SW1()`:** Start / ADC / Mux 8 Front/Back / IMU + Encoder / Calibrate RGB / Calibrate Line (BOOT = ถัดไป, SW1 = เลือก) — [ผลตรวจ](WAIT_SW1_VERIFICATION.md)
- **Encoder สองตัว:** เริ่มและรีเซ็ตแยกตัว — [EncoderDual](Logistic15U-PuppyBot6/examples/EncoderDual/README.md), [ผลตรวจ](ENCODER_VERIFICATION.md)
- **Calibrate Line:** วัดพื้น/เส้นแยก Front/Back บันทึก Flash โหลดอัตโนมัติ — [LINE_CALIBRATION.md](Logistic15U-PuppyBot6/LINE_CALIBRATION.md), [ผลตรวจ](LINE_CALIBRATION_VERIFICATION.md)
- **ตามเส้น PD:** ตัวกรอง D และเพดานแรงเลี้ยวร่วมกัน (`setLinePDOptions`), หน้าดูค่า PD บน TFT — [LINE_PD_MONITOR.md](Logistic15U-PuppyBot6/LINE_PD_MONITOR.md), [ผลตรวจ](LINE_PD_MONITOR_VERIFICATION.md)
- **PI ความเร็วล้อ:** `Run_PID_Encoder` / `Run_PID_B_Encoder` และวิ่งตามพัลส์ `Run_PID_Pulses` / `Run_PID_B_Pulses` พร้อมตัวอย่าง WheelSpeedMeasure — [WHEEL_SPEED_CONTROL.md](Logistic15U-PuppyBot6/WHEEL_SPEED_CONTROL.md), [ผลตรวจ](WHEEL_SPEED_VERIFICATION.md)
- **RGB0/RGB1:** อ่านสองตัว, สอบเทียบ 6 สีบน TFT บันทึก Flash v2, แยกสี (`getRGBColor`, `classifyRGB`), โหมด `beginRGBMode()` — [RGBMonitor](Logistic15U-PuppyBot6/examples/RGBMonitor/README.md), [RGBCalibration](Logistic15U-PuppyBot6/examples/RGBCalibration/README.md), [RGBClassify](Logistic15U-PuppyBot6/examples/RGBClassify/README.md), [ผลตรวจ](RGB_CALIBRATION_VERIFICATION.md)
- **Servo:** แก้ clock PWM เป็น 50 Hz, `servoStop(channel)`, `setServoPulseRange()` — [SERVO_API.md](Logistic15U-PuppyBot6/SERVO_API.md), [ผลตรวจ](SERVO_VERIFICATION.md)
- แก้การถอดค่า RGB ที่ชื่อ `word()` ชน macro ของ Arduino จนแสดง R=2, G=4, B=6, Clear=0 — [RGB_WORD_FIX_VERIFICATION.md](RGB_WORD_FIX_VERIFICATION.md)

**ย้ายจาก 0.1.0:** พัลส์ของมุม Servo เดิมเปลี่ยนหลังแก้ clock ต้องตรวจมุมใหม่ และต้องลบ `setModeGyro()` กับคำสั่ง 4WD ออกจากสเก็ตช์ ดู [DRIVE_2WD.md](Logistic15U-PuppyBot6/DRIVE_2WD.md)

## ติดตั้งใน Arduino IDE 2.x

1. เปิด Preferences → Additional Boards Manager URLs แล้วเพิ่มทั้งสอง URL:

   ```text
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json
   ```

2. ใน Boards Manager ติดตั้ง **Raspberry Pi Pico/RP2040/RP2350 by Earle F. Philhower, III** รุ่น **6.2.0** ก่อน แล้วติดตั้ง **Logistic15U-PuppyBot6** รุ่น **0.2.0** แพ็กเกจนี้อ้าง core และเครื่องมือของ Arduino-Pico จึงไม่คัดลอกไฟล์ซ้ำ
3. ดาวน์โหลด [Library ZIP](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.2.0.zip) แล้วเลือก Sketch → Include Library → Add .ZIP Library หรือใช้ไฟล์ `dist/Logistic15U-PuppyBot6-0.2.0.zip` ใน repository ถ้าเคยติดตั้ง 0.1.0 ให้ลบโฟลเดอร์ Lib เดิมใน `Documents/Arduino/libraries` ก่อน
4. ติดตั้ง dependency ด้านล่างผ่าน Library Manager ตามรุ่นที่ระบุ การเพิ่ม ZIP ไม่ได้ติดตั้ง dependency ให้อัตโนมัติ หาก Manager เลือกรุ่นใหม่ให้ ให้เปลี่ยนกลับเป็นรุ่นในตารางก่อน Verify
5. เลือกบอร์ด **Logistic15U-PuppyBot6** เปิด `main/main.ino` แล้วกด Verify หรือเปิด File → Examples → Logistic15U-PuppyBot6 → OriginalMain

| Library Manager name | รุ่นที่ทดสอบ |
|---|---|
| Adafruit GFX Library | 1.12.6 |
| Adafruit ST7735 and ST7789 Library | 1.11.0 |
| Adafruit TCS34725 | 1.4.4 |
| Adafruit MCP3008 | 1.3.3 |
| Adafruit BusIO | 1.17.4 |
| rp2040-encoder-library | 0.2.0 |
| SparkFun 9DoF IMU Breakout - ICM 20948 - Arduino Library | 1.3.2 |

**Lib URL เป็น URL ดาวน์โหลด ZIP** ส่วน `_index.json` ใช้กับ Boards Manager
ไลบรารีนี้ยังไม่ได้ลงทะเบียนใน Arduino Library Manager กลาง จึงยังค้นชื่อเพื่อติดตั้งโดยตรงไม่ได้
Wire, SPI, Pico SDK และ toolchain มาจาก Arduino-Pico ไม่ต้องติดตั้ง Servo เพิ่ม

## ใช้ API

```cpp
#include <Logistic15U_PuppyBot6.h>
```

ชื่อ API เดิม เช่น `puppybot_setup()`, `ao()`, `initIMU()` และ helper ยังอยู่
`main` เรียกเริ่มต้นอุปกรณ์ทั้งสอง core และเริ่มต้นเอาต์พุต servo ด้วย จึงต้องตรวจสายและสอบเทียบก่อนอัปโหลด
ค่าขา มุม servo และ PID ในซอร์สเดิมไม่ใช่ค่าที่สอบเทียบแล้วของหุ่น Logistics ตัวนี้

เวอร์ชันนี้คงโครงสร้าง header เดิม: include API เฉพาะ translation unit หลัก ยังไม่รองรับ include จากหลายไฟล์ `.cpp`
อ่านการเปลี่ยน IMU และข้อจำกัดใน [PORTING.md](PORTING.md)

## ติดตั้งด้วย CLI

ต้องมี Arduino CLI >= 1.0.4; ทดสอบด้วย CLI 1.5.1 ที่มากับ Arduino IDE
เมื่อ URL เผยแพร่แล้ว รันจาก repository:

```powershell
.\scripts\install.ps1 -ArduinoCli "$env:LOCALAPPDATA\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
```

สคริปต์ติดตั้ง core, บอร์ด, dependency ตามรุ่น และ Library ZIP จาก repository แล้วคอมไพล์โดยไม่อัปโหลด
ระบุ `-ConfigFile` ได้เพื่อใช้ sketchbook แยก สคริปต์อาจเปลี่ยนรุ่น dependency ที่ติดตั้งไว้ก่อนหน้าให้ตรงตาราง

## ไฟล์สำคัญ

- `Logistic15U-PuppyBot6/`: ซอร์ส library, metadata, เอกสาร และตัวอย่าง
- `hardware/logistic15u/rp2040/`: บอร์ดที่อ้าง Arduino-Pico
- `dist/`: ZIP library, ZIP บอร์ด และ manifest พร้อม SHA-256
- `package_logistic15u_index.json`: Boards Manager index
- `scripts/build_release.py`: สร้าง ZIP และ index ซ้ำด้วย Python standard library
- `scripts/verify_distribution.py`: ตรวจ ZIP, checksum และ index กับซอร์ส
- `VERIFICATION.md` และ `*_VERIFICATION.md`: ผลการติดตั้ง คอมไพล์ และทดสอบซอฟต์แวร์แต่ละส่วน
- `reference/original-main/main/main.ino`: ตัวอย่างเดิมที่ไม่แก้ไข
- `COPY_MANIFEST.json`: หลักฐานขั้นคัดลอกเดิม ไม่ใช่ manifest ปัจจุบัน

ไม่ได้แจกจ่าย InvenSense SDK `Invn` เดิม ใช้ SparkFun จาก Library Manager แทน
dependency มาตรฐานและ build cache ไม่อยู่ใน ZIP ดูที่มาและสิทธิ์ใน [NOTICE.md](Logistic15U-PuppyBot6/NOTICE.md)

หากเคยคอมไพล์แล้วเลือก Adafruit ST7735 1.0.8 จาก cache จนพบ `setSPISpeed` ไม่มีใน class ให้คอมไพล์ด้วย CLI `--clean` ตาม VERIFICATION.md; ไม่ต้องลบไลบรารีเดิมของโครงการอื่น
