# ผลตรวจ Line PD Monitor รุ่น 0.1.22-dev

ตรวจในเครื่องวันที่ 2026-10-22 ยังไม่อัปโหลดเฟิร์มแวร์หรือเผยแพร่ GitHub ไฟล์ dist และ Boards Manager index ไม่เปลี่ยน

## การเปลี่ยนแปลง

- เพิ่ม `PuppyBotLinePD.h` สำหรับกรอง D ตามเวลาจริงและจำกัดแรงเลี้ยว ค่าเริ่ม 60 ms, 20 จุดเปอร์เซ็นต์ และไม่ให้ PD สั่งล้อกลับทิศ ใช้ร่วมกับ Legacy PD และ Encoder PD ทั้ง Front/Back
- เพิ่ม `PuppyBotLinePDMonitor.h` สำหรับหน้าดู Position, P/D, correction และความเร็วล้อเป้า–จริง หน่วย ticks/s เปิดด้วย `beginLinePDMonitor()` และบริการด้วย `updateLinePDMonitor()`
- แยกตัวคำนวณ PREVIEW ออกจากตัวควบคุมจริง ไม่สั่งมอเตอร์หรือรีเซ็ต Encoder จากหน้าดูค่า เมื่อขับใช้ telemetry จริง พร้อมจำกัดการวาดแต่ละครั้งเป็นอักขระ 6×8 หนึ่งตัว
- สเก็ตช์ผู้ใช้เปิด PREVIEW หลัง Start ปิดคอมเมนต์คำสั่ง `follow_front_until_cross_stop(30, 0.09f, 0.05f, 400, 10000);` และ `ao();` คง `lift_down()` และ IMU เดิม
- อัปเดต README ของ repository และของสเก็ตช์ พร้อม `LINE_PD_MONITOR.md` ใน Lib ที่ติดตั้ง

## การทดสอบซอฟต์แวร์

รันจากโฟลเดอร์ Code-Logistic15y:

```powershell
python -X utf8 tests/line_pd/run.py
python -X utf8 tests/wheel_speed/run.py
python -X utf8 tests/line_calibration/run.py
python -X utf8 tests/drive_2wd/run.py
```

ทุกชุดผ่าน ผลตรวจครอบคลุม:

- D แรกหลัง reset เป็นศูนย์, กรองตาม dt จริง, millis wrap, เปลี่ยน Front/Back และปฏิเสธ NaN หรือ options ผิดช่วง
- จำลองตำแหน่ง 350 → 370 → 350 ที่ 20 ms และ Kp=0.02, Kd=2 ให้ D หลังกรองเป็น 0 → 10 → -2.5 แทน D ก่อนกรอง 0 → 40 → -40 เป็น fixture เพื่อพิสูจน์สูตร ไม่ใช่ผลวิ่งหุ่น
- คำสั่งก่อน PI ไม่เกิน ±100 และไม่กลับทิศล้อเมื่อปิด allowWheelReverse แม้ error/gain สูง รวมกรณี speed ติดลบของ Legacy และ speed ศูนย์
- PREVIEW อ่านเส้นที่ Save แยก Front/Back, ความเร็ว Encoder แบบมีเครื่องหมาย, เป้าหมายศูนย์ และไม่มีการสั่งมอเตอร์ รีเซ็ต counter หรือเปลี่ยนประวัติ PD จริง
- ขณะวิ่ง หน้าดูค่าตรงกับ telemetry ของ PD และ PI ไม่อ่านเซนเซอร์เส้นซ้ำ เมื่อหยุดแสดงเป้าหมายศูนย์ Legacy ไม่มีการสร้างค่า ticks/s เป้าหมายขึ้นเอง
- ใช้ body ของ Run_PID และคำสั่ง until จาก puppybot.h จริงทดสอบ Front/Back และเพดานแรงเลี้ยว
- TFT ไม่เขียนเมื่อค่าเดิม ลบตัวเลขเก่าและ LIMIT ได้ครบเมื่อข้อความสั้นลง วาดไม่ล้น 160×128 หรือ 128×160 และการวาดทยอยให้ผลพิกเซลตรงกับการวาดครบหน้า
- จำลองเวลา SPI 12 ms ต่ออักขระร่วมกับ adapter และ Encoder จำลอง ตัวควบคุมทำงานต่อเนื่อง 2 วินาทีโดยไม่เกิด watchdog ทั้งนี้ยังไม่ใช่การวัดเวลา SPI จริง
- ชุดเดิม PI, Front-until-cross, timeout/fault, Calibrate Line/Flash/BOOT/SW1 และการขับ 2WD/IMU helpers ผ่าน

log และภาพ TFT จาก raster fixture อยู่ใน `.arduino-test/line-pd-tests/` ภาพผ่านการเปิดดูประกอบกับการตรวจ bounds และพิกเซล ไม่ใช่ภาพถ่ายจอหุ่น

## คอมไพล์ Arduino

คอมไพล์สเก็ตช์ `C:\Users\User\Desktop\PrinceBot Work\Example Code\Logistic15U-PuppyBot6` ผ่าน ทั้งการระบุซอร์ส Lib และการเลือก Lib ที่ติดตั้งจริง:

| รายการ | ผล |
|---|---|
| FQBN | `logistic15u:rp2040:logistic15u` |
| Lib ที่ Arduino เลือก | `C:\Users\User\Documents\Arduino\libraries\Logistic15U-PuppyBot6`, 0.1.22-dev |
| Flash | 132988 / 4186112 bytes |
| Global RAM | 16996 / 524288 bytes |
| ผลคอมไพล์ | exit 0 |

log เต็มอยู่ที่ `.arduino-test/line-pd-tests/arduino-installed-compile.log` ตรวจว่าซอร์ส header ทั้ง 31 ไฟล์ใน repository ตรงกับ Lib ที่ติดตั้งแล้ว

เทียบ SHA-256 กับสำเนาก่อนงานนี้แล้ว ไฟล์ IMU, TurnPID, Encoders, IMUStartup, IMUStartupDisplay และ Servos ไม่เปลี่ยน `RobotSetup.h` และเนื้อหา setup1()/loop1() ของสเก็ตช์เหมือนเดิม ค่าผู้ใช้ยืนยัน M1/Encoder1 sign=-1 และ M2/Encoder2 sign=+1 ที่ 4667.0 ticks/s ณ PWM30 คงเดิม

## สิ่งที่ยังต้องตรวจบนหุ่น

การคอมไพล์และ fixture ยังไม่ยืนยันความนิ่งบนสนาม ต้องตรวจทิศ Position/การเลี้ยว, รอบควบคุมจริงเมื่อวาด TFT, อัตราพัลส์ขณะรับโหลด และจูน Kp/Kd/filter/correction จากการวิ่งจริง ค่าเริ่มทดลองไม่แทนการสอบเทียบกลไกหรือ Encoder ไม่มีการเพิ่ม CPR/PPR ขาอุปกรณ์ หรือมุม Servo ที่เดาขึ้น
