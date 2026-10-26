# การขับเคลื่อน 2WD — 0.1.15-dev

หุ่นนี้ใช้มอเตอร์ขับเคลื่อนสองตัวตามข้อมูลผู้ใช้ Library จึงใช้ 2WD ตลอด ไม่มีโหมด 4WD และไม่ต้องเรียก `setModeGyro()` ก่อนวิ่ง

คำสั่งเคลื่อนที่ใช้ **M1 และ M2** ตามซอร์สเดิม การจับคู่สายซ้าย/ขวาและทิศเดินหน้าของหุ่นจริงยังต้องทดสอบ ไม่เปลี่ยนขา สลับทิศ หรือใส่ค่าจูน PID/Encoder ใหม่

## คำสั่งที่ใช้ได้

- `fd`, `bk`, `fd2`, `bk2`, `tl`, `tr`, `sl`, `sr` ใช้คู่ขับ M1/M2
- `turnPID`, `turnByAngle`, `turnByAngle_fix` ใช้คู่ขับนี้เสมอ ค่า condition ยังคงหมายถึงรูปแบบหมุน/เลี้ยว ไม่ใช่จำนวนล้อ
- `moveStraightPID`, `moveStraightPID_Accel`, `moveStraightPID_step` และ `moveStraightPID_Encoder` ใช้สองมอเตอร์ รวมถึงคำสั่ง Direction/Snap ที่เรียกต่อกัน
- วิ่งตามเส้นหน้าใช้ `Run_PID`; เส้นหลังใช้ `Run_PID_B` คำสั่ง until_frontSensor/until_backSensor ที่ไม่มี 4DW ยังคงอยู่
- `ao()`, `aoS()`, `motorBreak()` และการหยุดเป็นกลุ่มของ `motorStop(0)`/`motorBreak(0)` ทำงานเฉพาะคู่ขับ ไม่มีการหยุดอุปกรณ์เสริมที่ช่องอื่น
- `motor_control(state, speed)` รองรับ state 0 เดินหน้า, 1 ถอยหลัง, 2/3 เลี้ยวแบบหยุดข้างหนึ่ง, 4/5 หมุนอยู่กับที่ ค่าอื่น รวมถึง 6–9 เดิม จะหยุดคู่ขับ ไม่แปลงคำสั่งเลื่อนข้างเป็นการเลี้ยวเงียบ ๆ

`motor(7, speed)` เดิมเป็นคำสั่งรวมทุกช่อง ตอนนี้สั่งเพียง M1/M2 และส่งค่าความเร็วเปอร์เซ็นต์ไปแปลงเพียงครั้งเดียว ส่วน `motor(channel, speed)` ที่ระบุช่อง 3–6 โดยตรงยังเป็นคำสั่งเอาต์พุตบอร์ดสำหรับอุปกรณ์เสริม ไม่ได้ถูกเรียกจากคำสั่งเคลื่อนที่หรือ PID การหยุด/เบรกที่ระบุช่องอุปกรณ์เสริมโดยตรงยังทำงานตามเดิม

## ย้ายโค้ดจากรุ่นก่อน

| เดิม | รุ่นนี้ |
| --- | --- |
| `PuppyBotIMU::setModeGyro(...)` / `Set_Mode_Gyro` | ลบออก ไม่ต้องเลือกโหมด |
| `Run_PID4DW(...)` | `Run_PID(...)` |
| `Run_PID_B4DW(...)` | `Run_PID_B(...)` |
| `Run_PID4DW_until_frontSensor(...)` | `Run_PID_until_frontSensor(...)` |
| `Run_PID_B4DW_until_backSensor(...)` | `Run_PID_B_until_backSensor(...)` |
| `mecanumSlide...` ทุกคำสั่ง | ลบออก ฐานสองล้อนี้เลื่อนข้างแบบ Mecanum ไม่ได้ |
| helper `follow_line_*_safe(driveMode, ...)` | เอาอาร์กิวเมนต์ driveMode ตัวแรกออก |

รูปแบบ helper ใหม่:

```cpp
follow_line_by_encoder_safe(signedSpeed, kp, kd, targetPulses, timeoutMs, readEnc);
follow_line_by_encoder_store_safe(signedSpeed, kp, kd, targetPulses, timeoutMs,
                                  resetEnc, readEnc, outPulses);
follow_line_cross_safe(signedSpeed, kp, kd, sumThreshold, crosses, extraVal,
                       extraMode, stopDelay, timeoutMs, resetEnc, readEnc);
```

สเก็ตช์ปัจจุบันไม่มีการเรียกโหมดเดิม จึงไม่ต้องเพิ่มการตั้งค่าใหม่และไม่ได้เพิ่มคำสั่งให้หุ่นเคลื่อนเอง การเริ่ม IMU ใน setup1(), การอ่าน RGB, Servo และ Encoder สองตัวยังคงเดิม การแก้ครั้งนี้เปลี่ยนการเลือกมอเตอร์ ไม่ใช่การจูนหรือตรวจรับความแม่นยำ PID ทั้งชุด คำสั่ง Encoder บางตัวเดิมยังไม่มี timeout และต้องตรวจสัญญาณก่อนใช้งานจริง

ตัวอย่างทดสอบบนคอมพิวเตอร์ครอบคลุมเอาต์พุตของคำสั่งขับเคลื่อนและตรวจว่ามีเพียง GPIO ของ M1/M2 ยังไม่มีการอัปโหลดหรือวิ่งบนหุ่นจริง
