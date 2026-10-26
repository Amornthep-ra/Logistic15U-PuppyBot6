# แยกสีแบบ High-PuppyBot6 — 0.1.8-dev

RGB0 และ RGB1 ใช้สูตรเดียวกับ `HighRGBCheck.h`: หาร R/G/B ดิบด้วยผลรวม R+G+B แล้วเทียบระยะกับสีอ้างอิงทั้ง 6 สี เลือกสีที่ใกล้ที่สุดเมื่อผ่านทั้งเกณฑ์ระยะและเกณฑ์แยกจากอันดับสอง ใช้ค่าเฉลี่ยจากการ Calibrate ของเซนเซอร์นั้นเป็นสีอ้างอิง ไม่มีค่าสีสำเร็จรูปใส่ให้

Clear ใช้ตรวจสัญญาณเท่านั้น ไม่เข้าคะแนนเลือกชื่อสี เหมือน High-PuppyBot6 ค่า Min–Max ยังเก็บและส่งออกได้ แต่ไม่ใช้ตัดสินสีแล้ว ขาวและดำที่มีสัดส่วน RGB ใกล้กันอาจกำกวม ต้องตรวจบนพัสดุจริง

## อัปเกรดและสอบเทียบใหม่

รุ่นนี้ใช้ 50.4 ms (ชื่อค่าคงที่ 50MS) / Gain 4× ทั้งสองช่อง ข้อมูล Flash รูปแบบ v1 ที่เก็บด้วย 2.4 ms จะโหลดไม่ผ่าน และไม่ถูกลบทิ้งอัตโนมัติ ต้องเปิดตัวอย่าง RGBCalibration สอบเทียบ 6 สีแยก RGB0/RGB1 และเลือก Save all ก่อนกลับมาใช้สเก็ตช์นี้

รูปแบบ v2 เก็บ ATIME, gain และเกณฑ์ Clear ร่วมกับข้อมูลและ CRC32 ที่ EEPROM 3072–3399 รวม 328 ไบต์ ไม่มีการแปลงค่าดิบเก่าโดยประมาณ

## คำสั่งหลัก

```cpp
// setup() หลัง puppybot_setup()
loadRGBCalibration();
beginRGBColorDisplay();

// loop() บน core เจ้าของ TFT/Calibration
showRGBColor();

// setup1() หลังบอร์ดและ IMU ตั้งค่าเสร็จ
beginRGB();

// loop1() หลังอัปเดต IMU ทุกครั้ง
updateRGB();
```

อย่าคัดลอกตัวอย่าง RGBClassify ทับ main ที่มี IMU: ตัวอย่างนี้ทดสอบสีอย่างเดียว ให้คง setup1/loop1 ของ IMU ใน main แล้วเรียก API ข้างต้น

| API | ผล |
| --- | --- |
| `getRGBColor(RGB0)` / `getRGBColor(RGB1)` | คืน Red, Green, Blue, Yellow, White, Black หรือ Unknown |
| `classifyRGB(channel)` | คืน `RGBColorResult`: color, status, candidates และ matched() |
| `classifyRGB(channel, reading, options)` | ใช้เฟรมที่ส่งให้กับข้อมูลสอบเทียบปัจจุบัน |
| `classifyRGB(references, channel, reading, options)` | ใช้ข้อมูลสอบเทียบที่ระบุ |
| `rgbColorName(color)` / `rgbColorStatusName(status)` | ชื่อภาษาอังกฤษ |
| `showRGBColor(options0, options1, refreshMs)` | ตั้งตัวเลือกแยกสองช่องและรอบ TFT; ค่าเริ่มต้น 100 ms |

ตัวแยกสีและหน้าจออ่านเฉพาะ cache ไม่ทำ I2C หรือเขียน Flash ใช้บน core เดียวกับเจ้าของข้อมูล Calibration หน้าจอวาดเฉพาะชื่อสี/สถานะที่เปลี่ยน

## ตัวเลือกและสถานะ

| RGBColorOptions | ค่าเริ่มต้น | ความหมาย |
| --- | --- | --- |
| `maxDistance` | 0.035 | ระยะ Euclidean สูงสุดจากสัดส่วน RGB อ้างอิง; รับ 0.001–1.0 |
| `separationRatio` | 0.70 | ระยะอันดับหนึ่งต้องน้อยกว่าอัตราส่วนนี้คูณระยะอันดับสอง; รับ 0.01–0.99 |
| `maxAgeMs` | 250 | ปฏิเสธเฟรมที่มีอายุถึงค่านี้; 0 ปิดการตรวจอายุ |

`padding` ถูกถอดออกเพราะไม่ใช้กล่อง Min–Max แล้ว `candidates` คือจำนวนสีที่อยู่ในระยะ maxDistance และไม่ใช่เปอร์เซ็นต์ความมั่นใจ การตัดสินยังต้องตรวจอันดับสอง แม้ candidates จะมีเพียงหนึ่งก็ตาม

สถานะ Matched หมายถึงผ่านเกณฑ์ ส่วน Unknown คือไม่มีสีใกล้พอ, Ambiguous คืออันดับหนึ่งไม่ชัดกว่าอันดับสอง, LowSignal คือ RGB รวมเป็นศูนย์หรือ Clear < 10, Saturated คือช่องใดช่องหนึ่ง >= 21504 สถานะอื่นคือ NoSample, Offline, Stale, IncompleteCalibration, InvalidChannel และ InvalidOptions ทุกสถานะที่ไม่ใช่ Matched คืนสี Unknown

## การอ่านร่วมกับ IMU

`beginRGB()` เริ่มกระบวนการเปิดเซนเซอร์โดยไม่รอผลวัด ต้องเรียก `updateRGB()` ต่อเนื่องใน loop1 ฟังก์ชันรอ oscillator 3 ms ด้วยการเช็กเวลา แล้วปล่อยเซนเซอร์เก็บแสงเอง อ่านสถานะและอ่าน Clear, R, G, B แยกทีละ 16 บิตด้วยคำสั่งเดียวกับ High ทุกอย่างน้อย 55 ms ทั้งสองตัวเก็บแสงพร้อมกัน

ไม่มีการเรียก getRawData()/delay() ในเส้นทางอ่านและ reconnect ของ API ใหม่นี้ การรับส่ง I2C ยังใช้เวลาบนบัสตามปกติ ไม่เปลี่ยน clock หรือรีเซ็ต IMU และต้องให้ RGB กับ IMU ที่ใช้ Wire1 มีเจ้าของ core เดียวกัน คำสั่ง RGB รุ่นเก่าไม่ได้รับการเปลี่ยนให้เป็นแบบนี้

ผลทดสอบซอฟต์แวร์เทียบกับ Check_Color ของ High โดยตรง ยังต้องทดสอบเวลา IMU และความแม่นยำบนหุ่นจริง ไม่มีการยืนยันสีหลายเฟรมอัตโนมัติ
