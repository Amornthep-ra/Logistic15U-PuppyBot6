# Logistic15U-PuppyBot6

Arduino library และแพ็กเกจบอร์ดรุ่น **0.1.0** ใช้ชื่อบอร์ด **Logistic15U-PuppyBot6**
พร้อม `main/main.ino` สำหรับตรวจความเข้ากันได้ ยังไม่มีโปรแกรมภารกิจและยังไม่ได้อัปโหลดทดสอบหุ่น

บอร์ดใช้ Arduino-Pico 6.2.0 อ้าง RP2350B / ARM / 150 MHz / Flash 4 MB จาก
`reference/board/context.json` ของ KBIDE ไม่ใช่ผลตรวจ flash บนฮาร์ดแวร์จริง
FQBN: `logistic15u:rp2040:logistic15u`

**สถานะเผยแพร่:** เตรียม URL และไฟล์แล้ว แต่ยังไม่ได้อัปโหลด GitHub จึงยังใช้ URL ด้านล่างติดตั้งสาธารณะไม่ได้ ดู `PUBLISHING.md`

## ติดตั้งใน Arduino IDE 2.x

1. เปิด Preferences → Additional Boards Manager URLs แล้วเพิ่มทั้งสอง URL:

   ```text
   https://github.com/earlephilhower/arduino-pico/releases/download/global/package_rp2040_index.json
   https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/package_logistic15u_index.json
   ```

2. ใน Boards Manager ติดตั้ง **Raspberry Pi Pico/RP2040/RP2350 by Earle F. Philhower, III** รุ่น **6.2.0** ก่อน แล้วติดตั้ง **Logistic15U-PuppyBot6** รุ่น **0.1.0** แพ็กเกจนี้อ้าง core และเครื่องมือของ Arduino-Pico จึงไม่คัดลอกไฟล์ซ้ำ
3. ดาวน์โหลด [Library ZIP](https://raw.githubusercontent.com/Amornthep-ra/Logistic15U-PuppyBot6/main/dist/Logistic15U-PuppyBot6-0.1.0.zip) แล้วเลือก Sketch → Include Library → Add .ZIP Library หรือใช้ไฟล์ `dist/Logistic15U-PuppyBot6-0.1.0.zip` ใน repository
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
.\scripts\install.ps1 -ArduinoCli 'C:\Users\User\AppData\Local\Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
```

สคริปต์ติดตั้ง core, บอร์ด, dependency ตามรุ่น และ Library ZIP จาก repository แล้วคอมไพล์โดยไม่อัปโหลด
ระบุ `-ConfigFile` ได้เพื่อใช้ sketchbook แยก สคริปต์อาจเปลี่ยนรุ่น dependency ที่ติดตั้งไว้ก่อนหน้าให้ตรงตาราง

## ไฟล์สำคัญ

- `Logistic15U-PuppyBot6/`: ซอร์ส library, metadata และตัวอย่าง
- `hardware/logistic15u/rp2040/`: บอร์ดที่อ้าง Arduino-Pico
- `dist/`: ZIP library, ZIP บอร์ด และ manifest พร้อม SHA-256
- `package_logistic15u_index.json`: Boards Manager index
- `scripts/build_release.py`: สร้าง ZIP และ index ซ้ำด้วย Python standard library
- `VERIFICATION.md`: ผลการติดตั้งและคอมไพล์
- `reference/original-main/main/main.ino`: ตัวอย่างเดิมที่ไม่แก้ไข
- `COPY_MANIFEST.json`: หลักฐานขั้นคัดลอกเดิม ไม่ใช่ manifest ปัจจุบัน

ไม่ได้แจกจ่าย InvenSense SDK `Invn` เดิม ใช้ SparkFun จาก Library Manager แทน
dependency มาตรฐานและ build cache ไม่อยู่ใน ZIP ดูที่มาใน `NOTICE.md` ของ library

หากเคยคอมไพล์แล้วเลือก Adafruit ST7735 1.0.8 จาก cache จนพบ `setSPISpeed` ไม่มีใน class ให้คอมไพล์ด้วย CLI `--clean` ตาม VERIFICATION.md; ไม่ต้องลบไลบรารีเดิมของโครงการอื่น
