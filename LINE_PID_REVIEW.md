# Review PID ตามเส้นหน้า–หลัง — 2026-10-22

ตรวจรุ่นในเครื่อง 0.1.18-dev: `puppybot.h` และ `KB_PIDFollowHelpers.h` เทียบกับ Library ที่ติดตั้งแล้วตรงกัน เป็นการรีวิวตามเส้นโดยเฉพาะ ไม่ใช่ heading/IMU ใน PID_REVIEW.md ก่อนหน้า ไม่แก้ Library, สเก็ตช์, ค่าจูน หรืออัป GitHub

## ข้อพบ

1. **P1 — Calibrate เขียนทับ extrema ที่ถูกต้องเมื่อเจอ 0/1023** (`puppybot.h:815-824`, `1012-1021`). เงื่อนไข `PID_Max >= 1023` บังคับแทนค่ามากสุดด้วยค่าปัจจุบัน แม้ค่าปัจจุบันต่ำกว่า ส่วน `PID_Min == 0` ก็แทนค่าน้อยสุดด้วยค่าปัจจุบันแม้สูงกว่า จำลองค่าตามลำดับ 1023 → 100 ได้ Min=100/Max=100 ทั้งหน้าและหลัง แทน 100/1023; ลำดับ 0 → 100 ก็เสียค่า Min=0 เช่นกัน ควรเก็บ extrema แบบ monotonic ภายใน session และมีคำสั่งเริ่ม session ใหม่ชัดเจน
2. **P1 — ไม่มีการป้องกันการหารศูนย์ตอน normalize/หาตำแหน่ง** (`puppybot.h:907-910,931`, `1039-1042,1062`). ถ้า Min=Max (เช่นข้อ 1 หรือสอบเทียบบนพื้นสีเดียว) map() มีตัวหารเป็นศูนย์ แต่หน้าสอบเทียบยังแจ้ง completed อีกกรณีที่เป็นอิสระจาก Min/Max คือ Sensitive=100 และทุกค่า normalized เป็น 1..5: ตั้ง onLine=true แต่ไม่รวมค่าพวกนี้ใน sum ทำให้ avg/sum หารศูนย์ ควรตรวจช่วงสอบเทียบและ sum ก่อนใช้ พร้อมสถานะ invalid ที่ caller จัดการได้
3. **P1 — คำสั่ง until_sensor ไม่มีทางออกเมื่อไม่ถึง threshold** (`puppybot.h:1129-1164`). ลูป do/while ไม่มี timeout/cancel/lost-line limit ถ้าเซนเซอร์ไม่เจอเส้น/แยกหรือ threshold สูงเกิน จะส่งมอเตอร์ต่อและไม่คืนการควบคุม ส่วน readline/readline_B แทน lost line ด้วยค่าขอบตามตำแหน่งเก่า ซึ่งเป็นวิธีค้นเส้นได้ แต่ไม่มีธงแยกว่าเห็นขอบจริงหรือไม่เห็นเลย ควรแยกสถานะ lost และจำกัดเวลาค้นเส้น/เวลารวม สำหรับ until_sensor ที่คืนแล้ว เอาต์พุตยังค้างอยู่ ไม่มี ao() อัตโนมัติ ต้องระบุสัญญาการหยุดหรือวิ่งต่อให้ชัด ไม่ถือว่าการไม่ ao() เป็นบั๊กโดยลำพัง
4. **P2 — D เก็บประวัติจากงานก่อน ไม่มี reset ตอนเริ่มงานใหม่** (`puppybot.h:940,948`, `1070,1079`). previous_error/previous_error_B อยู่ข้ามคำสั่ง; ao() หยุดเอาต์พุตเท่านั้น เมื่อหยุดแล้วย้ายหุ่นมาเริ่มเส้นใหม่ ค่า D ครั้งแรกอาจรุนแรง แม้ขณะนี้ error=0 ตัวอย่างจำลอง Kp=0/Kd=1 หลังเคยอยู่ขอบขวา แล้วเริ่มใหม่กลางเส้น ได้ D=-350 และเอาต์พุตหลัง clamp = -100/+100 ตัวเลขนี้เป็นหลักฐานเชิงซอฟต์แวร์ ไม่ใช่ค่าจูนแนะนำ ควรมี begin/reset PD ที่ใช้ error ปัจจุบันตั้งประวัติเริ่มต้น รวมถึงหลังเปลี่ยนจำนวน/ลำดับเซนเซอร์
5. **P2 — นับแยกเดิมซ้ำได้** (`KB_PIDFollowHelpers.h:91-93`). มีเพียงตรวจ threshold แล้ว count++ และ delay(50) ไม่มีรอพ้นแยกก่อนนับใหม่ จำลองแยกเดียวที่ยังถูกเซนเซอร์ทับตลอด สามารถผ่าน crosses=3 ภายในสามรอบ ต้องนับขอบการเข้าแยก และ re-arm หลังพ้น พร้อม hysteresis/debounce
6. **P2 — ผลรวมตรวจแยกไม่ clamp เหมือนตำแหน่งเส้น** (`puppybot.h:1093-1118` เทียบ `913/1044`). เมื่อ ADC ออกนอกช่วงที่สอบเทียบ map อาจติดลบหรือเกิน 100 ใน sum แม้ readline clamp แต่ละช่องไว้ 0..100 ผลรวม threshold จึงตีความไม่ตรงกัน เช่น Min=200/Max=800, ช่อง 3 อ่าน 0 อีกเจ็ดช่องอ่าน 900: ตำแหน่งอ่านช่อง 3 ถูก แต่ sum เหลือ 21 แทน 100 ตามค่า clamp ควรใช้ normalized frame ชุดเดียวกันทั้งตำแหน่งและตรวจแยก
7. **P2 — centroid มี bias จากการบวก 50 ทุกช่อง** (`puppybot.h:918`, `1049`). `avg += value*(i*100)+50` ทำให้ตำแหน่งขยับตามความเข้มรวม แม้รูปแบบเซนเซอร์สมมาตร จำลอง 8 ช่องค่า normalized เท่ากัน 25 ได้ตำแหน่ง 352 แทนกึ่งกลาง 350 ทั้งหน้าและหลัง ควรถ่วงน้ำหนักด้วยตำแหน่งจริงของช่อง แล้วจัดการ rounding ที่ผลหาร หากตั้งใจชดเชยตำแหน่ง ให้เป็น offset ที่แยกและคงที่

