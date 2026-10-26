# RobotStarter

ตัวอย่างโค้ดตั้งต้นสำหรับ Logistics Robot รุ่นอายุไม่เกิน 15 ปี ใช้บอร์ด `Logistic15U-PuppyBot6` กับ Arduino IDE และ Lib รุ่น `0.2.1` ไดรฟ์แบบ **2WD เท่านั้น** (M1/M2)

เปิดจาก **File → Examples → Logistic15U-PuppyBot6 → RobotStarter** แล้วกด **File → Save As** เก็บเป็นสเก็ตช์ของตัวเองก่อนแก้ IDE จะคัดลอกทั้ง `.ino`, `RobotSetup.h` และ `RobotActions.h` ไปด้วย ตัวอย่างต้นฉบับใน Lib จะไม่ถูกแก้

> **ค่าใน `RobotSetup.h` วัดจากหุ่นต้นแบบตัวเดียว** หุ่นแต่ละตัวต้องวัด `encoderSign` และ `referenceTicksPerSecond` ใหม่ด้วยตัวอย่าง WheelSpeedMeasure ([หัวข้อ 5.3](#53-ค่าที่วัดจริง-2026-10-22))

## สารบัญ

1. [เริ่มต้นใช้งาน](#1-เริ่มต้นใช้งาน)
2. [โครงสร้างไฟล์และลำดับการทำงาน](#2-โครงสร้างไฟล์และลำดับการทำงาน)
3. [เมนู wait_SW1 และปุ่ม](#3-เมนู-wait_sw1-และปุ่ม)
4. [IMU](#4-imu)
5. [Encoder](#5-encoder)
6. [ตามเส้นด้วย PID](#6-ตามเส้นด้วย-pid)
7. [RGB](#7-rgb)
8. [Servo](#8-servo)
9. [เพิ่มคำสั่งเองใน RobotActions.h](#9-เพิ่มคำสั่งเองใน-robotactionsh)
10. [ข้อมูลที่ต้องวัดจากหุ่นจริง](#10-ข้อมูลที่ต้องวัดจากหุ่นจริง)
11. [ประวัติรุ่น](#11-ประวัติรุ่น)

---

## 1. เริ่มต้นใช้งาน

| รายการ | ค่าที่ใช้ |
| --- | --- |
| ชื่อบอร์ดใน Arduino IDE | `Logistic15U-PuppyBot6` |
| FQBN | `logistic15u:rp2040:logistic15u` |
| Arduino-Pico core | `6.2.0` |
| Lib / แพ็กเกจบอร์ด | `0.2.1` |
| Header หลัก | `#include <Logistic15U_PuppyBot6.h>` |
| Repository | https://github.com/Amornthep-ra/Logistic15U-PuppyBot6 |

วิธีติดตั้งบอร์ด Lib และ dependency อยู่ใน [README ของ Lib](../../README.md)

**ขั้นตอนครั้งแรก**

1. อัปโหลดสเก็ตช์ แล้วรอหน้าสถานะ IMU จนขึ้น `IMU READY` ([หัวข้อ 4.1](#41-หน้าสถานะ-imu-ตอนเปิดเครื่อง))
2. ในเมนู `wait_SW1()` เลือก **6. Calibrate Line** → Front → Save ([หัวข้อ 6.1](#61-สอบเทียบเซนเซอร์เส้น-calibrate-line))
3. ถ้าจะใช้สี ให้เลือก **5. Calibrate RGB** → Save all ([หัวข้อ 7.2](#72-สอบเทียบ-rgb-ผ่านเมนู))
4. เลือก **1. Start** หุ่นจะทำคำสั่งใน `setup()` หลัง `wait_SW1()`

ค่าที่ Save ไว้ใช้ต่อได้หลังปิดเครื่อง ไม่ต้องสอบเทียบใหม่ทุกครั้ง ยกเว้นเปลี่ยนแสง ความสูงเซนเซอร์ หรือสนาม

---

## 2. โครงสร้างไฟล์และลำดับการทำงาน

| ไฟล์ | หน้าที่ |
| --- | --- |
| `RobotStarter.ino` | ลำดับงานหลัก `setup()`/`loop()` บน core 0 และ IMU/RGB ใน `setup1()`/`loop1()` บน core 1 |
| `RobotSetup.h` | ตั้งค่าอุปกรณ์ของหุ่นตัวนี้: Encoder และค่าความเร็วล้อที่วัดจริง |
| `RobotActions.h` | คำสั่งที่ตั้งชื่อเอง: `lift_up()`, `lift_down()`, `lineFrontCross()`, `lineFrontCrossPulses()` |

ทั้งสามไฟล์ต้องอยู่ในโฟลเดอร์เดียวกัน แก้ `RobotSetup.h`/`RobotActions.h` ได้โดยไม่ต้องแก้ Lib

### ลำดับใน .ino ปัจจุบัน

```cpp
void setup() {
  Serial.begin(115200);
  puppybot_setup();
  rgbBoardReady.store(true, std::memory_order_release);  // ปล่อยให้ core 1 เริ่ม IMU/RGB
  RobotSetup::beginEncoders();
  waitIMUCalibration();                 // หน้าสถานะ IMU
  lift_down();                          // Servo ช่อง 0 ไปมุม 90
  wait_SW1();                           // เมนู; เลือก Start เพื่อไปต่อ

  RobotSetup::resetEncoders();
  RobotSetup::beginWheelSpeedControl();
  // lineFrontCrossPulses(30, 0.085f, 0.05f, 400, 540, 300, 200, 10, 10000);  // คอมเมนต์ปิดไว้
  // ao();
}
```

| ส่วน | การทำงาน |
| --- | --- |
| `setup()` | ตามโค้ดด้านบน ตอนนี้คำสั่งวิ่ง (`lineFrontCross`/`lineFrontCrossPulses`) และ `ao()` คอมเมนต์ปิดทั้งหมด เลือก Start แล้วหุ่นยังไม่วิ่ง เปิดทีละคำสั่งตาม[หัวข้อ 6.2](#62-คำสั่งตามเส้นหน้าใน-robotactionsh) |
| `loop()` | อ่านพัลส์ Encoder ลง `encoder1Pulses`, `encoder2Pulses` (`updateRGBMode()` คอมเมนต์ปิดไว้) |
| `setup1()` | รอ `rgbBoardReady` → `initIMU(USE_IMU)` → ตั้ง offset yaw 300 รอบ → `beginRGB()` |
| `loop1()` | อัปเดต IMU และตัวแปรมุม จัดการธงรีเซ็ต yaw และ `updateRGB()` |

คง `rgbBoardReady.store(...)` ไว้ก่อน `waitIMUCalibration()` และ `wait_SW1()` เสมอ ไม่เช่นนั้น core 1 จะไม่เริ่ม IMU/RGB

---

## 3. เมนู wait_SW1 และปุ่ม

**BOOT กดแล้วปล่อย = ถัดไป / SW1 กดแล้วปล่อย = เลือก** มีเสียงคนละโทน กดค้างไม่ทำซ้ำ กดสองปุ่มซ้อนกันจะรอให้ปล่อยทั้งคู่ก่อน

| ลำดับ | รายการ | การทำงาน |
| --- | --- | --- |
| 1 | Start | ออกจาก `wait_SW1()` ไปทำคำสั่งถัดไป |
| 2 | ADC A0–A15 | ดูค่าดิบทั้ง 16 ช่อง |
| 3 | Mux 8 Sensor Front/Back | ดู F0–F7 และ B0–B7 |
| 4 | IMU และ Encoder 1/2 | ดู Pitch, Roll, Yaw, Yaw+ (`Con_yaw_loop1`) และพัลส์ดิบ ENC1/ENC2 |
| 5 | Calibrate RGB | เลือก RGB0/RGB1 สอบเทียบ บันทึก กลับเมนูหลัก |
| 6 | Calibrate Line | เลือก Front/Back วัดพื้นและเส้น บันทึกแยกแผง |

- Start ถูกเลือกเป็นรายการแรกเมื่อเข้าเมนู
- ในหน้าดูค่า ADC/Mux/IMU: BOOT วนสามหน้าดูค่า SW1 เลือก Exit กลับเมนูหลัก (ไม่เริ่มโปรแกรม ต้องเลือก Start เอง)
- เมนูอ่านมุมจากตัวแปรที่ `loop1()` อัปเดต ไม่เริ่ม IMU ซ้ำ การดู Encoder ไม่รีเซ็ตพัลส์ การดู Mux ไม่เปลี่ยน `Sensor_mode_F`/`Sensor_mode_B`
- เมนูไม่สั่งมอเตอร์หรือ Servo `wait_SW1_one()` และ `wait_SW1_ALL()` ยังใช้ได้ตามเดิม

---

## 4. IMU

### 4.1 หน้าสถานะ IMU ตอนเปิดเครื่อง

`waitIMUCalibration()` ใน `setup()` แสดง **CONNECTING → CALIBRATING IMU → IMU READY** พร้อมเวลาและแถบความคืบหน้า

- สำเร็จ = เสียงสูงสั้นหนึ่งครั้ง; มีปัญหา/timeout เกิน 15 วินาที = เสียงต่ำยาวและค้างแจ้งปัญหา
- เมื่อมีปัญหา: ตรวจสายแล้วเปิดใหม่ หรือกด SW1 เพื่อใช้ต่อโดยไม่มี IMU
- 3 วินาทีของการตั้ง offset คือช่วงตั้งอ้างอิง Yaw ไม่ใช่การยืนยันว่า gyro นิ่งสมบูรณ์ **วางหุ่นนิ่งระหว่างนี้**

ถ้าไม่ได้ติดตั้ง IMU ให้แก้บรรทัดบนสุดของ .ino:

```cpp
constexpr bool USE_IMU = false;
```

จะข้ามหน้าสถานะและไม่มีเสียงเตือน รายละเอียดอยู่ใน [IMU_STARTUP.md](../../IMU_STARTUP.md)

### 4.2 อ่านมุมในโค้ด

`loop1()` อัปเดตตัวแปร global เหล่านี้ให้อัตโนมัติเมื่อ `isIMUCalibrated()` เป็น true อ่านจาก `setup()`/`loop()` ได้เลย ไม่ต้องเรียก `updateIMU()` เอง

| ตัวแปร | ความหมาย |
| --- | --- |
| `yaw_loop1` | Yaw 0–360° (วนรอบ) |
| `Con_yaw_loop1` | Yaw ต่อเนื่อง ไม่วนรอบ เช่น หมุนขวาสองรอบได้ ~720° **คำสั่งเลี้ยว/เดินตรงทั้งหมดใช้ตัวนี้** |
| `pitch_loop1`, `roll_loop1` | มุมก้ม/เอียง (roll กลับเครื่องหมายให้เอียงซ้ายเป็นบวก) |
| `offset_yaw_loop1` | ค่า offset Yaw ที่ตั้งตอนเปิดเครื่อง |

```cpp
Serial.println(Con_yaw_loop1);
if (Con_yaw_loop1 > 170 && Con_yaw_loop1 < 190) { /* หันประมาณ 180° */ }
```

### 4.3 รีเซ็ตมุม

ห้ามเรียกฟังก์ชันรีเซ็ต IMU จาก core 0 ตรง ๆ ให้ตั้งธงแล้ว `loop1()` จะรีเซ็ตให้บน core 1:

| ธง | ผลใน loop1() |
| --- | --- |
| `status_resetYaw = 1;` | `resetContinuousYaw()` ล้างรอบสะสม ให้ continuous yaw กลับไปเท่ากับ yaw 0–360 ปัจจุบัน |
| `status_resetYaw180 = 1;` | `resetGyro()` ตั้งทิศปัจจุบันเป็น `Con_yaw_loop1 = 180` |

ธงถูกล้างเป็น 0 หลังรีเซ็ต รอหลัง loop1 ทำงานอย่างน้อยหนึ่งรอบ (เช่น `delay(20)`) ก่อนอ่านมุมใหม่

### 4.4 เลี้ยวด้วย IMU

| คำสั่ง | หน้าที่ |
| --- | --- |
| `turnByAngle(angle, condition)` | หมุนจากมุมปัจจุบันเพิ่ม `angle` องศา (ใช้ค่าตั้งจาก `set_data_for_turnDirection`) |
| `turnByAngle_fix(angle, condition)` | เหมือนข้างบน แต่ snap มุมเป้าหมายไปทิศ 0/90/180/270 ที่ใกล้สุด |
| `turnPID(condition, targetYaw, speedMin, speedMax, kp, kd)` | หมุนไปมุม `Con_yaw_loop1` เป้าหมายโดยตรง |
| `set_data_for_turnDirection(errorDeg, speedMin, speedMax, kp, kd)` | ตั้งค่าที่ `turnByAngle*` ใช้ ค่าเริ่มต้น `1, 15, 60, 2, 0` |

`condition` คือรูปแบบการเลี้ยว:

| condition | การทำงาน |
| --- | --- |
| `0` | หมุนอยู่กับที่ (ล้อหนึ่งเดินหน้า อีกล้อถอย) |
| `1` | ล้อหนึ่งหยุด อีกล้อเดินหน้า (pivot ไปข้างหน้า) |
| `2` | ล้อหนึ่งหยุด อีกล้อถอย (pivot ไปข้างหลัง) |

```cpp
set_data_for_turnDirection(1, 15, 60, 2.0f, 0.0f);  // ค่าเริ่มต้นของ Lib ยังไม่ได้จูน
turnByAngle(90, 0);    // หมุนอยู่กับที่ +90°
turnByAngle(-90, 1);   // pivot -90°
```

- หยุดเมื่อคลาดเคลื่อนน้อยกว่า `errorDeg` หรือครบ **2.5 วินาที** (timeout พิมพ์ `Timeout! Exiting loop...`) แล้ว `ao()`
- มุมบวกคือทิศที่ Yaw เพิ่ม ต้องทดสอบว่าเป็นซ้ายหรือขวาบนหุ่นจริง
- `turnByAngle*` ตั้ง `status_resetYaw = 1` หลังหมุนเสร็จ
- คำสั่งเหล่านี้สั่ง `motor()` ตรง ไม่ใช้ PI ความเร็วล้อ และเป็น blocking

### 4.5 เดินตรงด้วย IMU

| คำสั่ง | หยุดเมื่อ |
| --- | --- |
| `moveStraightPID(dir, targetYaw, speed, durationMs, kp, ki, kd, slow)` | ครบเวลา `durationMs`; `slow = 1` ลดความเร็วใน 500 ms สุดท้าย |
| `moveStraightPID_Accel(dir, targetYaw, speed, durationMs, kp, ki, kd, accelMs, decelMs)` | ครบเวลา พร้อม Ramp เร่ง/ชะลอเป็นเส้นตรง |
| `moveStraightPID_Encoder(dir, targetYaw, speed, targetPulses, kp, ki, kd, slow)` | พัลส์ของ Encoder `selection_Encoder` (ค่าเริ่ม 1) เปลี่ยนครบ `targetPulses`; `slow = 1` ลดความเร็วตามพัลส์ที่เหลือ |
| `moveStraightSnapToNearest(dir, speed, targetPulses, kp, ki, kd, slow)` | เหมือน `_Encoder` แต่ใช้ทิศ 0/90/180/270 ที่ใกล้สุดเป็นเป้าหมาย |
| `moveStraightPID_step(dir, targetYaw, speed, kp, kd)` | ทำหนึ่งรอบต่อการเรียก ใช้ใน loop ที่ตรวจเงื่อนไขเอง |

`dir`: `0` = เดินหน้า ค่าอื่น = ถอยหลัง `speed` 10–100

```cpp
// ตัวอย่างรูปแบบการเรียก ค่า gain ยังไม่ได้จูน
float heading = Con_yaw_loop1;                          // ล็อกทิศปัจจุบัน
moveStraightPID(0, heading, 40, 1000, 1.5f, 0.0f, 0.5f, 1);           // เดินหน้า 1 วินาที
moveStraightPID_Encoder(0, heading, 40, 2000, 1.5f, 0.0f, 0.5f, 1);   // เดินหน้า 2000 พัลส์
```

- ระหว่างวิ่ง ความเร็วแต่ละล้อถูกจำกัดขั้นต่ำ 10 (`_Encoder` ขั้นต่ำ 0) แล้ว `ao()` เมื่อจบ
- `moveStraightPID_Encoder` ใช้พัลส์ดิบของล้อเดียวแบบค่าสัมบูรณ์ ไม่ใช่ระยะทางเป็นซม.
- ทุกคำสั่งในหัวข้อนี้มาจากซอร์สเดิม ใช้ `Con_yaw_loop1` และสั่ง `motor()` ตรง ดู [DRIVE_2WD.md](../../DRIVE_2WD.md)

---

## 5. Encoder

### 5.1 API ใน Lib

API อยู่ใน namespace `PuppyBotIMU` การอ่านและรีเซ็ตควรอยู่บน core เดียวกัน (core 0)

| API | หน้าที่ |
| --- | --- |
| `PuppyBotIMU::init_encoders()` | เริ่ม Encoder ทั้งสองตัว |
| `PuppyBotIMU::init_encoder(1)` / `(2)` | เริ่มเฉพาะตัวที่เลือก |
| `PuppyBotIMU::get_pulse_Encoder(1)` / `(2)` | อ่านพัลส์สะสม (มีเครื่องหมายตามทิศ) |
| `PuppyBotIMU::resetEncoder(1)` / `(2)` | รีเซ็ตเฉพาะตัวที่เลือก |

### 5.2 คำสั่งใน RobotSetup.h

| คำสั่ง | ตำแหน่งเรียก | หน้าที่ |
| --- | --- | --- |
| `RobotSetup::beginEncoders()` | `setup()` ก่อน `wait_SW1()` | เริ่ม Encoder ทั้งสองตัว |
| `RobotSetup::resetEncoders()` | หลัง `wait_SW1()` | รีเซ็ตทั้งคู่เมื่อเลือก Start |
| `RobotSetup::wheelSpeedSettings()` | — | คืนค่าที่วัดจริงของแต่ละล้อ (หัวข้อ 5.3) |
| `RobotSetup::beginWheelSpeedControl()` | หลัง `resetEncoders()` ก่อนวิ่ง | เปิด PI ความเร็วล้อ พิมพ์ผลทาง Serial คืน `true` เมื่อค่าครบ |

### 5.3 ค่าที่วัดจริง (2026-10-22)

วัดโดยยกล้อพ้นพื้น ที่ PWM +30% แบตเตอรี่ประมาณ 9.98–10 V คำสั่งมอเตอร์บวกทำให้ทั้งสองล้อเดินหน้า

| มอเตอร์ / Encoder | encoderSign | referencePwm | referenceTicksPerSecond |
| --- | ---: | ---: | ---: |
| M1 / Encoder 1 | -1 | 30 | 4667.0 |
| M2 / Encoder 2 | +1 | 30 | 4667.0 |

- `encoderSign` = เครื่องหมายพัลส์เมื่อสั่งมอเตอร์บวก
- `referenceTicksPerSecond` = อัตราพัลส์ ณ `referencePwm` ไม่ใช่พัลส์ต่อรอบล้อ ไม่ต้องใช้ CPR หรือเส้นผ่านศูนย์กลางล้อ
- ไม่มีการชดเชยแรงดันแบตเตอรี่ในโค้ด ไม่ได้ Save ลง Flash ค่าอยู่ใน `RobotSetup.h`

**วัดใหม่:** เปิด **File → Examples → Logistic15U-PuppyBot6 → WheelSpeedMeasure** ยกล้อพ้นพื้น กด SW1 เพื่อทดสอบ +30% ประมาณ 1.25 วินาทีต่อครั้ง (กด SW1 ระหว่างวิ่งเพื่อยกเลิก) อ่าน sign และ ticks/s บน TFT/Serial แล้วกรอกใน `RobotSetup.h` และอัปโหลดสเก็ตช์หลักกลับ

### 5.4 ใช้พัลส์แทนระยะทาง

Lib ไม่แปลงพัลส์เป็นซม. คำสั่งวิ่งตามพัลส์ (`lineFrontCrossPulses`, `Run_PID_Pulses`) ใช้ **ค่าเฉลี่ยพัลส์สัมบูรณ์ของสองล้อ** นับจากจุดเริ่มของช่วงนั้น จึงไม่ต้องรีเซ็ต Encoder ก่อนเรียก

หาพัลส์ต่อระยะเอง: เข้าเมนู **4. IMU และ Encoder** จดค่า ENC1/ENC2 เข็นหุ่นตรงตามระยะที่รู้ (เช่น 30 ซม.) แล้วคำนวณ `(|ΔENC1| + |ΔENC2|) / 2 ÷ ระยะ`

ตัวอย่างอ่านผ่าน Serial: [EncoderDual](../../examples/EncoderDual/EncoderDual.ino) (ส่ง `1` หรือ `2` เพื่อรีเซ็ตตัวที่เลือก)

---

## 6. ตามเส้นด้วย PID

### 6.1 สอบเทียบเซนเซอร์เส้น (Calibrate Line)

เลือก **6. Calibrate Line** ในเมนู ไม่ต้องแก้ .ino

1. เลือก **Front** หรือ **Back** แล้วเลือก **All 8 channels** (วัสดุทับครบแผง) หรือ **One by one** (เส้นแคบ)
2. **1/2 FLOOR**: วางบนพื้นสนาม กด Capture แล้วถือนิ่งจนครบ 16 ตัวอย่าง
3. **2/2 LINE**: วางบนเส้น แล้ว Capture เช่นกัน (One by one เก็บพื้นช่อง 0–7 ก่อน แล้วจึงเส้นช่อง 0–7)
4. ตรวจ **Min / Max / Gap**: `+` สีเขียว = ผ่าน, `!` = ต้องเก็บใหม่ แล้วเลือก **Save** รอ `Saved on board`
5. **Exit** กลับหน้าเลือก Front/Back; Exit อีกครั้งกลับเมนูหลัก

- ระหว่าง Capture จอแสดง `Reading... wait` ปุ่มไม่ยกเลิกหรือเริ่มซ้ำ ห้ามเลื่อนข้ามพื้นกับเส้นปนกัน
- Exit โดยไม่ Save ทิ้งชุดใหม่ ค่าที่บันทึกไว้ยังอยู่; Retry เริ่มเก็บแผงที่เลือกใหม่
- บันทึก Flash แยก Front/Back แยกจาก RGB; เปิดเครื่องใหม่ `wait_SW1()` โหลดให้เอง (ค่าเก่าแบบ 32 ตัวอย่างยังใช้ได้)
- Min/Max/Gap คือช่วงค่าที่เก็บได้ ไม่ได้สมมติว่าพื้นต้องต่ำกว่าเส้น
- ลำดับช่อง 0–7 ตามหน้าดู Mux ไม่ได้ยืนยันซ้าย–ขวาทางกายภาพ **ต้องตรวจทิศเลี้ยวจริงก่อนวิ่ง**

รายละเอียดเงื่อนไขตรวจค่า: [LINE_CALIBRATION.md](../../LINE_CALIBRATION.md)

### 6.2 คำสั่งตามเส้นหน้าใน RobotActions.h

ทั้งสองคำสั่งตรวจก่อนวิ่งว่า Save Calibrate Front แล้ว และแผงเป็น Mux 8 ช่อง ถ้าไม่ผ่านจะพิมพ์ `Front line: check calibration/settings` และไม่วิ่ง เลือกโหมด **ENCODER + LINE PD** อัตโนมัติเมื่อ `beginWheelSpeedControl()` สำเร็จ ไม่เช่นนั้นใช้ **LINE PD ONLY**

#### lineFrontCross — เจอแยกแล้วหยุด

```cpp
lineFrontCross(speed, kp, kd, crossThreshold, rampMs = 300, timeoutMs = 10000);
lineFrontCross(30, 0.09f, 0.05f, 400, 300, 10000);
```

#### lineFrontCrossPulses — เจอแยกแล้ววิ่งต่อตามพัลส์

```cpp
lineFrontCrossPulses(speed, kp, kd, crossThreshold, afterPulses,
                     rampMs = 300, rampDownPulses = 0, minSpeed = 10, timeoutMs = 10000);

lineFrontCrossPulses(30, 0.085f, 0.05f, 400, 540, 300, 200, 10, 10000);  // ผู้ใช้ทดสอบแล้ว
lineFrontCrossPulses(50, 0.1f, 0.00f, 400, 540, 300, 200, 10, 10000);    // ทดลองความเร็ว 50 ยังไม่ยืนยัน
```

| พารามิเตอร์ | ความหมาย |
| --- | --- |
| `speed` | ความเร็วฐาน 1–100 |
| `kp`, `kd` | gain PD ตามเส้น (≥ 0) |
| `crossThreshold` | เกณฑ์ผลรวมเซนเซอร์ 8 ช่อง (0–800) ที่ถือว่าเจอแยก; 400 = 50% เป็นค่าเริ่มทดลอง |
| `afterPulses` | พัลส์เฉลี่ยสองล้อที่วิ่งต่อหลังเจอแยก (> 0) |
| `rampMs` | เวลาเร่งความเร็วฐาน 0 → `speed`; `0` = ปิด |
| `rampDownPulses` | ช่วงท้ายที่ลดความเร็ว `speed` → `minSpeed`; `0` = ปิด; ต้องไม่เกิน `afterPulses` |
| `minSpeed` | ความเร็วตอนครบพัลส์ 1–`speed` |
| `timeoutMs` | เวลาจำกัดรวมทั้งเที่ยว (นับช่วง Ramp ด้วย) |

ลำดับ: ออกตัวพร้อม Ramp → ตามเส้นจนผลรวม ≥ เกณฑ์ → จดพัลส์ ณ จุดแยก **ไม่หยุด** → ตามเส้นต่อ → ลดความเร็วใน `rampDownPulses` สุดท้าย → ครบ `afterPulses` หยุด

**ค่าที่คืน** (ทั้งสองคำสั่ง): `true` = จบตามเงื่อนไข; `false` = ตั้งค่าไม่ถูก/หมดเวลา/Encoder fault ทุกกรณีมอเตอร์หยุด จอแสดงสถานะเช่น `FRONT CROSS: STOP`, `FRONT PULSES: STOP`, `FRONT TIMEOUT: STOP`, `ENCODER STOP`

```cpp
if (!lineFrontCrossPulses(30, 0.085f, 0.05f, 400, 540, 300, 200, 10, 10000)) {
  ao();
  return;  // ไม่ทำขั้นต่อไปถ้าไม่เจอแยก
}
```

- ถ้าเริ่มบนแยกที่ผ่านเกณฑ์อยู่แล้ว จะนับว่าเจอแยกตั้งแต่ต้น
- Ramp ปรับคำสั่งฐาน ไม่ได้บังคับ PWM หรือความเร็วจริงสองล้อให้เท่ากันทุกขณะ
- `ao()` หยุดสัญญาณขับ ตำแหน่งหยุดจริงขึ้นกับความเร็วและแรงเฉื่อย

### 6.3 คำสั่งตามเส้นใน Lib

| คำสั่ง | แผง | การทำงาน |
| --- | --- | --- |
| `Run_PID(speed, kp, kd)` | หน้า | PD หนึ่งรอบต่อการเรียก ต้องเรียกซ้ำใน loop |
| `Run_PID_B(speed, kp, kd)` | หลัง | เหมือน `Run_PID` |
| `Run_PID_Encoder(speed, kp, kd)` | หน้า | PD + PI ความเร็วล้อ หนึ่งรอบต่อการเรียก คืน `false` เมื่อ Encoder fault |
| `Run_PID_B_Encoder(speed, kp, kd)` | หลัง | เหมือนข้างบน |
| `Run_PID_Pulses(speed, kp, kd, targetPulses, timeoutMs = 10000, rampMs = 300)` | หน้า | ตามเส้นจนพัลส์เฉลี่ยครบแล้วหยุด เลือกโหมด Encoder เอง คืน `true` เมื่อครบ |
| `Run_PID_B_Pulses(...)` | หลัง | เหมือนข้างบน |
| `Run_PID_until_frontSensor(speed, kp, kd, sum)` | หน้า | `Run_PID` ซ้ำจนผลรวม ≥ `sum` (ไม่มี timeout ไม่หยุดมอเตอร์ให้) |
| `Run_PID_B_until_backSensor(speed, kp, kd, sum)` | หลัง | เหมือนข้างบน |
| `readline()` / `readline_B()` | หน้า/หลัง | ตำแหน่งเส้น 0–700 กลาง 350 |
| `Read_sumValue_sensor()` / `_B()` | หน้า/หลัง | ผลรวมค่า normalize 8 ช่อง 0–800 |

ตัวอย่างเขียน loop เอง (ตามเส้นหลังจนเจอแยก ไม่เกิน 5 วินาที):

```cpp
RobotSetup::beginWheelSpeedControl();
uint32_t t0 = millis();
while (Read_sumValue_sensor_B() < 400 && millis() - t0 < 5000) {
  if (!Run_PID_B_Encoder(30, 0.085f, 0.05f)) break;
}
stopWheelSpeedControl();
ao();
```

`Run_PID_Encoder`/`Run_PID_B_Encoder` ต้องปิดด้วย `stopWheelSpeedControl()` ส่วนคำสั่ง `_Pulses` และ `lineFrontCross*` จัดการให้เอง

### 6.4 ปรับตัวกรอง D และเพดานแรงเลี้ยว

ตั้งก่อนเริ่มช่วงวิ่ง ใช้ร่วมกันทั้ง `Run_PID*` และโหมด Encoder:

```cpp
LinePDOptions opt;
opt.derivativeFilterMs = 60;    // ตัวกรอง D (0–2000 ms) ค่าเริ่ม 60
opt.maxCorrection = 20;         // เพดานแรงเลี้ยว ±จุดเปอร์เซ็นต์ (0–100) ค่าเริ่ม 20
opt.allowWheelReverse = false;  // อนุญาตให้ล้อถอยขณะเลี้ยวแรง
setLinePDOptions(opt);          // คืน false ถ้าค่าผิดช่วง
```

ค่าเริ่มต้นเป็นจุดเริ่มทดลอง ไม่ใช่ผลจูนหุ่นจริง

### 6.5 PI ความเร็วล้อ (Encoder + Line PD)

เมื่อ `RobotSetup::beginWheelSpeedControl()` สำเร็จ PD ตามเส้นจะส่งเป้าความเร็วให้ PI แยกสองล้อ แทนการสั่ง PWM ตรง

- PI ภายในเริ่ม kp = 0.20 / ki = 0.50 ต่อวินาที เป็นค่าเริ่มของ Lib ยังไม่ยืนยันกับหุ่นจริง
- คาบควบคุมอย่างน้อย 20 ms **ต้องจูน Kp/Kd ตามเส้นใหม่** อย่าถือว่าค่าของ PD เดิมใช้ได้ทันที
- ค่ายังไม่ครบ → จอ `LINE PD ONLY / ENCODER NOT SET` และวิ่งแบบ PD เดิม
- เลือกโหมดก่อนเริ่มเที่ยวเท่านั้น ถ้า Encoder fault ระหว่างวิ่งจะหยุดทั้งสองมอเตอร์ ไม่สลับกลับไป PD
- Telemetry: `getWheelSpeedTelemetry()` ใช้ motor/Encoder บน core 0 เท่านั้น

รายละเอียดหน่วย เป้าหมาย integral และ fault: [WHEEL_SPEED_CONTROL.md](../../WHEEL_SPEED_CONTROL.md)

### 6.6 หน้าดู Position, PD และความเร็วล้อ

ปิดไว้ในสเก็ตช์ปัจจุบัน เปิดได้โดยเพิ่ม:

```cpp
// setup() ก่อนคำสั่งวิ่ง
beginLinePDMonitor(30, 0.085f, 0.05f);

// loop()
updateLinePDMonitor();
```

- ระหว่างวิ่งแสดง RUN / ENCODER + PD หลังหยุดกลับ PREVIEW
- ดูอย่างเดียว: คอมเมนต์คำสั่งวิ่งและ `ao()` ไว้ แล้วขยับหุ่นด้วยมือดู Position (0–700 กลาง 350), P/D, Turn หมุนล้อด้วยมือดู ACT ของ M1/M2 (ticks/s)
- อย่าเปิดพร้อมหน้า Show RGB

ความหมายแต่ละช่อง: [LINE_PD_MONITOR.md](../../LINE_PD_MONITOR.md)

### 6.7 แนวทางจูน

1. ตรวจทิศก่อน: วางหุ่นเยื้องขวาของเส้น ดูใน Monitor ว่า Turn พาหุ่นกลับเข้าเส้น ถ้ากลับทิศ ให้ตรวจลำดับช่องเซนเซอร์
2. เริ่มที่ความเร็วต่ำ (20–30) `kd = 0` เพิ่ม `kp` จนหุ่นตามเส้นได้แต่เริ่มส่าย
3. เพิ่ม `kd` ทีละน้อยจนส่ายลดลง
4. เพิ่มความเร็วทีละขั้น แล้วปรับ `kp`/`kd` ใหม่ทุกครั้ง
5. ปรับ `crossThreshold` จากค่า `Read_sumValue_sensor()` บนเส้นปกติเทียบกับบนแยกจริง ให้อยู่ระหว่างสองค่านี้

---

## 7. RGB

เซนเซอร์ TCS34725 สองตัว: RGB0 และ RGB1 บน I2C คนละบัส (RGB1 ใช้ Wire1 ร่วมกับ IMU บน core 1)

### 7.1 อ่านค่า

การเริ่มและอ่าน I2C ต้องอยู่ core เดียวกัน (ในสเก็ตช์นี้คือ `setup1()`/`loop1()`) การแสดงผลบน core 0 อ่านผ่าน cache ที่มี mutex

| API | หน้าที่ |
| --- | --- |
| `beginRGB()` / `beginRGB(RGB0)` / `beginRGB(RGB1)` | เริ่มทั้งสองตัวหรือตัวที่เลือก |
| `updateRGB()` | อ่านทั้งสองตัวแบบไม่รอ integration และอัปเดต cache |
| `getRGB(RGB0)` / `getRGB(RGB1)` | คืน `RGBReading` ล่าสุดจาก cache ไม่อ่าน I2C เพิ่ม |
| `beginRGBDisplay()` / `showRGB()` / `showRGB(RGB0)` | หน้าจอแสดงค่า อัปเดตเฉพาะค่าที่เปลี่ยน |

`RGBReading`: `rawRed`, `rawGreen`, `rawBlue`, `clear` เป็นค่าดิบ 16 บิต; `red`, `green`, `blue` เป็นค่า normalize ตัวอย่าง: [RGBMonitor](../../examples/RGBMonitor/RGBMonitor.ino)

### 7.2 สอบเทียบ RGB ผ่านเมนู

1. เลือก **5. Calibrate RGB** รอให้ LIVE มีค่าก่อนเริ่ม (RGB เริ่มหลัง IMU ตั้ง offset เสร็จ)
2. หน้าเลือกเซนเซอร์: BOOT วน **RGB0 → RGB1 → Save all → Exit** SW1 เลือก
3. หน้าสี: BOOT วน **Red → Green → Blue → Yellow → White → Black → Save all → Sensor menu → Exit**
4. วางสีจริงตรงชื่อบนจอ กด SW1 เก็บ 20 เฟรม (~1.1 วินาที) ถือนิ่งจนขึ้น `Captured in RAM` และเสียงสำเร็จ ทำครบ 6 สีต่อเซนเซอร์
5. **Save all** บันทึกทั้ง RGB0/RGB1 ลง Flash รอ `Saved to flash`
6. **Exit** ไม่ Save อัตโนมัติ ถ้ายังไม่บันทึกจะขึ้น `Exit: RAM not saved` ค่าใน RAM หายเมื่อรีเซ็ตหรือปิดเครื่อง

- จอ: LIVE = RGBC ดิบล่าสุด, MEAN = ค่าเฉลี่ยชุดที่เก็บแล้ว, `+` = สีที่มีข้อมูล, `6/6` = จำนวนสีที่ครบ, `RAM*` = มีค่ายังไม่บันทึก
- ข้อมูลที่บันทึกโหลดเมื่อเข้า Calibrate ครั้งแรกหลังเปิดเครื่อง การเข้าครั้งถัดไปไม่โหลดทับค่าใน RAM
- เก็บไม่ผ่านจะขึ้น `RGB zero: retry`, `Clear <10: retry` หรือ `Saturated: retry` ช่อง LIVE เป็น FAIL ค่าชุดเดิมยังอยู่

| เสียง | ความหมาย |
| --- | --- |
| 1400 Hz 35 ms | ถัดไป |
| 2600 Hz 80 ms | เลือก |
| 2000 Hz 70 ms → 3200 Hz 110 ms | เก็บหนึ่งสีสำเร็จ |

### 7.3 แยก 6 สีในโค้ดภารกิจ

| API | หน้าที่ |
| --- | --- |
| `loadRGBCalibration()` | โหลดค่าสอบเทียบจาก Flash (จำเป็นถ้าไม่ได้เข้าเมนู Calibrate ในรอบเปิดเครื่องนี้) |
| `getRGBColor(RGB0)` | คืน `RGBColor::Red/Green/Blue/Yellow/White/Black` หรือ `Unknown` |
| `classifyRGB(RGB0)` | คืน `RGBColorResult` (`color`, `status`, `candidates`, `matched()`) |
| `beginRGBColorDisplay()` / `showRGBColor()` | หน้าจอแสดงชื่อสี ไม่อ่าน I2C ไม่บันทึก Flash |

```cpp
// setup() หลัง wait_SW1()
loadRGBCalibration();

// ระหว่างภารกิจ
RGBColor c = getRGBColor(RGB0);
if (c == RGBColor::Red) {
  lift_up();
} else if (c == RGBColor::Unknown) {
  // ไม่แน่ใจ: อ่านซ้ำหรือข้าม
}
```

- เซนเซอร์ที่ใช้ต้องสอบเทียบครบ 6 สี อีกตัวไม่จำเป็น
- ใช้สัดส่วน RGB เทียบระยะใกล้สุดกับค่าเฉลี่ยแต่ละสี ค่าเริ่ม `maxDistance = 0.035`, `separationRatio = 0.70` ห่างเกินหรือกำกวมคืน `Unknown`
- เฟรมที่ไม่เชื่อมต่อ ไม่มีข้อมูล หรือเก่ากว่า 250 ms คืน `Unknown` ปรับผ่าน `RGBColorOptions`

ตัวเลือกและสถานะ: [RGBClassify README](../../examples/RGBClassify/README.md)

### 7.4 แสดงชื่อสีบนจอหลัง Start

เปิดสองบรรทัดที่คอมเมนต์ไว้ใน .ino:

```cpp
// setup() หลัง wait_SW1();
beginRGBMode(RGB_MODE_SHOW);

// loop()
updateRGBMode();
```

ถ้าเคยสอบเทียบในรอบเปิดเครื่องนี้จะใช้ค่าใน RAM ไม่เช่นนั้นโหลดจาก Flash ครั้งแรก คง `setup1()`/`loop1()` ตามเดิม

### 7.5 สเก็ตช์สอบเทียบแบบแยก (standalone)

[RGBCalibration](../../examples/RGBCalibration/RGBCalibration.ino) เป็นสเก็ตช์เต็มแยกจาก main มีคำสั่ง Serial (115200 baud) ด้วย สเก็ตช์หลักในโฟลเดอร์นี้ไม่มีตัวรับคำสั่ง Serial เหล่านี้

| API | หน้าที่ |
| --- | --- |
| `beginRGBCalibration()` | โหลดข้อมูลและเปิดหน้า TFT (เรียกหลัง `puppybot_setup()`) |
| `beginRGBCalibrationSW1()` / `updateRGBCalibrationSW1()` | เมนูปุ่ม |
| `updateRGBCalibration()` / `showRGBCalibration()` | สะสมเฟรม / วาดจอ |
| `startRGBCalibration(RGB0, RGBColor::Red, 20)` / `cancelRGBCalibration()` | เริ่ม / ยกเลิกการเก็บ |
| `getRGBCalibration(RGB0, RGBColor::Red)` | อ่านชุดอ้างอิง |
| `saveRGBCalibration()` / `loadRGBCalibration()` | บันทึก / โหลด Flash |
| `printRGBCalibration(Serial)` | ส่งออก CSV |

| คำสั่ง Serial | หน้าที่ |
| --- | --- |
| `0`, `1` | เลือก RGB0/RGB1 |
| `r`, `g`, `b`, `y`, `w`, `k` | เลือกสี |
| `c` / `x` | เริ่ม / ยกเลิกการเก็บ |
| `s` / `l` | บันทึก / โหลด Flash |
| `p` / `?` | ส่งออก CSV / แสดงคำสั่ง |

ที่เก็บ: EEPROM จำลองไบต์ 3072–3399 (Flash v2 มี version และ CRC32) รายการที่ยังไม่เก็บมี `samples == 0` ต้องไม่ใช้เป็นสีอ้างอิง เอกสารเพิ่มเติม: [คู่มือสอบเทียบ](../../examples/RGBCalibration/README.md), [API สอบเทียบ](../../examples/RGBCalibration/API.md)

---

## 8. Servo

`puppybot_setup()` เริ่ม PWM แบบไม่มีพัลส์จนกว่าจะสั่งมุม มี 6 ช่อง (0–5) มุม 0–180

| API | หน้าที่ |
| --- | --- |
| `servoRun(channel, angle)` | สั่งมุม คืน `false` ถ้าช่องหรือมุมผิดช่วง |
| `servoStop(channel)` | หยุดพัลส์เฉพาะช่อง (Servo ไม่ค้างแรงบิด) |
| `getServoLastAngle(channel)` | มุมที่สั่งล่าสุด ไม่ใช่มุมที่วัดจริง (-1 = ยังไม่สั่ง) |
| `servoSweep(channel, startAngle, endAngle, step, delayMs)` | เคลื่อนทีละ `step` องศา รอ `delayMs` ต่อก้าว (blocking) |
| `control_servo(channel, startAngle, targetAngle, delayMs)` | `servoSweep` แบบก้าวละ 1 องศา |
| `setServoPulseRange(channel, minUs, maxUs)` | ตั้งช่วงพัลส์จากค่าที่วัดจริง ต้อง `servoStop` ช่องนั้นก่อน ไม่บันทึก Flash |

### คำสั่งยกลิฟใน RobotActions.h

```cpp
inline void lift_up() {     // ช่อง 0 ไปมุม 10 รอ 300 ms แล้วหยุดพัลส์
  servoRun(0, 10);
  delay(300);
  servoStop(0);
}

inline void lift_down() {   // ช่อง 0 ไปมุม 90 และค้างพัลส์ไว้
  servoRun(0, 90);
}
```

ตัวอย่างเคลื่อนช้า ๆ:

```cpp
servoSweep(0, 90, 10, 2, 15);   // จาก 90 ไป 10 ก้าวละ 2° รอ 15 ms ต่อก้าว
control_servo(1, 0, 180, 10);   // ช่อง 1 จาก 0 ไป 180 ก้าวละ 1°
```

- ไม่ใช้ `servoRun(channel, -1)` เพื่อหยุด ให้ใช้ `servoStop()`
- `servoSweep` ต้องระบุมุมเริ่มเอง ควรใช้ค่าจาก `getServoLastAngle()` เพื่อไม่ให้ Servo กระตุก
- ช่วงพัลส์เริ่มต้น 300–3000 µs อ้างซอร์สเดิม ยังไม่ยืนยันกับกลไก มุม 10/90 ของลิฟยังไม่ได้ยืนยันว่าเป็นตำแหน่งบน/ล่างจริง

รายละเอียด: [SERVO_API.md](../../SERVO_API.md)

---

## 9. เพิ่มคำสั่งเองใน RobotActions.h

เพิ่มฟังก์ชันใต้คอมเมนต์ `// เพิ่มฟังก์ชันการทำงานอื่น ๆ ของหุ่นไว้ด้านล่างนี้` ด้วยรูปแบบ `inline` แล้วเรียกจาก `setup()` หลัง `puppybot_setup()`:

```cpp
// ตามเส้นไปแยกถัดไป เลี้ยวขวา 90° แล้วยกของ
inline bool go_and_pick() {
  if (!lineFrontCrossPulses(30, 0.085f, 0.05f, 400, 540, 300, 200, 10, 10000)) return false;
  turnByAngle(90, 0);
  lift_up();
  return true;
}
```

ค่าในตัวอย่างใช้แสดงรูปแบบเท่านั้น มุมเลี้ยวและทิศต้องทดสอบบนหุ่นจริง

---

## 10. ข้อมูลที่ต้องวัดจากหุ่นจริง

- ขาอุปกรณ์ใน Lib อ้างจากซอร์สบอร์ดเดิม ไม่ใช่ผลตรวจการต่อสายของหุ่นตัวนี้
- ยังต้องยืนยัน: พัลส์ต่อระยะทาง, ทิศเลี้ยว (ซ้าย/ขวา) ของ Line PD และ `turnByAngle`, มุม Servo, gain ของ Turn/Straight PID
- ค่า wheel speed เป็นผลทดสอบล้อยกไม่มีโหลด ต้องจูนบนสนามและขณะบรรทุกจริง
- การแยกสีผ่านการทดสอบซอฟต์แวร์ด้วยข้อมูลจำลอง ยังไม่ยืนยันความแม่นยำบนหุ่นจริง ต้องตรวจด้วยตัวอย่างที่ไม่ได้ใช้สอบเทียบ เปลี่ยนระยะ มุม แสง gain หรือ integration time ต้องสอบเทียบใหม่
- อ่าน BOOTSEL และ Save Flash ทำให้อีก core หยุดชั่วคราว จังหวะ IMU อาจถูกขัดช่วงสั้น ๆ

ผลติดตั้งและคอมไพล์จาก URL สาธารณะ: [PUBLIC_URL_VERIFICATION.md](https://github.com/Amornthep-ra/Logistic15U-PuppyBot6/blob/main/PUBLIC_URL_VERIFICATION.md)

---

## 11. ประวัติรุ่น

| รุ่น | การเปลี่ยนแปลงหลัก |
| --- | --- |
| 0.2.1 | สเก็ตช์นี้เข้า Lib เป็นตัวอย่าง `RobotStarter` เปิดได้จาก File → Examples |
| 0.2.0 | รุ่นเผยแพร่ที่รวมงาน 0.1.x-dev ทั้งหมด; Lib มี `Run_PID_Pulses()`/`Run_PID_B_Pulses()` และ README ของ Lib; สเก็ตช์เพิ่ม `lineFrontCrossPulses()` ใน `RobotActions.h` |
| 0.1.22-dev | ตัวกรอง D และเพดานแรงเลี้ยวร่วมกันใน `Run_PID*`, หน้า Line PD Monitor |
| 0.1.21-dev | PI ความเร็วล้อ (`Run_PID_Encoder`), ปุ่มตอบสนองเร็วขึ้นใน Calibrate Line, เก็บ 16 ตัวอย่างต่อพื้นผิว |
| 0.1.20-dev | เมนู `6. Calibrate Line` ใน `wait_SW1()` |
| 0.1.18-dev | หน้าสถานะ IMU ก่อน Servo และเมนู |
| 0.1.15-dev | ใช้ 2WD เท่านั้น ไม่ต้อง `setModeGyro()` ไม่มีคำสั่ง 4WD/Mecanum ([DRIVE_2WD.md](../../DRIVE_2WD.md)) |
| 0.1.14-dev | แก้ PWM และคำสั่ง Servo (พัลส์ของมุมเดิมเปลี่ยนหลังแก้ clock) |
| 0.1.13-dev | รวม Calibrate RGB ไว้ใน `wait_SW1()` ข้อมูล Flash v2 และการตั้งค่า 50.4 ms / Gain 4× ใช้ได้ต่อ |
| 0.1.9-dev | แก้ค่า RGB ค้าง R=2, G=4, B=6, C=0 จาก `word()` ชน macro ของ Arduino → เปลี่ยนเป็น `decodeRaw16()` |
| 0.1.8-dev | แยกข้อความเก็บสีไม่ผ่าน `RGB zero` / `Clear <10` / `Saturated` |