## สิ่งที่เป็นพฤติกรรม/ข้อจำกัด ไม่ใช่บั๊กที่ยืนยัน

- Run_PID และ Run_PID_B ใช้ P + D เท่านั้น ไม่มี Ki แม้มีตัวแปร integral สะสมอยู่ จึงเป็น PD ตามเส้น ไม่จำเป็นต้องเพิ่ม I เพียงเพราะชื่อฟังก์ชันว่า PID
- D ใช้ error ต่างกันต่อการเรียก ไม่ได้หาร dt ซึ่งทำได้สำหรับ discrete PD เมื่อรอบเรียกคงที่ แต่ delay(1) ไม่ใช่เวลารอบทั้งหมด: อ่าน MUX มี delayMicroseconds(200) ต่อช่อง และ caller อาจทำงานอื่น ต้องกำหนดคาบหรือระบุวิธีจูนให้ตรง อย่าเปลี่ยนเป็น D ต่อวินาทีแล้วใช้ Kd เดิมโดยไม่แปลง/จูนใหม่
- Run_PID_B(speed บวก, ...) สั่งถอยและอ่าน ADC_B ส่วน helper follow_line_*_safe(speed ติดลบ, ...) ยังเรียก Run_PID และอ่านด้านหน้า ไม่เลือกด้านหลังอัตโนมัติ ถ้าต้องการเลือกแผงหลังควรมีพารามิเตอร์/API ชัดเจน การถอยโดยใช้แผงหน้าอาจเป็นเจตนาของงานบางแบบ จึงไม่สรุปว่าผิดเสมอ
- เครื่องหมายซ้าย/ขวาขึ้นกับลำดับแผงหน้า–หลังและขั้วมอเตอร์จริง ยังไม่ยืนยัน จึงไม่เสนอให้กลับเครื่องหมายแบบเดา
- ค่าเริ่มต้น PID_NumPin/PID_NumPin_B=3 และทุก PID_SetupPin เป็น 0; สเก็ตช์ปัจจุบันยังไม่มีการตั้งโหมด MUX/จำนวนช่อง/ลำดับช่องหรือสอบเทียบเส้น การเปิดหน้าดู MUX ในเมนูไม่ตั้ง PID ให้อัตโนมัติ ต้องตั้งตามการต่อจริงก่อนใช้งาน
- Arduino-Pico core 6.2.0 ที่ติดตั้งมี _readBits=10 โดยปริยาย จึงไม่ได้สรุปว่าเพดาน 1023 ผิดเพราะเป็น ADC 12-bit; ถ้าผู้ใช้เปลี่ยน analogReadResolution จึงต้องทบทวนช่วงนั้น
- การผสมมอเตอร์อาจทำให้ข้างหนึ่งกลับทิศเมื่อ correction สูง และ motor() clamp ที่ -100..100 นี่เป็นนโยบายที่ต้องเลือกให้ตรงการค้นเส้น/เลี้ยว ไม่ใช่หลักฐานว่าเกินช่วง PWM จริง

## หลักฐานจำลอง

ดึง body ของฟังก์ชันจริงลง `.arduino-test/line-pid-review/ActualLine.h` ใช้ `review.cpp` จำลอง ADC, เวลา, TFT และมอเตอร์ พร้อม helper จริง ไม่สั่งฮาร์ดแวร์ ค่า motor ในรายงานถูก clamp -100..100 ตาม driver จริง map จำลองมี guard เพื่อรายงานตัวหารศูนย์แทนการ crash; กรณี avg/sum ใน readline รัน body จริงใน subprocess แยกและเกิด arithmetic failure

Calibration samples 1023 then 100: F min/max=100/100, B=100/100 (true extrema 100/1023)
After above calibration: map denominator zero
Calibration samples 0 then 100: F min/max=100/100 (true extrema 0/100)
Calibration without contrast: map denominator zero
Equal normalized weights=25 over eight sensors: F=352, B=352; center should be 350
No line from startup: reported position=0, motors=-40,100; no lost flag
No line, until_frontSensor: after 10001 simulated ms still motors=-40,100, ao called=0
One continuously occupied crossing counts as three: motor updates=3, elapsed=153ms
One line sensor outside calibration span: clipped position=300, raw sum=21 (clipped sum should be 100)
New centered segment after previous edge + ao: error=0, D=-350, motors=-100,100
Negative-speed encoder helper: front ADC reads=40, back ADC reads=0
readline with sensitivity=100, each normalized value=1: 
Sensitivity case process exit=3221225620 (nonzero arithmetic failure)

เวลาในผลทดสอบเป็นเวลาจำลองที่ไม่รวมเวลาอ่าน ADC จริง และ watchdog ที่หยุดตัวอย่าง until_sensor เป็นของ harness ไม่ใช่ของ Library ไม่อ้างว่าได้วัดความแม่นยำบนหุ่นจริง

## ลำดับที่แนะนำ

แก้การเก็บ Min/Max และตรวจข้อมูลผิดก่อน ตามด้วย lost-line/timeout, reset ประวัติ PD, centroid และผลรวม frame เดียวกัน แล้วทดสอบทิศซ้าย–ขวาและจูน Kp/Kd กับแผงจริง การนับแยกต้องตรวจการเข้า/ออก ไม่ใช้เวลาหน่วงแทนการพ้นแยก
